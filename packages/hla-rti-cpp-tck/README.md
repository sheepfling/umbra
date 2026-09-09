# Portable C++ HLA TCK

`hla-rti-cpp-tck` is a vendor-neutral IEEE 1516.1-2025 C++ test executable.
The reusable source includes only the official `RTI` headers and the C++
standard library. It does not include a provider transport, private provider
headers, a provider test fixture, or a test framework.
The test logic calls services through `RTIambassador` and receives them through
a direct `FederateAmbassador` implementation; the standard factory and
configuration types are used only to construct and configure that boundary.
The cross-platform runner is `tools/run_cpp_tck.py`; it invokes CMake and CTest
with direct argument lists and does not require a shell, a private registry, or
provider-specific glue.

The provider adapter supplies the official API include directory, provider
libraries, compile definitions, and link options at configure time. The
reusable target has no provider include-directory input and no provider-specific
setting embedded in its source.

## P0–P6 ordinary-service, time, FOM, DDM, synchronization, callback, and save/restore coverage

The executable covers these ordinary public-API workflows, under both
`HLA_EVOKED` and `HLA_IMMEDIATE` callback models by default:

| Scenario | Standard surface exercised |
| --- | --- |
| `java-tck.factory-discovery` | Standard C++ `RTIambassadorFactory` creation plus official `HLAinteger32BE` encode/decode availability |
| `java-tck.api-surface-inventory` | Official C++ ambassador, callback, `NullFederateAmbassador`, authorization extension, logical-time, configuration, authorization, credential, exception, handle, enum, `RangeBounds`, federation-record, `DataElement`, `EncoderException`, and `VariableLengthData` types plus factory, configuration-builder copy/assignment/independence, all standard enum-family distinctness, credential and authorization-result copy/assignment, local `Authorizer`/`AuthorizerFactory` polymorphism, all 109 official derived HLA exception constructors with name/message/copy/assignment/polymorphic/stream semantics, every standard null-callback overload invocation, pure `DataElement` clone/type/encoding/boundary/hash/decode semantics, local abstract `LogicalTime`/`LogicalTimeInterval`/`LogicalTimeFactory` contract coverage, copied-storage, borrowed-storage, official handle sets and handle/value maps with copy/assignment/lookup/erase/deep-independence semantics, federation-record and pair/vector copy/assignment/deep-independence checks, collection, and replacement checks |
| `cpp-tck.variable-length-data-contract` | Provider- and FOM-independent `VariableLengthData` ownership and storage contract: caller-owned copies, copy/assignment independence, borrowed storage, custom-deleter adoption and replacement, empty construction, and the standard array-deleter `takeDataPointer` overload |
| `cpp-tck.logical-time-contract` | Provider- and FOM-independent concrete standard logical-time value and factory contract: integer and floating-point logical times and intervals, exact encodings, direct-buffer decode, initial/final/epsilon boundaries, arithmetic, copy independence, invalid-value boundaries, and factory selection |
| `cpp-tck.logical-time-factory-factory-contract` | Provider- and FOM-independent official logical-time factory-factory contract: standard default and integer selection, reference-factory forwarding, unknown-name rejection, and initial-value construction |
| `cpp-tck.logical-time-data-elements-contract` | Standard `HLAlogicalTime` and `HLAlogicalTimeInterval` DataElement wrappers: selected-factory round trips, nested-buffer encode/decode, clone and copy independence, type compatibility, boundaries, and truncation failures; provider, FOM, endpoint, callback model, and time implementation are supplied by the adapter |
| `cpp-tck.exception-hierarchy-contract` | Provider- and FOM-independent official C++ exception hierarchy contract: every standard derived exception constructor, name/message accessors, copy/assignment, polymorphic base behavior, and stream output |
| `cpp-tck.enum-contract` | Provider- and FOM-independent official C++ enumeration contract: distinct standard values for settings, callback, order, resign, save, restore, service-group, synchronization, and authorization-result families |
| `cpp-tck.handle-and-collection-contract` | Provider- and FOM-independent official C++ handle and collection contract: invalid-handle identity, hashing/order, handle sets, handle-value maps, `RangeBounds`, region-pair vectors, and federation/restore record vectors |
| `cpp-tck.configuration-and-authorization-contract` | Provider- and FOM-independent official C++ configuration, federation-record, credential, authorization, and local `Authorizer`/`AuthorizerFactory` contract: builder/accessor behavior, copy/assignment independence, credential decoding/storage, authorization results, and polymorphic dispatch |
| `cpp-tck.authorizer-factory-factory-contract` | Provider- and FOM-independent official C++ `HLAauthorizerFactoryFactory` contract: standard-authorizer selection, factory and authorizer naming/creation, and unsupported-name rejection |
| `cpp-tck.runtime-identity-contract` | Provider-neutral official C++ `rtiName()`/`rtiVersion()` contract: callable, non-empty, process-stable runtime identity without asserting vendor-specific strings |
| `cpp-tck.rti-ambassador-factory-contract` | Provider- and FOM-independent official C++ `RTIambassadorFactory` construction contract: repeatable creation of usable standard `RTIambassador` objects without provider, endpoint, or FOM assumptions |
| `cpp-tck.null-federate-ambassador-contract` | Provider- and FOM-independent official C++ `NullFederateAmbassador` callback contract: every standard no-op callback family and each ordinary/timestamped callback overload used by the portable TCK |
| `cpp-tck.data-element-contract` | Provider- and FOM-independent official C++ `DataElement` base contract: clone/type identity, encoded length and boundary, hashing, complete append encoding, direct decode, and offset decode |
| `cpp-tck.basic-data-elements-contract` | Provider- and FOM-independent scalar `BasicDataElements` contract: exact standard wire encodings and decode/validation boundaries for integer, Boolean, octet/byte, floating-point, ASCII/Unicode, opaque-data, and octet-pair types |
| `cpp-tck.composite-data-elements-contract` | Provider- and FOM-independent composite `DataElement` contract: fixed/variable arrays, aligned fixed records, nested record arrays, mapped and unknown variant alternatives, extendable variants, typed forms, and malformed padding/length boundaries |
| `cpp-tck.connection-callback-contract` | Standard adapter-backed connection and callback-control contract: all four `connect` overloads, callback-model rejection, pre-connect boundaries, duplicate-connect handling, callback enable/disable and servicing, disconnect, and reconnect |
| `cpp-tck.federation-lifecycle-contract` | Standard adapter-backed federation lifecycle contract: create/join/resign/destroy, automatic-resign directives, federation/member reports, federate handle lookups, duplicate-membership failures, and missing-federation boundaries |
| `java-tck.overloads-and-exceptions` | Pre-connect `NotConnected` boundaries for listing, lookup, name reservation, and disconnect; all four official Connect overloads, unsupported callback-model rejection, duplicate-connect handling, reconnect-after-disconnect, empty-queue callback servicing, Disable/Enable Callbacks, and Disconnect |
| `java-tck.encoder-round-trip` | Official 16/32/64-bit integer, boolean, floating-point, UTF-16BE text, opaque, array, fixed-record, variant-record, and extendable-variant-record encodings, including alignment, signed element counts, caller-owned opaque storage, copied/borrowed storage, type-shape, custom boundary, and malformed-value boundaries |
| `java-tck.malformed-inputs` | Missing/malformed FOM sources and malformed encoded values |
| `java-tck.federation-membership` | Create/Join/Resign/Destroy, automatic-resign directive pre-connect/pre-join boundaries and round trips, duplicate-create/join and destroy-while-joined failures, federation/member reports, federate handle lookups, missing-federation report, and the standard boundary that ordinary self-resignation does not synthesize `federateResigned` |
| `cpp-tck.unnamed-join-overload` | Standard unnamed Join overload, generated federate-name and handle round trip, named-join control membership, and member-list reporting |
| `cpp-tck.unnamed-join-overload-contract` | Standard adapter-backed unnamed federation join overload contract for generated-name identity, member reporting, federate lookup, and cleanup |
| `cpp-tck.federation-list-services` | Focused federation-execution and member-list reports, missing-execution reporting, callback-boundary behavior, and stale evoked-report cleanup |
| `cpp-tck.federation-list-services-contract` | Standard adapter-backed federation-execution and member-list reporting contract across callback boundaries, resignation, disconnect, and destruction |
| `cpp-tck.federate-lookup-lifecycle` | Federate handle/name lookup boundaries before membership, across active members, foreign handles, and resignation |
| `cpp-tck.federate-lookup-lifecycle-contract` | Standard adapter-backed federate handle/name lookup contract across membership, invalid inputs, foreign handles, and resignation |
| `cpp-tck.explicit-mim-creation` | Adapter-supplied standard MIM composition during federation creation, shared MOM declaration handles, and ordinary FOM preservation |
| `cpp-tck.explicit-mim-creation-contract` | Pure standard C++ contract for adapter-supplied MIM composition, reserved standard designators, shared MOM declarations, and ordinary FOM preservation |
| `cpp-tck.federation-mom-current-fdd` | Standard federation MOM discovery, `HLAcurrentFDD` request/reflection, reliable transportation reporting, and refresh after an additional-FOM join |
| `cpp-tck.federation-mom-current-fdd-contract` | Pure standard C++ contract for federation-MOM `HLAcurrentFDD` discovery, request/reflection, transportation reporting, and additional-FOM refresh |
| `cpp-tck.federation-mom-save-conditionals-contract` | Pure standard C++ contract for federation MOM save-conditionals, timed save initiation/completion, and adapter-supplied logical-time/MIM inputs |
| `cpp-tck.joined-federate-mom-federate-state-save-restore` | Standard joined-federate MOM `HLAfederateState` transitions across save initiation/completion and restore initiation/completion, with callback and reflection metadata checks |
| `cpp-tck.joined-federate-mom-federate-state-save-restore-contract` | Pure standard C++ contract for joined-federate MOM save/restore state transitions using adapter-supplied provider, FOM, endpoint, callback, and logical-time configuration |
| `cpp-tck.service-report-interaction-failure` | Standard MOM failure reports for invalid ordinary interaction class, parameter, and publication inputs, including typed report arguments and no application callback |
| `cpp-tck.service-report-interaction-failure-contract` | Standard adapter-backed ordinary interaction service-report failure contract using the official MIM and interaction callbacks |
| `cpp-tck.service-report-interaction-contract` | Standard adapter-backed ordinary interaction service-report contract using the official MIM and interaction callbacks |
| `cpp-tck.service-report-regional-interaction` | Standard MOM service-report callback for regional `SendInteractionWithRegions`, paired with overlap-qualified regional application delivery and conveyed source-region metadata |
| `cpp-tck.service-report-regional-interaction-contract` | Pure standard C++ contract for successful regional interaction service reporting, typed MOM invocation metadata, and overlap-qualified delivery using adapter-supplied MIM/DDM inputs |
| `cpp-tck.service-report-regional-interaction-subscription` | Standard MOM service-report callbacks for regional `SubscribeInteractionClassWithRegions` and `UnsubscribeInteractionClassWithRegions`, including the passive-subscription indicator, typed association arguments, serial progression, and standard MIM/DDM setup |
| `cpp-tck.service-report-regional-interaction-subscription-contract` | Pure standard C++ contract for regional interaction subscription/unsubscription reports, passive state, typed associations, serial progression, and adapter-supplied MIM/DDM inputs |
| `cpp-tck.service-report-regional-interaction-failure` | Standard MOM failure reports for invalid regional interaction class, parameter, and region inputs, including typed report arguments and no application callback |
| `cpp-tck.service-report-regional-interaction-failure-contract` | Standard adapter-backed regional interaction service-report failure contract using the official MIM, DDM, and interaction callbacks |
| `cpp-tck.service-report-attribute-update` | Standard MOM service-report callback for successful ordinary `UpdateAttributeValues`, with typed report metadata and ordinary registration/publication setup |
| `cpp-tck.service-report-attribute-update-contract` | Standard adapter-backed ordinary attribute-update service-report contract using the official MIM and object callbacks |
| `cpp-tck.service-report-attribute-update-failure` | Standard MOM failure reports for unknown-object and undefined-attribute `UpdateAttributeValues`, including typed arguments, serial progression, and no reflection callback |
| `cpp-tck.service-report-attribute-update-failure-contract` | Standard adapter-backed ordinary attribute-update service-report failure contract using the official MIM and object callbacks |
| `cpp-tck.service-report-timestamped-attribute-update-failure` | Standard MOM failure reports for unknown-object, undefined-attribute, and invalid-logical-time timestamped `UpdateAttributeValues`, including the optional timestamp argument and no reflection callback |
| `cpp-tck.service-report-timestamped-attribute-update-failure-contract` | Standard adapter-backed timestamped attribute-update service-report failure contract using the official MIM, logical-time, and object callbacks |
| `cpp-tck.service-report-timestamped-interaction` | Standard MOM service-report callback for successful timestamped ordinary interaction delivery, paired with constrained time advancement and retraction metadata |
| `cpp-tck.service-report-timestamped-interaction-contract` | Standard adapter-backed timestamped interaction service-report contract using the official MIM, logical-time, and interaction callbacks |
| `cpp-tck.service-report-timestamped-interaction-failure` | Standard MOM failure reports for invalid timestamped interaction class, parameter, and logical-time inputs, including the optional timestamp argument and no application callback |
| `cpp-tck.service-report-timestamped-interaction-failure-contract` | Standard adapter-backed timestamped interaction service-report failure contract using the official MIM, logical-time, and interaction callbacks |
| `cpp-tck.service-report-timestamped-delete-object-instance-failure` | Standard MOM failure reports for unknown-object and invalid-logical-time timestamped `DeleteObjectInstance`, including the optional timestamp argument and no removal callback |
| `cpp-tck.service-report-timestamped-delete-object-instance-failure-contract` | Standard adapter-backed timestamped object-deletion service-report failure contract using the official MIM, logical-time, and object callbacks |
| `cpp-tck.service-report-request-attribute-value-update` | Standard MOM service-report callback for successful known-object and object-class attribute-value requests, paired with the standard provider callbacks |
| `cpp-tck.service-report-request-attribute-value-update-contract` | Standard adapter-backed RequestAttributeValueUpdate service-report contract using the official MIM, object, attribute, and provider callbacks |
| `cpp-tck.service-report-release-multiple-object-instance-names` | Standard MOM service-report callback for successful multiple object-name release after a standard multiple-reservation-success callback |
| `cpp-tck.service-report-release-multiple-object-instance-names-contract` | Standard adapter-backed multiple object-name release service-report contract using the official MIM and reservation callbacks |
| `cpp-tck.service-report-release-object-instance-name` | Standard MOM service-report callback for successful object-name release after a standard reservation-success callback |
| `cpp-tck.service-report-release-object-instance-name-contract` | Standard adapter-backed object-name release service-report contract using the official MIM and reservation callbacks |
| `cpp-tck.service-report-reserve-object-instance-name` | Standard MOM service-report callback for successful object-name reservation, paired with the standard reservation-success callback and seven-parameter report contract |
| `cpp-tck.service-report-reserve-object-instance-name-contract` | Standard adapter-backed object-name reservation service-report contract using the official MIM and reservation callbacks |
| `cpp-tck.service-report-register-object-instance` | Standard MOM service-report callback for successful object registration, paired with ordinary discovery delivery and the seven-parameter report contract |
| `cpp-tck.service-report-register-object-instance-contract` | Standard adapter-backed ordinary object-registration service-report contract using the official MIM and discovery callback |
| `cpp-tck.service-report-local-delete-object-instance` | Standard MOM service-report callback for successful local object deletion, including service identity, type, success, exception, serial, transport, and callback metadata |
| `cpp-tck.service-report-local-delete-object-instance-contract` | Standard adapter-backed local object-deletion service-report contract using the official MIM and object callbacks |
| `cpp-tck.service-report-local-delete-object-instance-failure` | Standard MOM failure reports for invalid and stale local object deletion, including failure status, exception, returned-argument encoding, serial progression, and cleanup |
| `cpp-tck.service-report-local-delete-object-instance-failure-contract` | Standard adapter-backed local object-deletion failure-report contract using the official MIM, object, and standard data-element callbacks |
| `cpp-tck.attribute-value-update-request-baseline` | Ordinary object-instance Request Attribute Value Update solicitation, exact current-owner callback metadata, request-tag propagation, and requester-owned/unowned suppression |
| `cpp-tck.attribute-value-update-request-baseline-contract` | Standard adapter-backed object-instance Request Attribute Value Update contract for ordinary publication, subscription, registration, provider solicitation, and cleanup |
| `cpp-tck.object-class-attribute-value-update-request-baseline` | Object-class Request Attribute Value Update expansion over concrete subclass instances, exact per-object owner callbacks, inherited-attribute filtering, and requester-owned suppression |
| `cpp-tck.object-class-attribute-value-update-request-baseline-contract` | Standard adapter-backed object-class Request Attribute Value Update contract over concrete subclass instances and provider callbacks |
| `cpp-tck.attribute-value-update-response` | Ordinary attribute-value request/response metadata, provider callback ordering, returned value/tag, reliable transport, producer identity, and empty region metadata |
| `cpp-tck.attribute-value-update-response-contract` | Standard adapter-backed ordinary attribute-value request, provider response, Update/Reflect metadata, transportation identity, and cleanup contract |
| `cpp-tck.service-report-delete-object-instance` | Standard MOM service-report callback for successful object deletion, paired with ordinary removal delivery and the same seven-parameter report contract |
| `cpp-tck.service-report-delete-object-instance-contract` | Standard adapter-backed ordinary object-deletion service-report contract using the official MIM and removal callback |
| `cpp-tck.service-report-delete-object-instance-failure` | Standard MOM failure reports for invalid and stale object deletion, including failure status, exception, returned-argument encoding, serial progression, and removal cleanup |
| `cpp-tck.service-report-delete-object-instance-failure-contract` | Standard adapter-backed object-deletion failure-report contract using the official MIM, object, and standard data-element callbacks |
| `java-tck.logical-time-factory` | Pre-connect and pre-join `getTimeFactory` lifecycle boundaries, standard `HLAinteger64Time`/`HLAfloat64Time` value and factory checks, concrete time/interval copy and assignment independence, zero/epsilon mutators, interval differences, direct interval encoding, default/named/unknown logical-time factory selection, adapter-selected logical-time values and zero/epsilon intervals, public `HLAlogicalTime`/`HLAlogicalTimeInterval` data-element round trips, nested-buffer boundaries, clone/copy independence, incompatible-type rejection, variable-length and direct-buffer encode/decode parity, encoded-length checks, truncated-buffer rejection, boundary transitions, comparison, interval arithmetic/order, and illegal underflow/overflow boundaries |
| `java-tck.time-advance` | Time-service `NotConnected`/`FederateNotExecutionMember` boundaries, Time Regulation/Constrained roles, pre-membership lookahead boundaries, logical-time and lookahead queries, deferred lookahead decrease, all alternate advance entry points, asynchronous-delivery controls, Time Advance Request/Grant, standard duplicate-role, in-progress, backward-time, and duplicate-disable failure boundaries, and role shutdown |
| `java-tck.support-services` | Full public federate, object, attribute, interaction, parameter, object-instance, and dimension name/handle lookup in both directions with pre-connect/pre-join lifecycle boundaries, standard invalid-name/handle boundaries, order/transport/update-rate/normalization lookup lifecycle boundaries, all public handle decoder lifecycle boundaries, ordinary handle encode/decode, direct-buffer and `VariableLengthData&` handle encoding, encoded-length and truncated-buffer checks, copied-handle equality/hash/ordering stability, valid `AttributeHandleSet` copy/assignment/lookup/erase semantics, independent `AttributeHandleValueMap` and `ParameterHandleValueMap` value storage, normalization stability, available-dimension boundaries, and order/transportation handles |
| `cpp-tck.support-services-contract` | Standard adapter-backed support-service contract for public lookup, normalization, handle encoding/decoding, available dimensions, order, transportation, update-rate, and lifecycle-boundary behavior |
| `cpp-tck.standard-order-and-transportation-lookups-contract` | Standard adapter-backed mandatory order and transportation lookup contract across lifecycle admission, round trips, invalid inputs, and cleanup |
| `java-tck.declaration-management` | Pre-connect and pre-join lifecycle boundaries for object and interaction declarations, ordinary object publication/subscription, active and passive declarations, interaction publication/subscription, withdrawal, and invalid-class/attribute failure boundaries |
| `cpp-tck.declaration-management-contract` | Standard adapter-backed declaration-management contract for object and interaction publication, active/passive subscription, withdrawal, lifecycle boundaries, and invalid-handle failures |
| `java-tck.object-management` | Pre-connect and pre-join registration, deletion, and object-name reservation lifecycle boundaries, ordinary and named registration/discovery, invalid-class/unreserved-name boundaries, unknown-object deletion, single and multiple name reservation lifecycle, value requests/responses with invalid class/attribute boundaries, local deletion, remote removal, and resign-time cleanup |
| `cpp-tck.object-management-contract` | Standard adapter-backed ordinary object-management contract for registration/discovery, single and multiple name reservation, named registration, identity lookups, value requests/responses, local/remote deletion, and negative boundaries |
| `cpp-tck.resign-delete-objects` | Resign-time deletion of a delete-privileged object, `NO_ACTION` ownership rejection, ordinary removal delivery, producer/tag metadata, and post-removal name invalidation |
| `cpp-tck.resign-delete-objects-contract` | Standard adapter-backed resign-time object deletion contract for ownership failure, removal delivery, metadata, and cleanup |
| `cpp-tck.resign-unconditional-divestiture` | Unconditional resign-time divestiture of the value and delete-privilege attributes, ownership-assumption delivery, surviving-federate acquisition, and acquisition metadata |
| `cpp-tck.resign-unconditional-divestiture-contract` | Standard adapter-backed resign-time unconditional-divestiture contract for ownership transfer, acquisition, metadata, and cleanup |
| `cpp-tck.object-name-reservation-lifecycle` | Single and multiple object-name reservation and release, standard name validation, asynchronous contention results, atomic mixed-set release, reuse after release, and resignation cleanup |
| `cpp-tck.object-name-reservation-lifecycle-contract` | Standard adapter-backed object-instance name reservation lifecycle contract for single and multiple reservation/release, contention, reuse, callbacks, and resignation cleanup |
| `cpp-tck.final-federate-resignation-cleanup` | Final-federate `NO_ACTION` resignation cleanup, object-name release, rejoin, reservation success, and named-registration reuse |
| `cpp-tck.final-federate-resignation-cleanup-contract` | Standard adapter-backed final-federate resignation cleanup contract for name release, rejoin, and named-registration reuse |
| `cpp-tck.object-registration-discovery-lifecycle` | Rich-FOM hierarchy-aware registration/discovery, exact and superclass subscription identity, evoked callback cancellation, late subscription discovery, stable identity lookup, and duplicate-discovery suppression |
| `cpp-tck.object-registration-discovery-lifecycle-contract` | Standard adapter-backed object registration/discovery contract for declaration, registration, discovery, class/name/instance lookups, callback servicing, and lifecycle boundaries |
| `cpp-tck.named-registration` | Standard object-name reservation/release, multiple-name reservation, named registration, discovery and identity lookup, reservation contention, failed-registration reuse, and callback-model parity |
| `cpp-tck.named-registration-contract` | Standard adapter-backed named object-registration contract for reservation/release and reuse, multiple-name lifecycle, discovery and identity lookup, contention, and invalid-name boundaries |
| `cpp-tck.object-attribute-subscription-lifecycle-contract` | Standard adapter-backed ordinary object-attribute subscription lifecycle for passive/active discovery, reflection, downgrade/reactivation, unsubscription, and stable identity lookups |
| `cpp-tck.local-delete-object-instance` | Standard local object deletion boundaries, ownership protection, pending-acquisition protection, fresh-session deletion, rediscovery, and continued ordinary attribute reflection |
| `cpp-tck.local-delete-object-instance-contract` | Standard adapter-backed local object deletion contract for service boundaries, ownership protection, fresh-session deletion, rediscovery, stable identity, and continued ordinary reflection |
| `java-tck.attribute-interaction` | Pre-connect and pre-join ordinary interaction-send boundaries plus active and passive attribute Update/Reflect and interaction Send/Receive, values, parameters, tags, producer, and transportation |
| `cpp-tck.attribute-interaction-contract` | Standard adapter-backed ordinary attribute Update/Reflect and interaction Send/Receive contract with values, parameters, tags, producer identity, transportation, and lifecycle boundaries |
| `cpp-tck.order-type-controls-contract` | Standard adapter-backed prospective attribute and interaction order-control contract with logical-time delivery and receive-order callbacks |
| `cpp-tck.receive-order-attribute-update-callback-cancellation` | In evoked mode, cancel a queued receive-order attribute reflection by unsubscribing before callback servicing; in immediate mode, verify delivery before the subscription is removed |
| `cpp-tck.receive-order-attribute-update-callback-cancellation-contract` | Standard adapter-backed receive-order attribute-reflection cancellation contract across evoked and immediate callback boundaries |
| `cpp-tck.receive-order-interaction-callback-cancellation` | In evoked mode, cancel a queued receive-order interaction by unsubscribing before callback servicing; in immediate mode, verify delivery before the subscription is removed |
| `cpp-tck.receive-order-interaction-callback-cancellation-contract` | Standard adapter-backed receive-order interaction cancellation contract across evoked and immediate callback boundaries |
| `cpp-tck.interaction-subscription-lifecycle` | Passive ordinary interaction subscriptions suppress delivery, active replacement enables it without replay, downgrade suppresses later messages, and unsubscribe removes the declaration |
| `cpp-tck.interaction-subscription-lifecycle-contract` | Standard adapter-backed ordinary interaction subscription lifecycle contract for passive/active declarations, downgrade, reactivation, delivery, and unsubscription |
| `cpp-tck.interaction-publication-send-fence` | Whole-class unpublication makes `sendInteraction` fail with `InteractionClassNotPublished`; republication restores parameter delivery and standard metadata |
| `cpp-tck.interaction-publication-send-fence-contract` | Standard adapter-backed ordinary interaction publication, unpublication, send, republication, parameter delivery, and lifecycle contract |
| `cpp-tck.object-attribute-subscription-lifecycle` | Passive ordinary object-attribute subscriptions suppress discovery/reflection, active replacement discovers existing objects and enables reflection, downgrade suppresses later updates, reactivation restores reflection, and unsubscribe removes delivery |
| `cpp-tck.object-publication-registration-fence` | Whole-class unpublication fences object registration with `ObjectClassNotPublished`; republishing restores registration, discovery, and stable identity lookups |
| `cpp-tck.object-publication-registration-fence-contract` | Standard adapter-backed ordinary object publication and registration fence contract using official publication, subscription, registration, lookup, discovery, and lifecycle APIs |
| `cpp-tck.timestamped-interactions` | Timestamped Send/Receive, invalid interaction-class and retraction-handle boundaries, constrained grant delivery, timestamp/order metadata, retraction designators, and Request Retraction callbacks |
| `cpp-tck.timestamped-interactions-contract` | Standard adapter-backed timestamped interaction and retraction contract using official time-role and interaction callbacks |
| `cpp-tck.timestamped-interaction-source-resignation-contract` | Standard adapter-backed queued timestamped interaction source-resignation contract, including the post-resignation retraction boundary and preserved delivery metadata |
| `cpp-tck.timestamped-interaction-source-resignation-fanout-contract` | Standard adapter-backed timestamped interaction source-resignation fan-out contract with independent recipient TAR/NMR servicing and preserved per-recipient delivery metadata |
| `cpp-tck.timestamped-interaction-source-resignation` | A queued timestamped interaction remains deliverable after producer resignation, preserving parameter, tag, producer, time/order, transport, and retraction metadata before the recipient TAR grant |
| `cpp-tck.timestamped-interaction-source-resignation-fanout` | Queued timestamped interactions remain deliverable to each constrained recipient after producer resignation, using independent TAR/NMR frontiers and preserving payload, producer, time/order, and retraction metadata |
| `cpp-tck.timestamped-interaction-mixed-advances` | One queued timestamped interaction is delivered before Flush Queue, Time Advance Request Available, and Next Message Request Available grants, with callback ordering, logical-time queries, and retraction metadata |
| `cpp-tck.timestamped-interaction-mixed-advances-contract` | Standard adapter-backed timestamped interaction contract for mixed Flush Queue, Time Advance Request Available, and Next Message Request Available delivery |
| `cpp-tck.timestamped-interaction-flush-queue-future-input` | Future timestamped interactions accepted around a Flush Queue request are delivered before its grant, preserving callback order, payload, time/order, transport, and retraction metadata while reporting the optimistic frontier |
| `cpp-tck.timestamped-interaction-flush-queue-future-input-contract` | Standard adapter-backed future-input timestamped interaction and Flush Queue contract with frontier, retraction, callback, and cleanup assertions |
| `cpp-tck.timestamped-interaction-tso-designator-terminalization` | All standard producer advance forms terminalize expired timestamped-interaction retraction handles without delivering to an idle constrained receiver |
| `cpp-tck.timestamped-interaction-tso-designator-terminalization-contract` | Standard adapter-backed timestamped interaction retraction-terminalization contract across all alternate advance forms |
| `cpp-tck.timestamped-interaction-cross-producer-order` | Timestamp order is preserved across multiple interaction producers for each constrained recipient, while equal-timestamp ordering remains unconstrained and producer/tag identity is preserved |
| `cpp-tck.timestamped-interaction-cross-producer-order-contract` | Standard adapter-backed timestamped interaction ordering contract across multiple producers and constrained recipients |
| `cpp-tck.timestamped-interaction-no-fanout` | Timestamped interaction sends return usable retraction handles and preserve terminal retraction behavior when no subscribing federate is eligible |
| `cpp-tck.timestamped-interaction-no-fanout-contract` | Standard adapter-backed timestamped interaction no-fan-out and terminal-retraction contract |
| `cpp-tck.timestamped-interaction-retraction-fanout` | Delivered timestamped interaction copies produce Request Retraction callbacks while still-queued fan-out copies are suppressed, including post-resignation immediate delivery |
| `cpp-tck.timestamped-interaction-retraction-fanout-contract` | Standard adapter-backed timestamped interaction delivered/queued retraction fan-out contract with recipient cleanup |
| `cpp-tck.timestamped-interaction-regulation-reenable` | Queued timestamped interaction survives Time Regulation disable/re-enable with changed lookahead, including Query Lookahead, grant ordering, metadata, and retraction |
| `cpp-tck.timestamped-interaction-regulation-reenable-contract` | Standard adapter-backed timestamped interaction Time Regulation re-enable contract for changed lookahead, grant ordering, metadata, and retraction |
| `cpp-tck.timestamped-interaction-reenable-contract` | Standard adapter-backed timestamped interaction Time Constrained re-enable contract for queued delivery, grant ordering, metadata, and retraction |
| `cpp-tck.timestamped-directed-interaction-reenable-contract` | Standard adapter-backed timestamped directed-interaction Time Constrained re-enable contract for target routing, grant ordering, metadata, and retraction |
| `cpp-tck.timestamped-directed-interaction-regulation-reenable` | Queued timestamped directed interaction survives Time Regulation disable/re-enable with changed lookahead, including target routing, Query Lookahead, grant ordering, metadata, and terminal retraction |
| `cpp-tck.timestamped-directed-interaction-regulation-reenable-contract` | Standard adapter-backed timestamped directed-interaction Time Regulation re-enable contract for target routing, changed lookahead, grant ordering, metadata, and retraction |
| `cpp-tck.timestamped-directed-interaction-source-resignation` | A queued timestamped directed interaction remains deliverable after producer resignation, preserving target, parameter, tag, producer, time/order, transport, and retraction metadata before the recipient TAR grant |
| `cpp-tck.timestamped-directed-interaction-source-resignation-fanout` | One queued timestamped directed interaction is delivered independently to two constrained recipients after producer resignation, preserving target, payload, producer, time/order, transport, retraction, and each recipient's TAR frontier |
| `cpp-tck.timestamped-directed-interaction-source-resignation-contract` | Standard adapter-backed queued timestamped directed-interaction source-resignation contract covering target routing, post-resignation delivery, metadata, and retraction boundaries |
| `cpp-tck.timestamped-directed-interaction-source-resignation-fanout-contract` | Standard adapter-backed timestamped directed-interaction source-resignation fan-out contract covering independent recipient TAR servicing, target/payload metadata, and per-recipient retraction boundaries |
| `cpp-tck.timestamped-directed-interaction-tar-nmr` | One timestamped directed interaction reaches two constrained recipients through independent TAR(7) and NMR(10) requests, with callback-before-grant ordering, target/payload/time/order/transport/retraction metadata, and logical-time query checks |
| `cpp-tck.timestamped-directed-interaction-tar-nmr-contract` | Standard adapter-backed timestamped directed-interaction TAR/NMR contract for target routing, callback-before-grant ordering, timestamp/order/retraction metadata, and independent time-advance servicing |
| `cpp-tck.timestamped-directed-interaction-immediate-source-resignation` | An accepted timestamped directed interaction remains deliverable on an immediate callback route after producer resignation, preserving target, parameter, tag, producer, time/order, transport, and retraction metadata |
| `cpp-tck.timestamped-directed-interaction-immediate-source-resignation-contract` | Standard adapter-backed immediate timestamped directed-interaction source-resignation contract for accepted delivery, callback metadata, resignation, and cleanup boundaries |
| `cpp-tck.timestamped-attribute-update` | Timestamped ordinary Update/Reflect, invalid object/attribute boundaries, immediate versus constrained delivery, timestamp/order metadata, and attribute-update retraction |
| `cpp-tck.timestamped-attribute-update-contract` | Standard adapter-backed timestamped ordinary attribute-update contract for Update/Reflect, retraction, time-role servicing, and invalid-handle boundaries |
| `cpp-tck.timestamped-attribute-update-ownership-transfer` | A timestamped attribute update remains deliverable after unconditional divestiture and If Available acquisition before the constrained grant, preserving value, tag, timestamp, producer, order, and retraction metadata |
| `cpp-tck.timestamped-attribute-update-ownership-transfer-contract` | Standard adapter-backed timestamped attribute ownership-transfer contract for queued delivery, ownership callbacks, reflection metadata, time advancement, and retraction |
| `cpp-tck.timestamped-attribute-source-resignation` | A queued timestamped attribute update remains deliverable after source resignation and ownership acquisition, preserving value, tag, time, producer, order, transportation, and retraction metadata before the grant |
| `cpp-tck.timestamped-attribute-source-resignation-contract` | Standard adapter-backed queued timestamped attribute source-resignation contract for ownership transfer, post-resignation delivery, reflection metadata, and retraction boundaries |
| `cpp-tck.timestamped-attribute-source-resignation-fanout` | A queued timestamped attribute update remains deliverable to each constrained recipient after source resignation, preserving per-recipient value, producer, time/order, transportation, and retraction metadata |
| `cpp-tck.timestamped-attribute-source-resignation-fanout-contract` | Standard adapter-backed timestamped attribute source-resignation fan-out contract for independent recipient grants, delivery metadata, and retraction boundaries |
| `cpp-tck.timestamped-attribute-order-cohort` | Timestamped ordinary attribute updates are ordered by timestamp for two constrained recipients, with the complete equal-timestamp cohort delivered before each grant and value, tag, producer, order, transportation, and retraction metadata preserved |
| `cpp-tck.timestamped-attribute-order-cohort-contract` | Standard adapter-backed timestamped attribute ordering and equal-timestamp cohort contract |
| `cpp-tck.timestamped-attribute-update-queued-passel-retraction` | Timestamped multi-attribute updates retain their adapter-defined passel shape, suppress a retracted queued update, and preserve value, tag, time, producer, order, transportation, and retraction metadata before the grant |
| `cpp-tck.timestamped-attribute-update-queued-passel-retraction-contract` | Standard adapter-backed timestamped multi-attribute passel retraction contract |
| `cpp-tck.timestamped-attribute-update-no-fanout-contract` | Standard adapter-backed timestamped attribute no-fan-out and terminal-retraction contract |
| `cpp-tck.timestamped-attribute-update-alternate-advances` | Timestamped ordinary Update/Reflect through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with callback-before-grant ordering, grant/query checks, and terminal retraction |
| `cpp-tck.timestamped-attribute-update-alternate-advances-contract` | Standard adapter-backed timestamped attribute contract across alternate advance services |
| `cpp-tck.timestamped-attribute-update-flush-queue-future-input` | Future timestamped attribute updates accepted beyond a Flush Queue boundary are delivered before its grant, preserving value, tag, time/order, transport, and retraction metadata while reporting the optimistic frontier |
| `cpp-tck.timestamped-attribute-update-flush-queue-future-input-contract` | Standard adapter-backed future timestamped attribute input and Flush Queue contract |
| `cpp-tck.timestamped-attribute-update-reenable` | A queued timestamped attribute update remains deliverable across Time Constrained disable/re-enable, preserving reflection metadata and terminal retraction |
| `cpp-tck.timestamped-attribute-update-reenable-contract` | Standard adapter-backed timestamped attribute Time Constrained re-enable contract |
| `cpp-tck.timestamped-attribute-update-regulation-reenable` | A queued timestamped attribute update remains deliverable across Time Regulation disable/re-enable with changed lookahead, preserving Query Lookahead, reflection metadata, grant ordering, and terminal retraction |
| `cpp-tck.timestamped-attribute-update-regulation-reenable-contract` | Standard adapter-backed timestamped attribute Time Regulation re-enable contract with changed lookahead and delivery-boundary assertions |
| `cpp-tck.timestamped-regional-attribute-update` | Timestamped region-qualified Update/Reflect, overlap and disjoint filtering, source-region metadata, constrained grant ordering, time-constrained re-enable, and retraction |
| `cpp-tck.timestamped-regional-attribute-alternate-advances` | Timestamped regional Update/Reflect through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with grant/query bounds and backward-time failures |
| `cpp-tck.timestamped-regional-attribute-association-replacement` | Queued timestamped regional updates retain their original source association while later updates use an explicitly replaced source region |
| `cpp-tck.timestamped-regional-attribute-regulation-reenable` | Queued timestamped regional Update/Reflect survives Time Regulation disable/re-enable with changed lookahead, preserving the original source-region snapshot, grant ordering, and retraction |
| `cpp-tck.timestamped-regional-attribute-source-resignation` | A queued timestamped regional Update/Reflect remains deliverable after its producer resigns, preserving object, producer, source-region, payload, time, order, and retraction metadata |
| `cpp-tck.timestamped-default-region-attribute-alternate-advances` | Timestamped default-region Update/Reflect through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with supplied-empty region metadata and grant ordering |
| `cpp-tck.timestamped-default-region-attribute-reenable` | Queued timestamped default-region Update/Reflect survives Time Constrained disable/re-enable, with supplied-empty source metadata, grant ordering, and retraction |
| `cpp-tck.timestamped-default-region-attribute-regulation-reenable` | Queued timestamped default-region Update/Reflect survives Time Regulation disable/re-enable with changed lookahead, including Query Lookahead, grant ordering, supplied-empty source metadata, and retraction |
| `cpp-tck.timestamped-default-region-attribute-mixed-fanout` | Timestamped default-region Update/Reflect splits between an immediate and a constrained regional recipient, with delivered-copy Request Retraction and pending-copy suppression |
| `cpp-tck.timestamped-default-region-interaction` | Timestamped default-region interaction delivery carries supplied-empty source-region metadata through a constrained grant and terminal retraction boundary |
| `cpp-tck.timestamped-default-region-interaction-alternate-advances` | Timestamped default-region interactions are delivered through Flush Queue Request, Time Advance Request Available, and Next Message Request Available with supplied-empty region metadata and grant/query bounds |
| `cpp-tck.timestamped-default-region-interaction-mixed-fanout` | Timestamped default-region interaction delivery splits between an immediate and a constrained regional recipient, with delivered-copy Request Retraction and recipient-local callback ordering |
| `cpp-tck.timestamped-default-region-interaction-source-resignation` | A queued timestamped default-region interaction remains deliverable to each constrained regional recipient after producer resignation, preserving supplied-empty region, producer, payload, time, order, and retraction metadata |
| `cpp-tck.timestamped-default-region-interaction-reenable` | Queued timestamped default-region interaction survives Time Constrained disable/re-enable, with supplied-empty source metadata, callback-before-grant ordering, and terminal retraction |
| `cpp-tck.timestamped-default-region-interaction-regulation-reenable` | Queued timestamped default-region interaction survives Time Regulation disable/re-enable with changed lookahead, Query Lookahead, supplied-empty source metadata, and terminal retraction |
| `cpp-tck.timestamped-object-deletion` | Timestamped Delete/Remove Object Instance, unknown-object boundary, object identity removal and reconstitution, timestamp/order metadata, and deletion retraction |
| `cpp-tck.timestamped-object-deletion-contract` | Standard adapter-backed timestamped ordinary object-deletion contract for Delete/Remove, retraction, identity, time-role servicing, and invalid-object boundaries |
| `cpp-tck.timestamped-object-deletion-no-fanout-contract` | Standard adapter-backed no-recipient timestamped object-deletion contract for exact-boundary retraction, name/identity restoration, ownership restoration, and no retraction fan-out |
| `cpp-tck.timestamped-object-deletion-tombstone-contract` | Standard adapter-backed timestamped object-deletion tombstone contract for terminal deletion, non-retractability, named registration reuse, and identity boundaries |
| `cpp-tck.timestamped-object-deletion-regulation-reenable-contract` | Standard adapter-backed timestamped object-deletion Time Regulation re-enable contract for changed lookahead, removal-before-grant ordering, metadata, identity cleanup, and terminal retraction |
| `cpp-tck.timestamped-object-deletion-source-resignation-fanout-contract` | Standard adapter-backed timestamped object-deletion source-resignation fan-out contract for independent recipient grants, removal metadata, and retraction boundaries |
| `cpp-tck.timestamped-object-deletion-retraction-joined-owners-contract` | Standard adapter-backed timestamped object-deletion joined-owner retraction contract for ownership cleanup, departed-recipient suppression, identity restoration, and retraction boundaries |
| `cpp-tck.timestamped-object-deletion-mixed-advances-contract` | Standard adapter-backed timestamped object-deletion contract for Flush Queue, Time Advance Request Available, and Next Message Request Available delivery paths |
| `cpp-tck.timestamped-local-delete-object` | Local deletion suppresses one recipient's queued timestamped Remove Object Instance callback while an independent constrained recipient receives the original removal and grant |
| `cpp-tck.timestamped-local-delete-attribute` | Local deletion suppresses one recipient's queued timestamped attribute reflection, while re-subscription restores discovery and a later update reaches both recipients |
| `cpp-tck.timestamped-object-deletion-no-fanout` | Timestamped object deletion at the exact lookahead boundary is non-retractable, while a no-recipient deletion can be retracted to restore object-name lookup and local attribute ownership without Request Retraction fan-out |
| `cpp-tck.timestamped-object-deletion-tombstone` | Terminal timestamped Delete Object Instance becomes non-retractable at the exact lookahead boundary and releases the named object instance for re-registration |
| `cpp-tck.timestamped-object-deletion-regulation-reenable` | Queued timestamped object deletion survives Time Regulation disable/re-enable with changed lookahead, including Query Lookahead, removal-before-grant ordering, identity cleanup, metadata, and terminal retraction |
| `cpp-tck.timestamped-object-deletion-mixed-advances` | A queued timestamped object deletion is delivered before Flush Queue, Time Advance Request Available, and Next Message Request Available grants, with callback ordering, grant/query checks, removal metadata, and terminal retraction |
| `cpp-tck.alternate-time-advances` | Flush Queue Request, Time Advance Request Available, and Next Message Request Available with reflection-before-grant ordering and backward-time failure boundaries |
| `cpp-tck.alternate-time-advances-contract` | Standard adapter-backed contract for Flush Queue, TAR available, and next-message available delivery paths plus backward-time boundaries |
| `cpp-tck.timestamped-directed-alternate-advances` | Timestamped directed delivery through Flush Queue Request, Time Advance Request Available, and Next Message Request Available with target, order, transport, and retraction metadata |
| `cpp-tck.timestamped-directed-alternate-advances-contract` | Standard adapter-backed timestamped directed-interaction alternate-advance contract for Flush Queue, TAR available, and next-message available delivery |
| `cpp-tck.next-message-request` | Queued timestamped interaction delivery before the Next Message Request grant, callback ordering, time query, and post-delivery retraction boundary |
| `cpp-tck.next-message-request-contract` | Standard adapter-backed Next Message Request contract for queued timestamped interaction delivery, callback ordering, time query, and retraction boundaries |
| `cpp-tck.available-time-advances-inclusive-galt` | Timestamped interaction delivery at the inclusive GALT boundary through Time Advance Request Available and Next Message Request Available, including callback ordering and post-delivery retraction |
| `cpp-tck.available-time-advances-inclusive-galt-contract` | Standard adapter-backed available time-advance contract for inclusive-GALT delivery, callback ordering, and retraction boundaries |
| `cpp-tck.time-bounds-queries` | Undefined and minimum no-TSO Query GALT/Query LITS results, pending regulator advances, grant stability, and resignation/disable transitions |
| `cpp-tck.time-bounds-queries-contract` | Standard adapter-backed Query GALT and Query LITS contract for undefined, active-regulator, pending-advance, resignation, and disconnect boundaries |
| `cpp-tck.query-lits-source-resignation` | Query GALT becomes undefined while Query LITS remains the queued timestamp after the sole source regulator resigns |
| `cpp-tck.query-lits-source-resignation-contract` | Standard adapter-backed Query LITS source-resignation contract for queued timestamp retention, time-management callbacks, and cleanup boundaries |
| `java-tck.directed-interactions` | Pre-connect and pre-join directed declaration and send boundaries, directed interaction publication, selective and universal subscription, target-object routing, callback delivery, unsubscription, unpublication, and invalid object/interaction-handle boundaries |
| `cpp-tck.directed-interaction-contract` | Standard adapter-backed ordinary directed-interaction contract for publication, selective/universal subscription, targeted send/delivery, invalid handles, and lifecycle boundaries |
| `cpp-tck.directed-interaction-publication-send-fence` | Whole-class directed-interaction unpublication fences `sendDirectedInteraction` with `InteractionClassNotPublished`; republication restores targeted parameter delivery and standard metadata |
| `cpp-tck.directed-interaction-publication-send-fence-contract` | Standard adapter-backed directed-interaction publication, unpublication, targeted send, republication, delivery metadata, and lifecycle contract |
| `cpp-tck.directed-interaction-target-lifecycle` | Directed delivery across universal subscription, target discovery, callback-model subscription changes, publication changes, target deletion, and removal cleanup |
| `cpp-tck.directed-interaction-target-lifecycle-contract` | Standard adapter-backed directed-interaction target lifecycle contract for subscription/publication fences, target deletion, removal, and cleanup |
| `cpp-tck.directed-interaction-subscription-kind-contract` | Standard adapter-backed directed-interaction subscription-kind contract for by-ownership/universal declarations, target routing, delivery metadata, and cleanup |
| `cpp-tck.timestamped-directed-interactions` | Timestamped directed Send/Receive, invalid class/target/retraction-handle boundaries, selective and universal target routing, constrained grant delivery, timestamp/order metadata, and retraction lifecycle |
| `cpp-tck.timestamped-directed-interactions-contract` | Standard adapter-backed timestamped directed-interaction and retraction contract using official target-routing, time-role, and callback APIs |
| `cpp-tck.region-lifecycle` | Pre-connect and pre-join region and region-qualified service boundaries, including timestamped regional send, two-dimensional region creation, dimension metadata and bounds, commit/query, and invalid/in-use/delete boundaries |
| `cpp-tck.region-lifecycle-contract` | Standard adapter-backed region and dimension lifecycle contract for region-qualified service boundaries, metadata and bounds, range commit/query, validation, and deletion |
| `cpp-tck.regional-unpublish-region-release` | Region-qualified object registration keeps a region in use until unpublishing the associated attribute releases it synchronously |
| `cpp-tck.regional-unpublish-region-release-contract` | Standard adapter-backed regional publication and region-release dependency contract, including synchronous region deletion after unpublication |
| `cpp-tck.regional-object-update` | Region-qualified publication/subscription, named registration, regional discovery and Update/Reflect, regional value requests, and region association changes |
| `cpp-tck.regional-object-update-contract` | Standard adapter-backed regional object publication, subscription, discovery, Update/Reflect, value-request, and region-reassociation contract |
| `cpp-tck.regional-attribute-value-request-filtering` | Regional Request Attribute Value Update filtering across foreign, incompatible, uncommitted, empty, disjoint, overlapping, and callback-time moved regions |
| `cpp-tck.regional-attribute-value-request-filtering-contract` | Standard adapter-backed regional Request Attribute Value Update filtering contract, including validation, source-region scope, default-region eligibility, and callback-time re-evaluation |
| `cpp-tck.regional-attribute-value-update-response-recheck` | Rechecks current regional overlap when a provider's attribute-value response is reflected, then verifies restored-overlap value, tag, transport, and producer metadata |
| `cpp-tck.regional-attribute-value-update-response-recheck-contract` | Standard adapter-backed regional attribute-value response eligibility and restored reflection metadata contract |
| `cpp-tck.default-region-object-routing` | Ordinary and explicit regional subscriptions, default-source discovery/reflection, association replacement/restoration, and supplied-empty default-region metadata |
| `cpp-tck.default-region-object-routing-contract` | Standard adapter-backed ordinary/default-region object routing and association replacement contract |
| `cpp-tck.passive-regional-subscription` | Passive regional subscription suppression, activation-triggered discovery, and ordinary regional Update/Reflect with conveyed source-region metadata |
| `cpp-tck.auto-provide` | Adapter-supplied Auto Provide FOM, switch verification, provider-owned object discovery, and grouped `provideAttributeValueUpdate` solicitation |
| `cpp-tck.allow-relaxed-ddm` | Adapter-supplied `Allow Relaxed DDM` switch composition, touching-region admission for ordinary regional object updates and interactions, strict positive-gap suppression, and conveyed source-region metadata |
| `cpp-tck.regional-multi-attribute-update` | Adapter-supplied multi-attribute DDM FOM, independent per-attribute source regions, X-only/Y-only filtering, restoration, and conveyed source-region metadata |
| `cpp-tck.regional-three-dimensional-overlap` | Adapter-supplied three-dimensional DDM FOM, complete-overlap discovery/reflection, one-dimension-at-a-time suppression, restoration, and conveyed source-region metadata |
| `cpp-tck.attribute-scope-advisories` | Attribute In/Out Of Scope callbacks, regional source and subscription transitions, switch suppression, and stale evoked-callback handling |
| `cpp-tck.regional-declaration-relevance-advisories` | Active and passive regional object and interaction subscriptions, with standard start/stop-registration and turn-interactions-on/off advisories |
| `cpp-tck.regional-interaction-routing` | Region-qualified ordinary interaction publication/subscription, overlap routing, disjoint suppression, conveyed region designators, and declaration changes |
| `cpp-tck.regional-interaction-source-region-snapshot` | Send-time source-region capture for queued ordinary regional interactions, disjoint suppression after source mutation, restored-overlap routing, and conveyed source-region metadata |
| `cpp-tck.regional-interaction-subscription-filtering` | Ordinary receive-order regional interaction filtering, explicit empty-region no-op behavior, overlap and disjoint delivery, callback-time subscription movement, conveyed source-region metadata, and standard region failures |
| `cpp-tck.timestamped-regional-interaction` | Timestamped region-qualified interaction delivery, constrained grants, retraction, timestamp/order metadata, and conveyed region designators |
| `cpp-tck.timestamped-regional-interaction-regulation-reenable` | Queued timestamped regional interaction survives Time Regulation disable/re-enable with changed lookahead, preserving the source-region snapshot, Query Lookahead, grant ordering, metadata, and terminal retraction |
| `cpp-tck.timestamped-regional-interaction-alternate-advances` | Timestamped regional interaction delivery through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with source-region metadata, grant/query bounds, callback ordering, and backward-time failures |
| `cpp-tck.timestamped-regional-interaction-no-overlap` | Timestamped regional interaction retraction handles remain valid and terminalize correctly when the published source region has no overlapping subscriber |
| `cpp-tck.timestamped-regional-interaction-subscription-replacement` | Queued timestamped regional interaction is suppressed rather than retargeted when the receiver replaces region A with disjoint region B, then a later source-B interaction is delivered once with replacement region metadata |
| `cpp-tck.timestamped-regional-interaction-source-resignation` | A queued timestamped regional interaction remains deliverable after its producer resigns, preserving producer, source-region, payload, time, order, and retraction metadata |
| `cpp-tck.timestamped-regional-interaction-tar-nmr` | Timestamped regional interactions are delivered before the matching ordinary TAR and NMR grants, with source-region metadata and independent recipient query times |
| `cpp-tck.regional-boundaries` | Zero-dimensional and partial regions, wrong-context and foreign-region failures, and region-in-use cleanup boundaries |
| `cpp-tck.synchronization-points` | Pre-connect and pre-join synchronization-service boundaries, global and explicit-set registration, late-join announcement, invalid-member failure, duplicate-label failure, achievement, and federation synchronization completion |
| `cpp-tck.synchronization-point-contract` | Standard adapter-backed federation synchronization-point contract for global and explicit-set registration, announcement, achievement, completion, callback delivery, and lifecycle boundaries |
| `cpp-tck.callback-controls` | Callback disable/enable gating around a delivered interaction under both callback models |
| `cpp-tck.callback-controls-contract` | Standard adapter-backed callback enable and disable contract for interaction delivery, callback servicing, and lifecycle cleanup |
| `cpp-tck.asynchronous-delivery` | Asynchronous-delivery enable/disable boundaries, receive-order callback gating, `evokeCallback`, `evokeMultipleCallbacks`, and time-advance release |
| `cpp-tck.asynchronous-delivery-contract` | Standard adapter-backed asynchronous-delivery and callback-servicing contract for enable/disable, callback gating, explicit servicing, and time-advance release |
| `cpp-tck.federation-save-restore` | Pre-connect and pre-join save/restore-service boundaries, including timestamped save request, untimed federation save/restore lifecycle, status responses, completion and failure boundaries, abort, and post-restore handle rebinding |
| `cpp-tck.federation-save-restore-contract` | Standard adapter-backed untimed federation save/restore contract for admission, lifecycle, status, completion/failure, abort, restore callbacks, handle rebinding, and lifecycle boundaries |
| `cpp-tck.federation-save-restore-interlocks` | Representative declaration, object, interaction, ownership, time, DDM, synchronization, and advisory services rejected with `SaveInProgress` and `RestoreInProgress` |
| `cpp-tck.timed-federation-save-restore` | Timestamped federation save/restore, queued timestamped interaction recovery, Flush Queue delivery, and retraction |
| `cpp-tck.timed-regional-interaction-save-restore` | Timestamped save/restore of a queued regional interaction with source-region metadata and retraction state |
| `cpp-tck.timed-default-region-interaction-save-restore` | Timestamped save/restore of a queued default-region interaction, including empty source-region metadata and retraction state |
| `cpp-tck.timed-default-region-attribute-save-restore` | Timestamped save/restore of a queued default-region attribute update, including empty source-region metadata, Flush Queue delivery, and retraction state |
| `java-tck.transport-order` | Receive/timestamp order controls, default and per-instance order and transport controls, request/confirmation boundaries, invalid class/object/attribute/transport boundaries, transport queries, and delivered transport identity |
| `cpp-tck.transport-order-contract` | Standard adapter-backed ordinary order and transportation contract for default and per-instance controls, queries, invalid-handle boundaries, and delivered transport identity |
| `java-tck.relevance-advisories` | Pre-connect and pre-join support-switch accessor boundaries, advisory/support-switch state, active/passive declaration relevance, registration and interaction turn-on/turn-off callbacks, named update-rate callbacks, and active per-attribute update-rate queries |
| `cpp-tck.relevance-advisories-contract` | Standard adapter-backed advisory and support-switch contract for lifecycle accessors, declaration relevance, registration/interaction callbacks, and update-rate queries |
| `cpp-tck.delay-subscription-evaluation-interaction` | Standard Delay Subscription Evaluation switch composition, ordinary interaction retention across a late subscription, callback-boundary subscription rechecking, and suppression after unsubscribe under both callback models |
| `cpp-tck.delay-subscription-evaluation-directed-interaction` | Standard Delay Subscription Evaluation switch composition, directed interaction retention across a late target subscription, callback-boundary selector rechecking, and suppression after unsubscribe under both callback models |
| `cpp-tck.delay-subscription-evaluation-attribute-update` | Standard Delay Subscription Evaluation switch composition, known-object attribute-update retention across a late declaration, callback-boundary subscription rechecking, and suppression after unsubscribe under both callback models |
| `cpp-tck.delay-subscription-evaluation-timestamped-interaction` | Standard Delay Subscription Evaluation switch composition, timestamped interaction retention until a time-constrained grant, timestamp/order/retraction metadata, and suppression after unsubscribe at the next grant |
| `cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction` | Standard Delay Subscription Evaluation switch composition, timestamped directed-interaction retention until a time-constrained grant, target/order metadata, and suppression after unsubscribe at the next grant |
| `cpp-tck.delay-subscription-evaluation-timestamped-attribute-update` | Standard Delay Subscription Evaluation switch composition, timestamped attribute-update retention until a time-constrained grant, timestamp/order/retraction metadata, and suppression after unsubscribe at the next grant |
| `cpp-tck.update-rate-queries` | Named-rate lookup, active/passive/default subscription effects, unsubscribe reset, per-federate isolation, and invalid rate/object/attribute boundaries |
| `cpp-tck.update-rate-queries-contract` | Standard adapter-backed update-rate query contract |
| `cpp-tck.federation-teardown-isolation` | Two similarly named live executions keep independent named update-rate admission history when one execution is resigned and destroyed |
| `cpp-tck.federation-teardown-isolation-contract` | Standard adapter-backed contract for update-rate isolation across federation teardown |
| `cpp-tck.mixed-update-rate-subscriptions` | Ordinary mixed-rate attribute delivery: an active named best-effort subscription is reduced while a default-rate reliable attribute remains deliverable, with transport-aware callback aggregation |
| `cpp-tck.mixed-update-rate-subscriptions-contract` | Standard adapter-backed contract for independent named and default update-rate subscriptions |
| `cpp-tck.timestamped-attribute-update-rate-reduction` | Adapter-FOM-driven timestamped rate reduction: reliable attributes remain deliverable, an active named best-effort subscription suppresses excess passels, and suppressed retraction handles reach the standard terminal boundary |
| `cpp-tck.timestamped-attribute-update-rate-reduction-contract` | Standard adapter-backed contract for timestamped update-rate reduction and retraction |
| `cpp-tck.handle-wire-formats` | Dimension, region, and message-retraction handle encoding/decoding, direct-buffer and `VariableLengthData&` parity, encoded-length and truncated-buffer checks, copied-handle value semantics, and DDM/timestamped-service boundaries |
| `cpp-tck.handle-wire-formats-contract` | Standard adapter-backed handle encoding and decoding contract |
| `java-tck.ownership` | Pre-connect and pre-join ownership-service boundaries, ownership queries (including unowned reports), invalid object/attribute boundaries, assumption offers, negotiated and If Wanted acquisition/divestiture, If Available acquisition/unavailability, unconditional divestiture, denial, and cancellation |
| `cpp-tck.ownership-management-contract` | Standard adapter-backed ordinary attribute-ownership contract for queries, assumption offers, negotiated and If Wanted acquisition/divestiture, If Available acquisition, unconditional divestiture, denial, cancellation, callbacks, and lifecycle boundaries |
| `cpp-tck.partial-attribute-ownership-transfer` | Multi-attribute ownership acquisition and cancellation with an If Wanted transfer of only the uncanceled attribute, including release, cancellation, acquisition, tag, and ownership-state assertions |
| `cpp-tck.partial-attribute-ownership-transfer-contract` | Standard adapter-backed partial attribute ownership-transfer contract for acquisition cancellation, Divestiture If Wanted, ownership callbacks, and lifecycle cleanup |
| `cpp-tck.divestiture-if-wanted-mixed-acquirers` | Transfers independently pending attributes to mixed regular and If Available acquirers with Divestiture If Wanted; immediate mode uses regular pending acquisition to preserve the standard pending boundary |
| `cpp-tck.divestiture-if-wanted-mixed-acquirers-contract` | Standard adapter-backed mixed-acquirer Divestiture If Wanted contract with exact attribute-set transfer, callback tags, ownership state, and cleanup |
| `cpp-tck.ownership-acquisition-cancellation-transfer-race` | Candidate evoked-callback race: cancellation begins from the owner release callback, Divestiture If Wanted wins the terminal race, and the requester receives only the acquisition notification; immediate mode is explicitly inapplicable because it closes the race window |
| `cpp-tck.ownership-acquisition-cancellation-transfer-race-contract` | Candidate pure standard C++ contract for the acquisition-cancellation versus Divestiture If Wanted race; the adapter owns provider, FOM, endpoint, and callback configuration |
| `cpp-tck.negotiated-divestiture-partial-acquisition-cancellation` | Cancels one attribute of a negotiated multi-attribute divestiture, confirms only the retained attribute, and verifies exact transfer, tags, cancellation, and ownership state under both callback models |
| `cpp-tck.negotiated-divestiture-partial-acquisition-cancellation-contract` | Standard adapter-backed partial negotiated-divestiture cancellation contract for exact retained-attribute transfer, tags, callbacks, ownership state, and cleanup |
| `cpp-tck.negotiated-divestiture-cancellation` | Negotiated divestiture confirmation for a pending acquisition, cancellation back to the ordinary release path, ownership preservation, and acquisition-cancellation confirmation |
| `cpp-tck.negotiated-divestiture-cancellation-contract` | Standard adapter-backed negotiated divestiture cancellation contract |
| `cpp-tck.negotiated-divestiture-pre-delivery-cancellation` | Cancels a pending acquisition before evoked negotiated-divestiture callback delivery, suppressing stale owner callbacks and preserving ownership |
| `cpp-tck.negotiated-divestiture-pre-delivery-cancellation-contract` | Standard adapter-backed pre-delivery negotiated cancellation contract |
| `cpp-tck.negotiated-willing-to-acquire-continuation` | Evoked-callback negotiated divestiture continuation to the second Willing-to-Acquire candidate after the first candidate supersedes and cancels its request, preserving tags and ownership state; immediate delivery is an expected callback-model skip |
| `cpp-tck.negotiated-willing-to-acquire-continuation-contract` | Candidate pure standard C++ contract for negotiated willing-to-acquire continuation after cancellation of the first candidate; immediate If Available delivery remains an explicit skip |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore` | Candidate evoked-callback continuation of a regular multi-attribute ownership candidate across timed regional attribute save/restore, resignation, negotiated divestiture, retained queued delivery, and Flush Queue metadata; immediate delivery is intentionally skipped because it closes the continuation window |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore-contract` | Candidate pure standard C++ contract for the same timed regional ownership-candidate continuation; provider, FOM, endpoint, logical-time, and callback configuration remain adapter inputs, and immediate delivery is intentionally skipped |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore` | Candidate cancellation of a regular negotiated ownership transfer before confirmation callback delivery after timed regional attribute save/restore, preserving publisher ownership, canceling the surviving acquisition reservation, and retaining restored queued delivery; immediate delivery is intentionally skipped |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore-contract` | Candidate pure standard C++ contract for the same pre-delivery negotiated cancellation after timed regional restore; provider, FOM, endpoint, logical-time, and callback configuration remain adapter inputs, and immediate delivery is intentionally skipped |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore` | Candidate regular-to-regular negotiated ownership cancellation after Request Divestiture Confirmation, preserving publisher ownership, rejecting stale Confirm Divestiture, canceling the surviving acquisition reservation, and retaining the restored queued regional update; immediate delivery is intentionally skipped |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore-contract` | Candidate pure standard C++ contract for the same post-confirmation negotiated cancellation after timed regional restore; provider, FOM, endpoint, logical-time, and callback configuration remain adapter inputs, and immediate delivery is intentionally skipped |
| `cpp-tck.resign-pending-acquisition-rejection` | Rejects unconditional resignation while ownership acquisition is pending, then cancels that work during standard cancel-then-delete-then-divest resignation |
| `cpp-tck.resign-pending-acquisition-rejection-contract` | Standard adapter-backed pending-acquisition resignation rejection contract |
| `cpp-tck.resign-cancel-pending-acquisition` | Cancels pending ownership-acquisition work during standard resignation and prevents a stale owner-release callback under both callback models |
| `cpp-tck.resign-cancel-pending-acquisition-contract` | Standard adapter-backed pending-acquisition cancellation contract |
| `cpp-tck.resign-cancel-if-available-pending` | Cancels a pending If Available ownership-acquisition reservation during standard resignation and suppresses stale acquisition/unavailable delivery under both callback models |
| `cpp-tck.resign-cancel-if-available-pending-contract` | Standard adapter-backed If Available cancellation contract |
| `cpp-tck.resign-cancel-negotiated-pending` | Cancels a pending negotiated ownership transfer during standard resignation and suppresses stale owner divestiture/release callbacks under both callback models |
| `cpp-tck.resign-cancel-negotiated-pending-contract` | Standard adapter-backed negotiated cancellation contract |
| `cpp-tck.ownership-transfer-regional-update` | Three-federate regional ownership transfer, former-owner update rejection, default-source delivery after transfer, and explicit replacement update-region association |
| `java-tck.ordinary-edges` | Passive-delivery boundaries, idempotent object/interaction declarations, unsubscribed delivery suppression, and standard invalid-class/object/attribute/publication/parameter failures |
| `cpp-tck.fom-model` | Rich valid FOM hierarchy, inheritance, dimensions, update rates, transportation, advisory switches, declarations, and representative typed delivery |
| `cpp-tck.fom-model-contract` | Standard adapter-backed FOM model contract for hierarchy, inheritance, dimensions, update rates, transportation, declarations, and representative typed delivery |
| `cpp-tck.fom-empty-module-validation-contract` | Standard adapter-backed empty-module validation contract for valid and invalid FOM module boundaries |
| `cpp-tck.custom-transportation-interaction-delivery` | Adapter-declared custom transportation lookup, ordinary interaction publication/subscription/send delivery, received transportation identity, and standard transportation query reporting |
| `cpp-tck.custom-transportation-regional-attribute-delivery` | Adapter-declared custom transportation with ordinary regional attribute publication/subscription/update delivery, conveyed source-region metadata, overlap filtering, and both callback models |
| `cpp-tck.custom-transportation-regional-interaction-delivery` | Adapter-declared custom transportation with ordinary regional interaction publication/subscription/send delivery, conveyed source-region metadata, parameter delivery, and both callback models |
| `cpp-tck.custom-transportation-timestamped-delivery` | Adapter-declared custom transportation with timestamped interaction publication/subscription/send delivery, constrained grants, payload/tag/time/order/retraction metadata, and no region metadata |
| `cpp-tck.custom-transportation-timestamped-directed-delivery` | Adapter-declared custom transportation with timestamped directed-interaction publication/subscription/send delivery to a registered target, constrained grants, payload/tag/target/time/order/retraction metadata, and no region metadata |
| `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery` | Adapter-declared custom transportation with timestamped regional attribute publication/subscription/update delivery, region metadata, retraction, DDM overlap, and both callback models |
| `cpp-tck.fom-module-composition` | Create-time FOM module composition and join-time module addition with shared declaration handles |
| `cpp-tck.fom-module-composition-contract` | Standard adapter-backed FOM module-composition contract for create-time and join-time module addition with shared declaration handles |
| `cpp-tck.connection-loss-cleanup` | Adapter-triggered Connection Lost callback, fault description, survivor service, and cleanup |

The shared ordinary-service cases reuse the Java TCK scenario IDs. The
federation-list, federate-lookup, object-name-reservation, object-registration-discovery, Allow Relaxed DDM, multi-attribute and three-dimensional regional object update, timestamped interaction, timestamped regional attribute, timestamped object-management, alternate-time,
Next Message Request, Query GALT/LITS, FOM, DDM, synchronization-point,
asynchronous-delivery, save/restore, and connection-loss cases are C++ adapter
extensions. Time-dependent scenarios are skipped when the
adapter does not select a logical-time implementation; connection loss is
skipped when no adapter trigger is supplied.
The transport-change duplicate-request assertion is strict in evoked mode;
immediate mode verifies the already-committed confirmation and delivery state,
because an immediate callback may close the pending window before a second
request is made.
The standard API has no fault-injection service, so the adapter starts a
provider-managed loss fixture and passes `--connection-loss-server-managed`.
The portable executable only observes the standard `connectionLost` callback;
the Python adapter owns fixture startup, endpoint discovery, and cleanup.
The current-process adapter includes the platform-neutral
`adapters/current-process/run_connection_loss.py` harness; it starts the
adapter fixture with argument lists, runs both callback models independently,
and records combined evidence in
`.build/cpp-tck-all/connection-loss-current-process.json`. That adapter lane
passed 2/2 cases and is now the promoted adapter-required connection-loss
scenario. Other adapters must provide the same fixture contract to run it.

The ownership case uses the ordinary `DivestAcquire` attribute in the portable
FOM and runs the same transfer, denial, cancellation, ownership-query, and
user-tag assertions under both standard callback models. HLA ownership is
attribute-level; the object-management case separately covers object-instance
identity, deletion, and removal.

The promoted partial-attribute ownership case uses the adapter-supplied
two-attribute FOM with ordinary publication and subscription. It requests both
attributes, cancels exactly one request, and verifies that `Divestiture If
Wanted` transfers only the remaining attribute, preserving callback tags and
ownership state under both callback models.

The promoted ownership-transfer/update-region case uses the same adapter-supplied
DDM FOM with three federates. It verifies that a source-region association does
not follow an attribute to a new owner: the former owner is rejected, the new
owner first sends through the default source, and a later update carries a
region only after an explicit replacement association. It also checks the
standard ownership and region-designator callbacks under both callback models.

The promoted timestamped regional attribute case uses the adapter-supplied DDM
FOM and logical-time implementation with four federates. It verifies overlap
and disjoint filtering, timestamp/order and source-region metadata, retraction
before a constrained grant, time-constrained re-enable, and callback-before-grant
ordering. It remains a pure standard-API test; the adapter supplies the FOM,
dimensions, endpoint, and logical-time implementation.

The promoted timestamped regional alternate-advance case runs the same
explicit-source DDM update through Flush Queue Request, Time Advance Request
Available, and Next Message Request Available. It checks reflection-before-grant
ordering, grant and logical-time query consistency, backward-time failures, and
the delivered message-retraction boundary under both callback models.

The promoted source-association replacement case keeps a queued timestamped
update from being retargeted when its source region is unassociated and a
replacement is installed. A later update carries only the replacement region,
which makes the enqueue-time versus later-association boundary observable
through the standard callback metadata.

The promoted regional regulation re-enable case queues an overlap-qualified
timestamped update, moves the live source region out of scope, then disables and
re-enables Time Regulation at a changed lookahead. It verifies Query Lookahead,
preservation of the original source-region snapshot, reflection-before-grant
ordering, and the standard terminal retraction boundary.

The promoted regional source-resignation case queues an overlap-qualified
timestamped update, resigns its producer, and releases the recipient with an
independent time regulator. It verifies that the queued reflection retains its
object, producer, source-region, payload, timestamp/order, and retraction
metadata, while post-resignation retraction is rejected at the standard
membership boundary.

The promoted default-region interaction case sends a timestamped interaction
without a source region to a subscriber using an explicit region. It verifies
constrained delivery, supplied-empty source-region metadata, parameters, tag,
producer, timestamp/order, transport, and the terminal message-retraction
boundary.

The promoted default-region interaction alternate-advance case sends the same
default-region interaction to three constrained recipients using Flush Queue
Request, Time Advance Request Available, and Next Message Request Available. It
verifies callback-before-grant ordering, grant and logical-time query bounds,
backward-time failures, supplied-empty source-region metadata, and terminal
retraction.

The promoted default-region interaction mixed-fanout case sends one ordinary
timestamped interaction to an immediate and a time-constrained regional
recipient. It verifies supplied-empty source metadata and full interaction
metadata for the delivered copy, Request Retraction only for that recipient,
and suppression of the pending copy before its grant.

The promoted default-region interaction source-resignation case fans one
queued default-region interaction out to two constrained regional recipients,
then resigns the producer. An independent regulator releases each recipient,
and each callback preserves supplied-empty source metadata, producer, payload,
timestamp/order, and retraction identity; the resigned producer is rejected by
the standard membership boundary on Retract.

The promoted default-region interaction re-enable case queues a default-region
interaction, disables and re-enables Time Constrained, and verifies one
callback before the matching grant with supplied-empty source metadata, full
interaction metadata, and terminal retraction.

The promoted default-region interaction regulation re-enable case queues a
default-region interaction, disables and re-enables Time Regulation with
changed lookahead, verifies Query Lookahead, and proves callback-before-grant
delivery and terminal retraction.

The promoted default-region attribute alternate-advance case uses ordinary object
registration with regional subscribers. It verifies that the provider’s
default source reaches Flush Queue Request, Time Advance Request Available,
and Next Message Request Available recipients, while callbacks preserve an
explicitly supplied-empty source-region set.

The promoted default-region re-enable case queues a timestamped update before
disabling and re-enabling Time Constrained. It verifies that the update remains
eligible for regional delivery after re-enable, arrives before the subsequent
grant with supplied-empty source-region metadata, and crosses the standard
post-delivery retraction boundary.

The promoted default-region regulation re-enable case queues a timestamped
update under the initial lookahead, disables and re-enables Time Regulation at
a changed lookahead, and verifies the new value through Query Lookahead. The
queued update remains eligible for regional delivery before the recipient’s
grant, with supplied-empty source-region metadata and the standard terminal
retraction boundary.

The promoted default-region attribute mixed-fanout case sends one ordinary
timestamped update to an immediate and a time-constrained regional recipient.
It verifies supplied-empty source metadata and full reflection metadata for
the delivered copy, Request Retraction only for that recipient, and suppression
of the pending copy before its grant.

The promoted timestamped interaction regulation re-enable case applies the
same changed-lookahead boundary to `Send Interaction`/`Receive Interaction`.
It verifies the queued interaction, Query Lookahead, reflection-before-grant
ordering, parameter/tag/producer/order metadata, and terminal retraction using
only adapter-selected interaction and parameter names.

The promoted Query LITS source-resignation case queues a timestamped
interaction, verifies the active source regulator contributes both GALT and
LITS, then resigns that source and verifies that GALT becomes undefined while
LITS remains the queued message timestamp. The scenario uses only the standard
time-query, interaction, and federation-membership services.

The promoted timestamped interaction source-resignation fan-out case sends one
queued ordinary timestamped interaction to two constrained recipients, admits
one through TAR and the other through NMR, and then resigns the producer. An
independent regulator releases each recipient frontier; both callbacks preserve
the original payload, producer, timestamp/order, transport, and valid
retraction metadata, while the resigned producer is rejected at the standard
membership boundary.

The promoted single-recipient timestamped interaction source-resignation case
isolates the same lifecycle with one constrained TAR recipient. It confirms
that producer resignation does not discard the accepted passel: the callback
still carries the original parameter, tag, producer, timestamp/order, transport,
and retraction metadata before the matching grant.

The promoted timestamped interaction no-fanout case sends without any eligible
subscriber. It verifies that the accepted send still returns a valid standard
retraction handle, that immediate and repeated retraction have the required
terminal boundary, and that advancing beyond an unretracted no-fanout message
does not create receive or Request Retraction callback fan-out.

The promoted `cpp-tck.timestamped-local-delete-object` case covers the
recipient-local deletion boundary. A constrained recipient locally deletes an
object while its timestamped removal is queued; that recipient receives only
its time-advance grant, while an independent constrained recipient receives the
original removal before its grant with the standard object, tag, producer, time,
order, and retraction metadata.

The promoted `cpp-tck.timestamped-local-delete-attribute` case applies the same
recipient-local boundary to queued timestamped attribute reflection. The local
recipient suppresses its pending reflection, then re-subscribes and receives a
later update alongside the independent recipient, preserving standard object,
attribute, tag, producer, time, order, transport, and retraction metadata.

The promoted `cpp-tck.named-registration` case isolates the standard named
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

The promoted `cpp-tck.local-delete-object-instance` case isolates the ordinary
recipient-local deletion path. It verifies the pre-connect and pre-join
boundaries, ownership and pending-acquisition protections, successful local
deletion from a fresh requester, rediscovery without federation-wide removal,
and continued ordinary attribute reflection using only the adapter-owned FOM.

The promoted `cpp-tck.local-delete-object-instance-contract` runner exposes the
same local deletion path as an independently selectable pure standard C++
contract. It retains the pre-connect and pre-join boundaries, ownership and
pending-acquisition protections, fresh-requester deletion, rediscovery and
stable identity lookups, and continued ordinary reflection while taking the
provider package, FOM, endpoint, and callback configuration from the adapter.

The promoted timestamped object-deletion no-fanout case covers the corresponding
object-management boundary. An exact-lookahead deletion is terminal and cannot
be retracted; a later deletion with no eligible recipient can be retracted,
restoring object-name lookup and local attribute ownership without a Request
Retraction callback.

The promoted receive-order attribute-update callback-cancellation case keeps
the callback lifecycle portable. In evoked mode, a queued reflection is
suppressed when the recipient unsubscribes before callback servicing; in
immediate mode, the reflection is observed before the same unsubscribe. It
uses only the adapter-selected ordinary FOM and the official
`RTIambassador`/`FederateAmbassador` surface.

The promoted receive-order interaction callback-cancellation case applies the same
portable lifecycle to `SendInteraction`/`receiveInteraction`: evoked delivery
is suppressed by unsubscribe before callback servicing, while immediate
delivery is observed before unsubscribe. Parameter, tag, producer, and
transport metadata are checked through the standard callback.

The promoted order-control contract isolates prospective default and per-instance
attribute ordering plus interaction order control, timestamped delivery, and
logical-time callback boundaries through the official C++ API.

The promoted interaction-subscription lifecycle case verifies the adjacent
ordinary declaration transitions. A passive subscription suppresses an
interaction, activation enables delivery without replaying that passive
message, a downgrade back to passive suppresses later delivery, and
unsubscribe removes the declaration. It uses only adapter-selected FOM
names and the standard `RTIambassador`/`FederateAmbassador` surface.

The promoted `cpp-tck.interaction-subscription-lifecycle-contract` runner
exposes that declaration boundary as an independently selectable pure standard
C++ contract. It uses only official API headers and the standard library while
taking provider, FOM, endpoint, and callback configuration from the adapter.

The promoted timestamped directed-interaction source-resignation case applies
the same lifecycle to a target-qualified interaction. A surviving owner keeps
the target discoverable while the directed producer resigns; an independent
time regulator releases the constrained recipient, which verifies target,
parameter, tag, producer, timestamp/order, transport, and retraction metadata
before its grant.

The promoted timestamped directed-interaction TAR/NMR case sends one
target-qualified timestamped interaction at logical time 7 to two constrained
recipients. One requests TAR(7) and the other NMR(10); each callback arrives
before its own grant, and the NMR grant returns at the message time. Both
callbacks preserve target, parameter, tag, producer, timestamp/order, transport,
and retraction metadata, while logical-time queries and the standard terminal
retraction boundary are checked.

The update-rate query case is promoted after passing both callback models in
the installed-package adapter. It isolates named and default subscription
state, unsubscribe reset, per-federate isolation, and
`getUpdateRateValueForAttribute` invalid-handle boundaries.

The federation-teardown-isolation case is the next standard-only expansion of
that slice. It creates two similarly named executions, establishes an active
named update-rate history in each, destroys only the first execution, and
confirms that the surviving execution still suppresses an in-interval update.
It uses only the official API and the adapter-supplied rich FOM; it is
promoted after three repeat focused runs passed all 6/6 callback-model cases
in the installed-package lane.

The ordinary mixed-update-rate subscription case is promoted after passing both
callback models with the adapter-supplied rich FOM. It verifies that the first
ordinary update delivers both a default-rate reliable attribute and an active
named-rate best-effort attribute, while the next update retains only the
reliable attribute. The assertion accepts standard callback splitting by
transport while preserving object, tag, producer, value, and transport identity;
the focused portable artifact passed 2/2 cases and the matching native oracle
passed 37 assertions.

The timestamped attribute update-rate reduction case is promoted after passing
both callback models with an adapter-supplied rich FOM. It keeps reliable
attribute updates deliverable while an active named best-effort subscription
suppresses excess passels, and verifies the standard terminal behavior of a
suppressed message-retraction handle. The source uses only the public
`RTIambassador`/`FederateAmbassador` surface; the FOM names and provider
configuration remain adapter inputs.

The explicit-MIM creation case is promoted after passing both callback models
with the official adapter-supplied 2025 MIM. It verifies MIM object and
attribute declaration composition, cross-member handle identity, and
preservation of the caller FOM declarations through the normal federation
lifecycle.

The handle wire-format case is promoted after passing both callback models in
the installed-package adapter. It covers the remaining public dimension,
region, and message-retraction handle encoders and decoders using only the
adapter-supplied DDM and logical-time inputs.

The rich FOM case is intentionally a public-API boundary test. It loads an
adapter-supplied model and verifies root/derived object and interaction
hierarchies, inherited handles, dimensions, update-rate and transportation
  metadata, advisory switches, standard basic and constructed data-element
  encodings (including UTF-16BE element counts, alignment, signed counts,
  caller-owned opaque-storage write-through, copied/borrowed storage, type-shape,
  and custom-boundary checks), and ordinary
attribute/parameter delivery. The module-composition case verifies both
create-time composition and a join that contributes an additional module.
Neither case depends on a provider's private parser, registry, schema, or
configuration API. The model's logical-time declaration must match the
adapter-selected `--time-implementation` value.

The DDM cases use a separate adapter-supplied FOM and dimension-name list.
The FOM must expose at least two dimensions, a two-dimensional object class
with an ordinary byte attribute, and an ordinary interaction with a byte
parameter. The default fixture is `fom/ddm-tck.xml`; another provider can
replace it and pass equivalent names with `--ddm-fom` and repeated
`--ddm-dimension` options. The reusable source discovers handles and checks
metadata through the standard API; it contains no provider registry or FOM
parser dependency. The three-dimensional overlap case uses
`fom/ddm-three-dimensional-tck.xml` by default and accepts a separate
`--three-dimensional-fom`, repeated `--three-dimensional-dimension`,
`--three-dimensional-object-class`, and `--three-dimensional-attribute` set.

## Interaction survey and portability boundary

The portable interaction and object-management surface is covered in several
layers. Ordinary interactions exercise parameters, tags, producer identity,
transport identity, active/passive delivery, and the standard negative
boundaries. Timestamped ordinary interactions add logical-time factory use,
constrained grant delivery, timestamp/order metadata, and retraction callbacks.
Timestamped ordinary attribute updates add the corresponding timed
Update/Reflect path; timestamped regional attribute updates add overlap/disjoint
filtering, conveyed source-region metadata, retraction, and constrained
re-enable/grant ordering; timestamped object deletion adds timed
Delete/Remove delivery and object identity reconstitution. Directed
interactions add object-target routing, by-ownership versus universal
subscription, selective and whole-class withdrawal, and the required
`receiveDirectedInteraction` callback. Timestamped directed interactions add
the same grant and retraction lifecycle to target-qualified delivery. The
transport/order, relevance, and alternate-advance scenarios cover the
surrounding controls that change how those interactions and object updates
become observable.

The surrounding federation-control slice now also covers global and
explicit-set synchronization points, invalid explicit-member rejection,
registration and announcement callbacks, achievement and completion failure sets,
callback enable/disable gating,
asynchronous-delivery gating with both callback-servicing operations, and the
untimed save/restore lifecycle with
status responses, abort/failure paths, and post-restore federate-handle
rebinding, and timed save/restore initiation with its restored queued-delivery
boundary. These checks stay on the official public API;
they do not require a provider-managed registry, private headers, or a
process-control fixture.

The promoted `cpp-tck.timed-regional-interaction-save-restore` expansion uses the
adapter-supplied DDM FOM to save at time 6, roll back a live retraction, restore
the queued timestamped regional interaction at time 8, and deliver it through
Flush Queue at time 10 with its payload, source-region, order, and retraction
metadata. Three repeat focused runs passed all 6/6 callback-model cases before
promotion.

The promoted `cpp-tck.timed-default-region-interaction-save-restore` expansion
sends through the standard default source region, saves at time 6, restores the
queued interaction at time 8, and delivers it through Flush Queue at time 10
while checking the empty conveyed source-region designator and restored
retraction handle. Three repeat focused runs passed all 6/6 callback-model
cases before promotion.

The promoted `cpp-tck.timed-default-region-attribute-save-restore` expansion
restores a queued default-source timestamped attribute update, verifies empty
conveyed source-region metadata and Flush Queue ordering, and replays the
public retraction handle after restore. Three repeat focused runs passed all
6/6 callback-model cases before promotion.

The native-gap survey now records three semantic native-to-portable mappings:
the two regional-interaction entries and the regular-candidate continuation
lane are covered by existing pure scenarios. Three remaining native fixtures
are mixed or retained ownership continuation/cancellation races that issue an
`If Available` request while the attribute is still owned; standard HLA reports
that request unavailable rather than treating it as a pending continuation,
so they are not force-mapped into the portable TCK. The catalog now also contains the unpromoted
standard-API base case `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore`:
its focused installed-package evidence passed three independent evoked repeats;
the immediate model is an explicit expected skip because it closes the
negotiated continuation window before the first candidate resigns. The catalog
also contains the separate regular-to-regular confirmation-cancellation case
`cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore`:
it passed one evoked run and three independent evoked repeats, with an explicit
immediate-model skip for the same callback-window boundary.
The pre-delivery cancellation case
`cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore`
also passed one focused evoked run and three independent evoked repeats; its
immediate model is explicitly skipped because callback servicing closes the
pre-delivery cancellation window.

The translated slice now includes the ordinary, non-regional timestamped
object-management and alternate-advance frontier, including backward-time
failure boundaries, plus the portable DDM
region, filtering, region-use release on attribute unpublication, and
attribute-scope advisory surface, synchronization-point life cycle, callback
enable/disable controls, asynchronous callback servicing, and the standard
untimed and timestamped federation save/restore control surface. The
connection-loss cleanup is promoted as an adapter-required scenario; the
current-process Python harness is the reference fixture adapter, and the
portable runner accepts an adapter-supplied fixture path. Further MOM service-report variants,
process-restart and fault-injection routes,
durable/timed save images, advanced DDM advisories, and provider-private
diagnostics remain follow-on work. They require capabilities that are not part
of this portable P0–P6 fixture contract and are not made portable by changing
only the language binding.

The adapter-supplied P0 FOM must expose `TckObject` with `Name`, `PayRate`, and
the configured ordinary attribute (default `Value`), mark
`HLAinteractionRoot.TckInteraction` as a directed interaction of that object
class, expose the `Payload` parameter, and define the `High` update-rate
designator. Providers may supply equivalent names through the existing command
line options for the configured ordinary class/attribute/interaction/parameter
surface; the advisory fixture names are part of this small portable contract.
The test source remains unaware of provider registries or private FOM APIs.

## Build and run against a provider

The reusable target is a normal CMake project. The direct-source inputs are
the official `RTI/RTI1516.h` and related headers plus explicit provider
libraries, compile definitions, and link options. A provider adapter owns the
translation from its package to those CMake inputs; the reusable package does
not discover a named provider.

For an installed provider package, use the standard-library-only Python
orchestrator from the repository root. It drives the downstream CMake adapter,
which consumes only the explicit package prefix and disables package-registry
fallback:

```text
python tools/run_cpp_tck.py --package-prefix C:/path/to/installed-provider-package --build-directory .build/cpp-tck-installed --configuration Release --scenario-set verified --callback-model both
```

The same command runs the direct executable after the CTest gate when
`--results` or `--junit` is supplied. Repeat `--scenario` for a focused
direct-run slice and use `--skip-ctest` when the CTest gate is already recorded:

```text
python tools/run_cpp_tck.py --package-prefix C:/path/to/installed-provider-package --build-directory .build/cpp-tck-installed --scenario cpp-tck.regional-three-dimensional-overlap --skip-ctest --results .build/cpp-tck-installed/three-dimensional.json
```

Focused result files contain only the requested slice. For the strict
catalog-wide evidence check, omit `--scenario` and validate the resulting full
promoted artifact with `python tools/cpp_tck.py --results <file> --promotion
promoted`.

The Python runner uses only `subprocess` argument lists, never a shell. It
accepts `--model-fom`, `--ddm-fom`, `--multi-attribute-fom`,
`--three-dimensional-fom`, `--mim-fom`, `--switches-fom`, repeated
`--ddm-dimension`, repeated `--three-dimensional-dimension`,
`--additional-fom`, `--invalid-fom`, and `--time-implementation`. It also
passes standard `RtiConfiguration` address/settings and the optional
adapter-managed connection-loss fixture through explicit flags. A provider can
therefore substitute an equivalent portable FOM corpus, standard MIM, DDM
dimensions, endpoint, callback model, and compatible logical-time
implementation without changing the reusable test source.
The installed-package CMake adapter exposes
`HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER` and
`HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED` as adapter cache
settings; they remain disabled by default because fault injection is outside
the IEEE API.

The installed-package adapter defaults to the verified scenario set: entries
  whose catalog promotion is `promoted`. With the default `--callback-model both`,
    that is 334 scenario IDs and 668 matrix cases. Run the later adapter-required
  set only after that gate is green by configuring `--scenario-set all`; the
     complete set is 344 IDs and 688 matrix cases, including ten candidates. The
  no-fixture candidate-inclusive baseline completes 686/686 CTest cases with no
  failures and records 676 direct passes plus 12 explicit skips: ten candidate
  immediate cases and two connection-loss cases. With `--connection-loss-fixture
  <path>`, the shell-free Python adapter owns the external fault; the ordinary
  matrix remains 686/686 CTest cases and the merged direct evidence records 678
  passes plus only the ten documented candidate immediate-model skips across all
  688 callback-model cases. The
aggregate `hla_rti_cpp_tck_installed` CTest
remains available for a single full-run check.

The promoted `cpp-tck.fom-empty-module-validation` scenario checks the
standard empty-FOM rejection boundary, then creates and joins the same
federation name with the adapter-supplied FOM to prove that the rejected
request did not reserve partial state. It uses only the official C++ API.

The promoted `cpp-tck.explicit-mim-creation-contract` and
`cpp-tck.federation-mom-current-fdd-contract` runners expose the standard MIM
composition and federation-MOM current-FDD surfaces as pure C++ contracts.
Their focused four-scenario lane passed 8/8 callback-model cases; provider,
FOM/MIM, endpoint, callback, and logical-time configuration remain adapter
inputs.

The promoted `cpp-tck.custom-transportation-interaction-delivery` scenario
uses the adapter-declared rich FOM to verify custom transportation handle/name
round-trips, ordinary interaction publication/subscription/send delivery,
received transportation identity, and the standard transportation-type query
report. Its focused portable artifact passed 2/2 callback-model cases, and the
matching native oracle passed 42 assertions; the reusable scenario source uses
only the official C++ API and standard library.

The promoted `cpp-tck.custom-transportation-regional-attribute-delivery`
scenario uses the adapter-declared rich FOM and DDM dimensions to verify ordinary
regional attribute publication/subscription/update delivery, conveyed source
region metadata, overlap filtering, and custom transportation identity. Its
focused portable artifact passed 2/2 callback-model cases, and the matching
native oracle passed 51 assertions; the reusable scenario source uses only the
official C++ API and standard library.

The promoted `cpp-tck.custom-transportation-regional-interaction-delivery`
scenario uses the same adapter-declared rich FOM and DDM dimensions to verify
ordinary regional interaction publication/subscription/send delivery, parameter,
tag, producer, conveyed source-region, and custom transportation metadata. Its
focused portable artifact passed 2/2 callback-model cases, and the matching
native oracle passed 40 assertions; the reusable scenario source uses only the
official C++ API and standard library.

The promoted `cpp-tck.custom-transportation-timestamped-delivery` scenario
uses the adapter-declared rich FOM to verify timestamped interaction
publication/subscription/send delivery, constrained grant timing,
payload/tag/producer/time/order/retraction metadata, and custom transportation
identity without region metadata. Its focused portable artifact passed 2/2
callback-model cases, and the matching native oracle passed 48 assertions; the
reusable scenario source uses only the official C++ API and standard library.

The promoted `cpp-tck.custom-transportation-timestamped-directed-delivery`
scenario uses the adapter-declared rich FOM to verify timestamped directed
interaction publication/subscription/send delivery to a registered target,
constrained grant timing, payload/tag/target/producer/time/order/retraction
metadata, and custom transportation identity. Its focused portable artifact
passed 2/2 callback-model cases, and the matching native oracle passed 42
assertions; the reusable scenario source uses only the official C++ API and
standard library.

The promoted `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery`
scenario reuses the standard regional timestamped-attribute oracle with
adapter-declared object, attribute, interaction, parameter, transportation, and
DDM names. Its focused portable artifact passed 2/2 callback-model cases, and the
matching native oracle passed 60 assertions; the reusable scenario source uses
only the official C++ API and standard library.

The promoted `java-tck.api-surface-inventory` case checks that the official
C++ headers expose the ambassador, callback, logical-time, all standard handle
classes, handle sets, handle/value maps, federation/save/restore records,
authorization credentials, and byte-container types. It verifies factory
creation, configuration-builder independence, standard enum-family
distinctness, credential and authorization value semantics, copied and borrowed
storage, invalid-handle value semantics, deep independence for handle/value
maps and record/pair vectors, and standard collection/vector behavior without
provider-specific headers or FOM assumptions. It also instantiates every one
of the 109 official derived HLA exception classes and verifies its standard
message/name, copy and assignment, polymorphic-base, and stream behavior.
It also instantiates the official `NullFederateAmbassador` and invokes every
standard callback overload once, covering the callback contract without a provider
or FOM dependency.
The standard `Authorizer` and `AuthorizerFactory` extension interfaces are
exercised through local implementations and `AuthorizationResult` dispatch, without
loading a provider authorizer library.
The official `EncoderException` contract and abstract `DataElement` clone,
same-type, encoding, boundary, hash, and decode operations are also exercised
with a local standard-library-only element.
The abstract `LogicalTime`, `LogicalTimeInterval`, and `LogicalTimeFactory`
interfaces are likewise exercised through local implementations, including
boundary values, arithmetic, comparisons, difference, both encoding overloads,
both decoding overloads, diagnostics, and standard error boundaries.
The same standard-value inventory checks `VariableLengthData` empty construction,
copy independence, borrowed storage, custom-deleter adoption, and the default
array-deleter `takeDataPointer` overload without provider or FOM dependencies.
The promoted `cpp-tck.variable-length-data-contract` runner exposes that value
contract as an independently selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.logical-time-contract` runner similarly exposes the
concrete standard logical-time value and factory contract as an independently
selectable, provider- and FOM-independent slice; RTI time-role and
time-advance behavior remains in `java-tck.logical-time-factory` and
`java-tck.time-advance`.
The promoted `cpp-tck.exception-hierarchy-contract` runner exposes the complete
official C++ exception hierarchy as an independently selectable,
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
truncation contract as an independently selectable slice; its provider package,
FOM, endpoint, callback model, and logical-time implementation remain adapter
inputs, and its parity anchor is `java-tck.logical-time-factory`.
The promoted `cpp-tck.connection-callback-contract` runner exposes the standard
connection and callback-control surface as an independently selectable,
adapter-backed slice: all four `connect` overloads, unsupported callback-model
rejection, pre-connect boundaries, callback servicing, disconnect, and
reconnect; its parity anchor is `java-tck.overloads-and-exceptions`.
The promoted `cpp-tck.federation-lifecycle-contract` runner exposes the standard
federation create/join/resign/destroy, automatic-resign directive, federation and
member report, federate lookup, and missing-federation boundary surface as an
independently selectable adapter-backed slice; its parity anchor is
`java-tck.federation-membership`.
The promoted `cpp-tck.declaration-management-contract`,
`cpp-tck.object-management-contract`, `cpp-tck.attribute-interaction-contract`,
and `cpp-tck.directed-interaction-contract` runners expose the standard P0
declaration, ordinary object, ordinary delivery, and directed-interaction
surfaces as independently selectable adapter-backed slices, each retaining its
Java parity anchor.
The promoted `cpp-tck.ownership-management-contract`,
`cpp-tck.synchronization-point-contract`, and
`cpp-tck.federation-save-restore-contract` runners add independently selectable
standard ownership, synchronization-point, and save/restore slices; they use
only the official C++ API and standard library while taking provider, FOM,
endpoint, callback, and logical-time configuration from the adapter.
The promoted `cpp-tck.asynchronous-delivery-contract`,
`cpp-tck.timestamped-attribute-update-contract`,
`cpp-tck.timestamped-object-deletion-contract`,
`cpp-tck.alternate-time-advances-contract`, and
`cpp-tck.timestamped-interactions-contract`,
`cpp-tck.timestamped-interaction-source-resignation-contract`,
`cpp-tck.timestamped-interaction-source-resignation-fanout-contract`,
`cpp-tck.timestamped-directed-interactions-contract`, and
`cpp-tck.timestamped-directed-interaction-source-resignation-contract`,
`cpp-tck.timestamped-directed-interaction-source-resignation-fanout-contract`,
`cpp-tck.timestamped-directed-alternate-advances-contract` runners extend that
same boundary to callback servicing, timestamped ordinary attribute and
interaction delivery, source-resignation queue retention, recipient fan-out,
directed target routing, directed source-resignation retention, per-recipient
fan-out, retraction, and the standard alternate time-advance paths. The promoted
`cpp-tck.timestamped-interaction-mixed-advances-contract`,
`cpp-tck.timestamped-interaction-flush-queue-future-input-contract`, and
`cpp-tck.timestamped-interaction-tso-designator-terminalization-contract`
runners expose the mixed-advance, future-input Flush Queue, and terminal
retraction boundaries as independently selectable pure standard C++ contracts.
They take provider, FOM, endpoint, callback, and logical-time configuration from
the adapter.
The promoted `cpp-tck.timestamped-interaction-cross-producer-order-contract`,
`cpp-tck.timestamped-interaction-no-fanout-contract`, and
`cpp-tck.timestamped-interaction-retraction-fanout-contract` runners add
timestamp ordering, no-fan-out terminalization, and delivered-versus-queued
retraction fan-out boundaries on the same pure standard API surface. The
promoted `cpp-tck.timestamped-attribute-order-cohort-contract`,
`cpp-tck.timestamped-attribute-update-queued-passel-retraction-contract`, and
`cpp-tck.timestamped-attribute-update-no-fanout-contract` runners add
timestamped attribute ordering, queued passel retraction, and no-recipient
terminalization on the same pure standard API surface. The
promoted `cpp-tck.timestamped-attribute-update-alternate-advances-contract`,
`cpp-tck.timestamped-attribute-update-flush-queue-future-input-contract`, and
`cpp-tck.timestamped-attribute-update-reenable-contract` runners add alternate
advance servicing, future-input Flush Queue behavior, and Time Constrained
re-enable boundaries on the same pure standard API surface. The
promoted `cpp-tck.timestamped-attribute-source-resignation-contract`,
`cpp-tck.timestamped-attribute-source-resignation-fanout-contract`, and
`cpp-tck.timestamped-attribute-update-regulation-reenable-contract` runners add
post-resignation ownership and queued delivery, per-recipient timestamped
attribute fan-out, and Time Regulation re-enable with changed lookahead on the
same pure standard API surface. They use only the official C++ API and standard
library while taking provider, FOM, endpoint, callback, and logical-time
configuration from the adapter. The
promoted `cpp-tck.timestamped-object-deletion-no-fanout-contract`,
`cpp-tck.timestamped-object-deletion-tombstone-contract`, and
`cpp-tck.timestamped-object-deletion-regulation-reenable-contract` runners
extend the same pure boundary to no-recipient deletion retraction and
ownership/name restoration, terminal deletion tombstones and named
re-registration, and Time Regulation re-enable with changed lookahead. They
use only the official C++ API and standard library while taking provider, FOM,
endpoint, callback, and logical-time configuration from the adapter. The
promoted `cpp-tck.timestamped-object-deletion-source-resignation-fanout-contract`,
`cpp-tck.timestamped-object-deletion-retraction-joined-owners-contract`, and
`cpp-tck.timestamped-object-deletion-mixed-advances-contract` runners add
independent post-resignation fan-out, joined-owner retraction cleanup, and
Flush Queue/TAR-available/NMR-available delivery boundaries on the same pure
standard surface. The
promoted `cpp-tck.timestamped-interaction-regulation-reenable-contract`,
`cpp-tck.timestamped-interaction-reenable-contract`,
`cpp-tck.timestamped-directed-interaction-reenable-contract`, and
`cpp-tck.timestamped-directed-interaction-regulation-reenable-contract` runners
add ordinary and directed timestamped Time Constrained/Time Regulation
re-enable boundaries with the same adapter-owned configuration. The
promoted `cpp-tck.support-services-contract`,
`cpp-tck.transport-order-contract`, and
`cpp-tck.relevance-advisories-contract` runners similarly expose the standard
support lookup, ordinary order/transport, and advisory-switch surfaces as
independently selectable pure C++ slices.
The promoted `cpp-tck.fom-model-contract`,
`cpp-tck.fom-module-composition-contract`, and
`cpp-tck.fom-empty-module-validation-contract` runners similarly expose the
standard FOM model, module-composition, and empty-module validation surfaces as
independently selectable pure C++ slices.
The promoted `cpp-tck.service-report-interaction-contract`,
`cpp-tck.service-report-attribute-update-contract`,
`cpp-tck.service-report-register-object-instance-contract`, and
`cpp-tck.service-report-delete-object-instance-contract` runners expose the
ordinary MOM service-report success routes as independently selectable pure C++
slices using adapter-supplied standard MIM/FOM inputs.
The promoted `cpp-tck.null-federate-ambassador-contract` runner exposes the
official C++ `NullFederateAmbassador` callback and overload contract as an
independently selectable, provider- and FOM-independent slice; its parity
anchor is `java-tck.api-surface-inventory`.
The promoted `cpp-tck.data-element-contract` runner exposes the official
C++ `DataElement` base clone, type, encoding, boundary, hash, and decode
contract as an independently selectable, provider- and FOM-independent
slice; its parity anchor is `java-tck.api-surface-inventory`.
The promoted `cpp-tck.basic-data-elements-contract` runner exposes the scalar
basic-data-element wire and validation contract as an independently selectable,
provider- and FOM-independent slice; composite record, array, and variant-record
coverage is separately exposed by `cpp-tck.composite-data-elements-contract`
and retained in `java-tck.encoder-round-trip` for cross-language parity.
The promoted `cpp-tck.composite-data-elements-contract` runner exposes the
composite array, record, and variant-record wire and validation contract as an
independently selectable, provider- and FOM-independent slice.

The promoted `cpp-tck.federation-mom-save-conditionals` scenario observes the
standard `HLAfederation` MOM object's `HLAnextSaveName/Time` and
`HLAlastSaveName/Time` conditionals. It verifies empty initial values, a
timestamped pending save, reliable RTI-originated reflection metadata, clearing
of the next values at save admission, and publication of the last values after
successful completion. It uses only the adapter-supplied standard MIM/FOM and
logical-time implementation with the official
`RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.joined-federate-mom-federate-state-save-restore` scenario
observes the standard `HLAfederateState` MOM attribute from a joined federate
across save initiation, save completion, restore initiation, and restore
completion. It verifies the state sequence `1 -> 3 -> 1 -> 5 -> 1`, conditional
initial-state reflection, suppression of the saving federate's own state
reflection, standard save/restore callbacks, and reliable RTI-generated
metadata in both callback models using only the standard MIM and official IEEE
C++ API.

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
`HLAinteractionCounts` decoding, sender metadata, and the empty response for an
idle joined federate in both callback models.

The promoted `cpp-tck.joined-federate-mom-reflections-received-counts` scenario
delivers one adapter-supplied attribute reflection reliably and two after a
standard best-effort transport change. It requests
`HLArequestReflectionsReceived` and verifies the two
`HLAreportReflectionsReceived` transport buckets, nested standard
`HLAobjectClassBasedCounts` decoding, receiver metadata, and the empty response
for a requester with no received reflections in both callback models.

The promoted `cpp-tck.joined-federate-mom-reflection-counts` scenario observes
the standard joined-federate MOM `HLAobjectInstancesReflected` and
`HLAreflectionsReceived` counters through direct attribute-value requests and
periodic `HLAsetTiming` reflections. It distinguishes repeated reflections of
one object from first reflections of another, includes a timestamped reflection,
and verifies both ordinary and timestamped callback paths using only the
adapter-supplied FOM, standard MIM, logical-time implementation, and official
IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-received-counts`
scenario sends one ordinary and one directed interaction of the same
adapter-supplied class, proves that ordinary receipt is excluded from
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

The promoted `cpp-tck.timestamped-attribute-update-no-fanout` scenario proves
that a time-regulating producer receives a valid retraction handle for a
timestamped update with no eligible recipient, can legally retract it once,
and receives the standard `MessageCanNoLongerBeRetracted` boundary on reuse.
It also verifies that no local reflection or retraction callback is generated.
The scenario uses only the adapter-supplied ordinary FOM, logical-time
implementation, and official IEEE C++ API.

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
cohort before their grants, followed by the timestamp-7 update. It allows the
standard's unspecified order within the equal-timestamp cohort while checking
exact value, tag, producer, time/order, transportation, and retraction
metadata through only the adapter-supplied ordinary FOM, logical-time
implementation, and official `RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.timestamped-attribute-update-queued-passel-retraction`
scenario submits two adapter-defined timestamped attribute values, retracts the
first queued update before its grant, and verifies complete value coverage,
callback-before-grant ordering, and exact tag, time, producer, order,
transportation, and retraction metadata for the later delivery. It uses only the
adapter-supplied multi-attribute FOM, logical-time implementation, and official
`RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.timestamped-object-deletion-retraction-joined-owners`
scenario transfers one attribute to a joined member, delivers a timestamped
object removal to that member, and then has it resign before the producer
retracts the deletion. It verifies that the departed member receives no
retraction callback, joined members recover the object identity, and the
departed member's former attribute remains unowned. The scenario uses only the
adapter-supplied multi-attribute FOM, logical-time implementation, and official
IEEE C++ API.

The promoted `cpp-tck.timestamped-object-deletion-no-fanout-contract` runner
checks the exact lookahead boundary, no-recipient deletion retraction, object
name/identity restoration, local attribute-ownership restoration, and the
absence of retraction fan-out. The promoted
`cpp-tck.timestamped-object-deletion-tombstone-contract` runner checks the
terminal deletion tombstone, `MessageCanNoLongerBeRetracted`, and named
registration reuse. The promoted
`cpp-tck.timestamped-object-deletion-regulation-reenable-contract` runner
checks changed Query Lookahead, removal-before-grant ordering, removal
metadata, object identity cleanup, and terminal retraction after Time
Regulation re-enable. All three runners use only the adapter-supplied FOM and
logical-time implementation plus the official IEEE C++ API.

The promoted `cpp-tck.service-report-interaction` scenario uses only the
official `RTIambassador`/`FederateAmbassador` API and standard encoder types to
subscribe to `HLAreportServiceInvocation` from an adapter-supplied standard
MIM. It verifies the successful ordinary `SendInteraction` report in both
callback models; provider-specific MOM headers and registry surfaces are not
required. The adjacent promoted `cpp-tck.service-report-attribute-update`
scenario verifies the same standard report contract for an ordinary
`UpdateAttributeValues` service.
The promoted `cpp-tck.service-report-timestamped-interaction` scenario verifies
the successful standard MOM report for a timestamped `SendInteraction` call and
independently checks constrained delivery, logical-time grant ordering,
payload/tag/time/order/transport metadata, and the usable retraction handle
using only the adapter-supplied MIM, ordinary FOM, and logical-time
implementation.
The promoted `cpp-tck.service-report-attribute-update-failure` scenario drives
unknown-object and undefined-attribute inputs through the same standard API. It
verifies typed failure reports, exception names, serial progression, and
suppression of ordinary attribute reflection in both callback models.
The promoted `cpp-tck.service-report-timestamped-attribute-update-failure`
scenario extends that matrix through the standard timestamped
`UpdateAttributeValues` overload. It verifies the optional timestamp report
argument, invalid-logical-time lookahead rejection, serial progression, and
suppression of ordinary attribute reflection in both callback models.
The promoted `cpp-tck.service-report-timestamped-interaction-failure` scenario
extends the same report contract through timestamped `SendInteraction`. It
verifies invalid interaction-class, parameter, and logical-time failures, the
optional timestamp report argument, serial progression, and suppression of the
ordinary application callback in both callback models.
The promoted `cpp-tck.service-report-timestamped-delete-object-instance-failure`
scenario applies the same standard MOM contract to timestamped
`DeleteObjectInstance`. It verifies unknown-object and invalid-logical-time
failures, the optional timestamp report argument, serial progression, and
suppression of ordinary removal callbacks in both callback models.
The promoted `cpp-tck.service-report-interaction-failure` scenario drives
invalid interaction-class, parameter, and unpublished-class inputs through the
same standard API. It verifies typed failure reports, exception names, serial
progression, and suppression of the ordinary application callback in both
callback models.
The promoted `cpp-tck.service-report-regional-interaction` scenario extends
that public MOM interaction route to standard `SendInteractionWithRegions`.
It uses only adapter-supplied DDM dimensions/FOM and MIM, verifies the
successful regional service report in both callback models, and independently
checks overlap-qualified application delivery, payload/tag/producer metadata,
and the conveyed source `RegionHandleSet`.
The promoted `cpp-tck.service-report-regional-interaction-subscription`
scenario applies the same standard MOM route to
`SubscribeInteractionClassWithRegions` and
`UnsubscribeInteractionClassWithRegions`. It verifies the service type,
passive-subscription indicator, exact typed association arguments for active
and passive declarations, null returned argument, exception, serial
progression, and the absence of an application callback in both callback
models.
The promoted `cpp-tck.service-report-regional-interaction-contract` and
`cpp-tck.service-report-regional-interaction-subscription-contract` runners
expose those successful regional service-report routes as independently
selectable pure standard C++ contracts. The focused four-scenario lane passed
8/8 callback-model cases; provider, MIM, DDM FOM, dimensions, endpoint, and
callback configuration remain adapter inputs.
The promoted `cpp-tck.federation-mom-save-conditionals-contract` and
`cpp-tck.joined-federate-mom-federate-state-save-restore-contract` runners
likewise expose the save/restore MOM routes as independently selectable pure
standard C++ contracts. Their focused four-scenario lane passed 8/8
callback-model cases; the catalog-wide aggregate passed 666/666 CTest cases
plus 666 direct passes with only the two expected connection-loss skips.
The ownership candidate contract-twin lane recorded 4 passes and 4 explicit
immediate-model skips across the two candidate scenarios; both remain
`promotion=candidate` until an all-model verification fixture is available.
The timed regular-candidate contract twin recorded 2 evoked passes and 2
explicit immediate-model skips in its focused four-case lane; it also remains
`promotion=candidate` pending broader adapter coverage.
The timed pre-delivery cancellation contract twin recorded 2 evoked passes and
2 explicit immediate-model skips in its focused four-case lane; it also remains
`promotion=candidate` pending broader adapter coverage.
The timed confirmation-cancellation contract twin recorded 2 evoked passes and
2 explicit immediate-model skips in its focused four-case lane; it also remains
`promotion=candidate` pending broader adapter coverage.
The promoted `cpp-tck.service-report-regional-interaction-failure` scenario
drives invalid interaction-class, parameter, and region inputs through the same
standard API. It verifies typed failure reports, exception names, serial
progression, and suppression of the regional application callback in both
callback models.
The promoted `cpp-tck.service-report-request-attribute-value-update` scenario
verifies the same standard report contract for a successful
`RequestAttributeValueUpdate` call and independently checks the standard
`provideAttributeValueUpdate` callback, using only the adapter-supplied MIM and
ordinary FOM. It exercises both standard overloads—the known-object overload
and the object-class overload—then checks independent MOM reports, serial
progression, and preservation of the object/class, attribute set, and request
tag in each provider callback.
The promoted `cpp-tck.service-report-reserve-object-instance-name` scenario
verifies the standard report contract for a successful
`ReserveObjectInstanceName` invocation and independently checks the standard
`objectInstanceNameReservationSucceeded` callback, using only the
adapter-supplied MIM and ordinary FOM.
The promoted `cpp-tck.service-report-release-object-instance-name` scenario
verifies the same standard report contract for a successful
`ReleaseObjectInstanceName` invocation after a standard reservation-success
callback, again using only the adapter-supplied MIM and ordinary FOM.
The promoted `cpp-tck.service-report-release-multiple-object-instance-names`
scenario verifies the same standard report contract for a successful
`ReleaseMultipleObjectInstanceNames` invocation after a standard
multiple-reservation-success callback, using only the adapter-supplied MIM and
ordinary FOM.
The promoted `cpp-tck.service-report-register-object-instance` scenario verifies
the same standard report contract for a successful ordinary
`RegisterObjectInstance` call and independently checks the matching discovery
callback, using only the adapter-supplied standard MIM and ordinary FOM.
The timestamped companion verifies that report delivery remains receive-order
while the ordinary interaction follows timestamped delivery, retraction, and
grant ordering.

The promoted `cpp-tck.service-report-local-delete-object-instance` scenario
verifies the standard MOM report for a successful `localDeleteObjectInstance`
call. It checks the seven standard report parameters, service identity and
type, reliable transport, success and empty-exception values, serial number,
and callback metadata using only the adapter-supplied standard MIM and the
official API.

The promoted `cpp-tck.service-report-delete-object-instance` scenario applies
the same standard MOM callback contract to a successful ordinary
`deleteObjectInstance` call and independently verifies the normal removal
callback, using only the adapter-supplied MIM and ordinary FOM.

The promoted `cpp-tck.service-report-delete-object-instance-failure` scenario
extends that contract to invalid and stale ordinary deletion calls. It verifies
the standard failure indicator, `ObjectInstanceNotKnown` exception text,
returned-argument encoding, serial progression around the successful deletion,
and the normal removal callback, using only the adapter-supplied MIM and
ordinary FOM.

The promoted `cpp-tck.service-report-local-delete-object-instance-failure`
scenario applies the same standard MOM failure contract to
`localDeleteObjectInstance`. It verifies unknown and stale handle failures
around a successful local deletion, the standard supplied and returned
argument encodings, exception text, serial progression, and ordinary object
discovery using only the adapter-supplied MIM and FOM.

The promoted `cpp-tck.service-report-interaction-failure-contract`,
`cpp-tck.service-report-attribute-update-failure-contract`, and
`cpp-tck.service-report-regional-interaction-failure-contract` runners expose
the ordinary MOM failure routes as independently selectable pure standard C++
contracts. They retain the standard typed report, exception, serial, and
application-callback assertions while taking MIM, FOM, DDM, endpoint, and
callback configuration from the adapter.

The timestamped failure-contract runners extend the same pure boundary to
timestamped `UpdateAttributeValues`, `SendInteraction`, and
`DeleteObjectInstance`. They take the logical-time implementation from the
adapter and preserve optional timestamp, exception, serial, and callback-
suppression assertions without adding provider-specific dependencies.

The promoted `cpp-tck.service-report-timestamped-interaction-contract` runner
exposes the successful timestamped interaction MOM route as an independently
selectable pure standard C++ contract. It retains the typed report,
timestamped-delivery, logical-time-grant, payload, retraction, and callback
assertions while taking MIM, FOM, endpoint, callback, and logical-time
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

The promoted `cpp-tck.attribute-value-update-request-baseline` scenario isolates
the ordinary object-instance Request Attribute Value Update overload. It verifies
that a known registered object solicits exactly the current owner’s attributes,
preserves the request tag, and does not echo requester-owned or unowned attributes.
The promoted `cpp-tck.object-class-attribute-value-update-request-baseline`
scenario separately expands the class overload across two concrete subclass
instances, verifies one callback per current owner with inherited-attribute
filtering, and checks requester-owned suppression. Both scenarios use only the
official API and adapter-supplied ordinary/rich FOM inputs.

The promoted `cpp-tck.attribute-value-update-response` scenario completes the
ordinary attribute-value request/response route. It verifies provider callback
object, attribute-set, and request-tag metadata, confirms that no reflection
arrives before the provider responds, and checks the returned value, response
tag, reliable transport, producer identity, and empty region metadata through
the official API and adapter-supplied ordinary FOM.

The corresponding `cpp-tck.attribute-value-update-request-baseline-contract`,
`cpp-tck.object-class-attribute-value-update-request-baseline-contract`, and
`cpp-tck.attribute-value-update-response-contract` runners expose those three
ordinary request/response boundaries as independently selectable pure standard
C++ contracts while taking provider, FOM, endpoint, and callback configuration
from the adapter.

The promoted `cpp-tck.regional-attribute-value-update-response-recheck` scenario
extends that route through standard DDM. It requests a response while the
subscriber overlaps the registered object, moves the committed subscriber
region out of overlap before the provider responds, and verifies that the
response is suppressed. After restoring overlap, a fresh request proves the
value, tag, reliable transport, and producer metadata through only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied DDM FOM.

The promoted `cpp-tck.object-attribute-subscription-lifecycle` scenario
verifies the same passive/active/downgrade/unsubscribe lifecycle for ordinary
object attributes: passive declarations suppress discovery and reflection,
activation discovers the existing object, downgrade suppresses later updates,
reactivation restores reflection, and unsubscribe removes delivery. It uses
only the adapter-supplied ordinary FOM and standard object callbacks.

The promoted `cpp-tck.object-publication-registration-fence` scenario verifies
that whole-class `unpublishObjectClass` removes the standard registration
fence: registration fails with `ObjectClassNotPublished`, republishing restores
registration, and the subscriber receives discovery with stable object/class/
name lookups. It uses only the adapter-supplied ordinary FOM and standard
object-management callbacks.

The promoted `cpp-tck.object-publication-registration-fence-contract` runner
exposes that same publication and registration boundary as an independently
selectable pure standard C++ contract. It uses only official API headers and
the standard library while taking provider, FOM, endpoint, and callback
configuration from the adapter.

The promoted `cpp-tck.interaction-publication-send-fence` scenario verifies
that whole-class `unpublishInteractionClass` fences `sendInteraction` with
`InteractionClassNotPublished`, while republication restores ordinary parameter
delivery, producer identity, tag, and transportation metadata. It uses only the
adapter-supplied ordinary FOM and standard interaction callbacks.

The promoted `cpp-tck.interaction-publication-send-fence-contract` runner
exposes that publication boundary as an independently selectable pure standard
C++ contract. It uses only official API headers and the standard library while
taking provider, FOM, endpoint, and callback configuration from the adapter.

The promoted `cpp-tck.directed-interaction-publication-send-fence` scenario
verifies the corresponding directed boundary. Whole-class directed
unpublication makes `sendDirectedInteraction` fail with
`InteractionClassNotPublished`; republication restores targeted parameter
delivery, target identity, producer identity, tag, and transportation
metadata. It uses only the adapter-supplied ordinary FOM and standard
directed-interaction/object-management callbacks.

The promoted `cpp-tck.directed-interaction-publication-send-fence-contract`
runner exposes that targeted publication boundary as an independently
selectable pure standard C++ contract. It uses only official API headers and
the standard library while taking provider, FOM, endpoint, and callback
configuration from the adapter.

The promoted `cpp-tck.directed-interaction-target-lifecycle` scenario verifies
targeted delivery across universal subscription, callback-model subscription
removal and restoration, source publication removal and restoration, and target
deletion/removal cleanup. The promoted
`cpp-tck.directed-interaction-target-lifecycle-contract` runner exposes that
same target lifecycle as an independently selectable pure standard C++ contract
with provider, FOM, endpoint, and callback configuration supplied by the
adapter.

The promoted `cpp-tck.directed-interaction-subscription-kind-contract`
runner exposes the by-ownership and universal directed-interaction
subscription boundary as an independently selectable pure standard C++
contract. It uses only official API headers and the standard library while
taking provider, FOM, endpoint, and callback configuration from the adapter.

The adapter also registers each selected catalog scenario as a separate CTest
for each selected callback model. The Python runner invokes that matrix with
`ctest -L \"^portable-cpp-tck$\"`; the scenario-set choice belongs to the
adapter, while the portable executable and source do not know how a provider
was selected.

The current Java parity audit covers 17 Java scenario IDs directly and maps
`java-tck.synchronization` to the promoted synchronization-point extension.
The Java catalog still marks DDM, MOM, and save/restore as unsupported, so
those three IDs remain out of the cross-language run set until their Java
capabilities are defined.

In the catalog, `promotion=promoted` is the verified baseline gate;
`default_status` separately describes availability: `run` needs no optional
adapter capability, `adapter-required` is a later extension, and `unsupported`
is never added to the matrix. This keeps promotion order visible without
changing the reusable test source.

The JSON and JUnit outputs are suitable for CI artifact collection. Skipped
time-management or connection-loss cases are explicit in both formats and do
not masquerade as passes.
The same case also instantiates every one of the 109 official derived HLA
exception classes and verifies its standard message/name, copy and assignment,
polymorphic-base, and stream behavior.
It also instantiates the official `NullFederateAmbassador` and invokes every
standard callback overload once, covering the callback contract without a provider
or FOM dependency.
The standard `Authorizer` and `AuthorizerFactory` extension interfaces are
exercised through local implementations and `AuthorizationResult` dispatch, without
loading a provider authorizer library.
The official `EncoderException` contract and abstract `DataElement` clone,
same-type, encoding, boundary, hash, and decode operations are also exercised
with a local standard-library-only element.
The abstract `LogicalTime`, `LogicalTimeInterval`, and `LogicalTimeFactory`
interfaces are likewise exercised through local implementations, including
boundary values, arithmetic, comparisons, difference, both encoding overloads,
both decoding overloads, diagnostics, and standard error boundaries.
The same standard-value inventory checks `VariableLengthData` empty construction,
copy independence, borrowed storage, custom-deleter adoption, and the default
array-deleter `takeDataPointer` overload without provider or FOM dependencies.
The promoted `cpp-tck.variable-length-data-contract` runner exposes that value
contract as an independently selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.logical-time-contract` runner similarly exposes the
concrete standard logical-time value and factory contract as an independently
selectable, provider- and FOM-independent slice; RTI time-role and
time-advance behavior remains in `java-tck.logical-time-factory` and
`java-tck.time-advance`.
The promoted `cpp-tck.exception-hierarchy-contract` runner exposes the complete
official C++ exception hierarchy as an independently selectable,
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
The promoted `cpp-tck.federation-lifecycle-contract` runner exposes the standard
federation create/join/resign/destroy, automatic-resign directive, federation and
member report, federate lookup, and missing-federation boundary surface as an
independently selectable adapter-backed slice.
The promoted `cpp-tck.declaration-management-contract`,
`cpp-tck.object-management-contract`, `cpp-tck.attribute-interaction-contract`,
and `cpp-tck.directed-interaction-contract` runners expose the standard P0
declaration, ordinary object, ordinary delivery, and directed-interaction
surfaces as independently selectable adapter-backed slices.
The promoted `cpp-tck.null-federate-ambassador-contract` runner exposes the
official C++ `NullFederateAmbassador` callback and overload contract as an
independently selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.data-element-contract` runner exposes the official
C++ `DataElement` base clone, type, encoding, boundary, hash, and decode
contract as an independently selectable, provider- and FOM-independent
slice.
The promoted `cpp-tck.basic-data-elements-contract` runner exposes the scalar
basic-data-element wire and validation contract as an independently selectable,
provider- and FOM-independent slice; composite record, array, and variant-record
coverage is separately exposed by `cpp-tck.composite-data-elements-contract`
and retained in `java-tck.encoder-round-trip` for cross-language parity.
The promoted `cpp-tck.composite-data-elements-contract` runner exposes the
composite array, record, and variant-record wire and validation contract as an
independently selectable, provider- and FOM-independent slice.
