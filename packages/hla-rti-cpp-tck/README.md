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
| `cpp-tck.standard-exception-boundaries` | Adapter-backed standard service-level exception outcomes: invalid resign actions and lookahead, joined-member disconnect, ownership cancellation and confirmation boundaries, delete privilege, and duplicate named registration |
| `cpp-tck.standard-exception-boundaries-contract` | Pure standard C++ contract twin for deterministic service-level exception boundaries, using only adapter-supplied provider, baseline FOM, endpoint, logical-time, and callback configuration |
| `cpp-tck.enum-contract` | Provider- and FOM-independent official C++ enumeration contract: distinct standard values for settings, callback, order, resign, save, restore, service-group, synchronization, and authorization-result families |
| `cpp-tck.handle-and-collection-contract` | Provider- and FOM-independent official C++ handle and collection contract: invalid-handle identity, hashing/order, handle sets, handle-value maps, `RangeBounds`, region-pair vectors, and federation/restore record vectors |
| `cpp-tck.configuration-and-authorization-contract` | Provider- and FOM-independent official C++ configuration, federation-record, credential, authorization, and local `Authorizer`/`AuthorizerFactory` contract: builder/accessor behavior, copy/assignment independence, ASCII and BMP Unicode plaintext-password encoding/decoding, authorization results, and polymorphic dispatch |
| `cpp-tck.authorizer-factory-factory-contract` | Provider- and FOM-independent official C++ `HLAauthorizerFactoryFactory` contract: standard-authorizer selection, factory and authorizer naming/creation, and unsupported-name rejection |
| `cpp-tck.runtime-identity-contract` | Provider-neutral official C++ `rtiName()`/`rtiVersion()` contract: callable, non-empty, process-stable runtime identity without asserting vendor-specific strings |
| `cpp-tck.rti-ambassador-factory-contract` | Provider- and FOM-independent official C++ `RTIambassadorFactory` construction contract: repeatable creation of usable standard `RTIambassador` objects without provider, endpoint, or FOM assumptions |
| `cpp-tck.null-federate-ambassador-contract` | Provider- and FOM-independent official C++ `NullFederateAmbassador` callback contract: every standard no-op callback family and each ordinary/timestamped callback overload used by the portable TCK |
| `cpp-tck.data-element-contract` | Provider- and FOM-independent official C++ `DataElement` base contract: clone/type identity, encoded length and boundary, hashing, complete append encoding, direct decode, and offset decode |
| `cpp-tck.basic-data-elements-contract` | Provider- and FOM-independent scalar `BasicDataElements` contract: exact standard wire encodings and decode/validation boundaries for integer, Boolean, octet/byte, floating-point, ASCII/Unicode, opaque-data, and octet-pair types |
| `cpp-tck.composite-data-elements-contract` | Provider- and FOM-independent composite `DataElement` contract: fixed/variable arrays, aligned fixed records, nested record arrays, mapped and unknown variant alternatives, extendable variants, typed forms, and malformed padding/length boundaries |
| `cpp-tck.connection-callback-contract` | Standard adapter-backed connection and callback-control contract: all four `connect` overloads, callback-model rejection, pre-connect boundaries, duplicate-connect handling, callback enable/disable and servicing, disconnect, and reconnect |
| `cpp-tck.connection-service-boundaries` | Standard connect overloads, callback-model validation, pre-connect service boundaries, callback controls, disconnect, and reconnect |
| `cpp-tck.connection-service-boundaries-contract` | Pure standard C++ contract for connection and callback-service boundaries |
| `cpp-tck.federation-lifecycle-contract` | Standard adapter-backed federation lifecycle contract: create/join/resign/destroy, automatic-resign directives, federation/member reports, federate handle lookups, duplicate-membership failures, and missing-federation boundaries |
| `cpp-tck.duplicate-federate-name-join-boundary` | Named join rejects a name already used by an active member, then allows that federate to retry with a distinct name; shares the `java-tck.federation-membership` parity anchor |
| `cpp-tck.duplicate-federate-name-join-boundary-contract` | Standard contract twin for duplicate federate-name rejection and distinct-name retry |
| `cpp-tck.repeated-join-by-member-boundary` | Repeating the named join through an already joined ambassador returns `FederateAlreadyExecutionMember`; the original member can still resign; shares the Java federation-membership parity anchor |
| `cpp-tck.repeated-join-by-member-boundary-contract` | Standard contract twin for the repeated membership join boundary |
| `cpp-tck.foreign-federate-name-lookup-boundary` | Each federate name resolves within its own execution; querying a name from a different execution returns `NameNotFound` (§10.2) |
| `cpp-tck.foreign-federate-name-lookup-boundary-contract` | Standard contract twin for the foreign federate-name lookup boundary |
| `cpp-tck.foreign-object-instance-name-lookup-boundary` | Each named object resolves in its own execution; querying the other execution's object name returns `ObjectInstanceNotKnown` (§10.7) |
| `cpp-tck.foreign-object-instance-name-lookup-boundary-contract` | Standard contract twin for the foreign object-instance name lookup boundary |
| `cpp-tck.foreign-object-instance-handle-lookup-boundary` | A handle known in another execution is rejected by `getObjectInstanceName` with `ObjectInstanceNotKnown` (§10.8), with unequal opaque values used to avoid cross-execution alias ambiguity |
| `cpp-tck.foreign-object-instance-handle-lookup-boundary-contract` | Standard contract twin for the foreign object-instance handle lookup boundary |
| `cpp-tck.local-delete-object-instance-name-lookup-boundary` | A subscriber's handle-to-name lookup returns `ObjectInstanceNotKnown` after local deletion, while the publisher still resolves the object (§6.18, §10.8) |
| `cpp-tck.local-delete-object-instance-name-lookup-boundary-contract` | Standard contract twin for handle-to-name lookup after local object forgetting |
| `cpp-tck.remote-delete-known-object-class-lookup-boundary` | The deleting federate and, after `removeObjectInstance`, a subscriber both get `ObjectInstanceNotKnown` from known-class lookup (§6.16, §6.17, §10.6) |
| `cpp-tck.remote-delete-known-object-class-lookup-boundary-contract` | Standard contract twin for known-class lookup after remote removal |
| `cpp-tck.remote-delete-object-instance-name-lookup-boundary` | The deleting federate's name-to-handle and the subscriber's handle-to-name lookups both return `ObjectInstanceNotKnown` after remote deletion (§6.16–6.17, §10.7–10.8) |
| `cpp-tck.remote-delete-object-instance-name-lookup-boundary-contract` | Standard contract twin for both object-name lookup directions after remote deletion |
| `cpp-tck.resign-delete-object-lookup-boundary` | After resign-time `DELETE_OBJECTS` and the removal callback, the surviving federate gets `ObjectInstanceNotKnown` for name-to-handle, handle-to-name, and known-class lookup (§4.12, §6.17, §§10.6–10.8) |
| `cpp-tck.resign-delete-object-lookup-boundary-contract` | Standard contract twin for all three object lookups after resign-time deletion |
| `cpp-tck.resign-no-action-owned-attributes-rejection-boundary` | `NO_ACTION` resignation returns `FederateOwnsAttributes` while an object attribute is still owned; the federate remains joined and can then resign with `DELETE_OBJECTS` (§4.12) |
| `cpp-tck.resign-no-action-owned-attributes-rejection-boundary-contract` | Standard contract twin for the owned-attributes resignation rejection |
| `cpp-tck.resign-delete-objects-then-divest-cross-object-effects` | `DELETE_OBJECTS_THEN_DIVEST` removes the departing member's registered object, releases its attribute on another member's object, and leaves that other object registered (§4.12, §6.17, §§7.2, 7.7, 7.9, 10.7–10.8) |
| `cpp-tck.resign-delete-objects-then-divest-cross-object-effects-contract` | Standard contract twin for the cross-object deletion and divestiture effects |
| `cpp-tck.resign-unconditionally-divest-attributes-preserves-objects` | `UNCONDITIONALLY_DIVEST_ATTRIBUTES` leaves another member's registered object known and releases its attribute for acquisition by a remaining federate (§4.12, §§6.8–6.9, 7.2, 7.7, 7.9, 10.7–10.8) |
| `cpp-tck.resign-unconditionally-divest-attributes-preserves-objects-contract` | Standard contract twin for resignation divestiture with retained objects |
| `cpp-tck.resign-cancel-pending-ownership-acquisitions-action` | Cancels a pending ordinary ownership request on resignation; after the owner divests, another member successfully acquires the attribute with no stale requester reservation (§4.12, §§7.2, 7.7–7.9) |
| `cpp-tck.resign-cancel-pending-ownership-acquisitions-action-contract` | Standard contract twin for pending-acquisition cleanup on resignation |
| `cpp-tck.resign-cancel-then-delete-then-divest-cross-object-effects` | Checks compound resignation cancellation, divestiture on a retained object, and removal of the departing member's separate object (§4.12, §§6.8–6.9, 6.17, 7.2, 7.7–7.9, 10.7–10.8) |
| `cpp-tck.resign-cancel-then-delete-then-divest-cross-object-effects-contract` | Standard contract twin for all three compound resignation effects |
| `cpp-tck.automatic-resign-directive-delete-objects` | Promoted standard automatic-resign contract after a joined federate is lost: `DELETE_OBJECTS`, `connectionLost`, ordinary object removal, and object-name lookup cleanup |
| `cpp-tck.automatic-resign-directive-delete-objects-contract` | Promoted adapter-backed contract for automatic resignation on connection loss using only standard callbacks, lookups, and lifecycle calls |
| `java-tck.overloads-and-exceptions` | Pre-connect `NotConnected` boundaries for listing, lookup, name reservation, and disconnect; all four official Connect overloads, unsupported callback-model rejection, duplicate-connect handling, reconnect-after-disconnect, empty-queue callback servicing, Disable/Enable Callbacks, and Disconnect |
| `java-tck.encoder-round-trip` | Official 16/32/64-bit integer, boolean, floating-point, UTF-16BE text, opaque, array, fixed-record, variant-record, and extendable-variant-record encodings, including alignment, signed element counts, caller-owned opaque storage, copied/borrowed storage, type-shape, custom boundary, and malformed-value boundaries |
| `java-tck.malformed-inputs` | Missing/malformed FOM sources and malformed encoded values |
| `cpp-tck.malformed-inputs-contract` | Standard adapter-backed malformed FOM and encoded-value input contract |
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
| `cpp-tck.federation-mom-fom-module-designator-list` | Standard `HLAFOMmoduleDesignatorList` initial request and conditional refresh after an additional-module join, compared without list-order assumptions |
| `cpp-tck.federation-mom-fom-module-designator-list-contract` | Pure standard C++ contract for federation FOM-module designator reflection and composition |
 | `cpp-tck.federation-mom-content-reports` | Standard federation MOM FOM-module and MIM content requests/reports, typed data decoding, callback-boundary behavior, and malformed-request failure |
 | `cpp-tck.federation-mom-content-reports-contract` | Pure standard C++ contract for federation MOM FOM-module and MIM content reports using adapter-supplied provider, FOM, endpoint, and callback configuration |
| `cpp-tck.federation-mom-save-status` | Standard federation MOM save-status response for every joined federate under both callback models |
| `cpp-tck.federation-mom-save-status-contract` | Pure standard C++ contract for federation MOM save-status lookup and callback delivery |
 | `cpp-tck.federation-mom-synchronization-queries` | Standard federation MOM synchronization-point list and per-federate status queries across pending, partial, missing, and completed states |
 | `cpp-tck.federation-mom-synchronization-queries-contract` | Pure standard C++ contract for federation MOM synchronization-point list/status queries using adapter-supplied MIM, FOM, endpoint, and callbacks |
 | `cpp-tck.service-report-order-transportation-lookups` | Standard MOM service reports for order and transportation lookup return values, typed arguments, reliable metadata, and serial progression |
 | `cpp-tck.service-report-order-transportation-lookups-contract` | Standard adapter-backed order/transportation lookup service-report contract using the official MIM and public APIs |
| `cpp-tck.service-report-order-transportation-lookup-failures` | Standard MOM failure reports for invalid order and transportation lookups, including typed null returns, exception classes, and serial progression |
| `cpp-tck.service-report-order-transportation-lookup-failures-contract` | Standard adapter-backed order/transportation lookup failure-report contract using provider-neutral invalid inputs |
| `cpp-tck.service-report-dimension-handle` | Standard-MIM `GetDimensionHandle` result and typed success report, including reliable delivery and reporter identity |
| `cpp-tck.service-report-dimension-handle-contract` | Adapter-backed standard C++ contract for dimension-handle lookup and its standard-MIM success report |
| `cpp-tck.service-report-dimension-handle-set` | Standard-MIM `GetDimensionHandleSet` result and success report for a one-dimension region; no regional routing assertion |
| `cpp-tck.service-report-dimension-handle-set-contract` | Adapter-backed standard C++ contract for region dimension-set lookup and its typed report |
| `cpp-tck.service-report-range-bounds` | Standard-MIM `GetRangeBounds` committed result and typed success report for a one-dimension region; no regional routing assertion |
| `cpp-tck.service-report-range-bounds-contract` | Adapter-backed standard C++ contract for committed region range lookup and its typed report |
| `cpp-tck.service-report-set-range-bounds` | Standard-MIM successful `SetRangeBounds` report with typed region/dimension/bound arguments and inherited federate identity |
| `cpp-tck.service-report-set-range-bounds-contract` | Adapter-backed standard C++ contract for the successful `SetRangeBounds` service report |
| `cpp-tck.service-report-commit-region-modifications` | Standard-MIM successful region-commit report with the region-set argument and inherited federate identity |
| `cpp-tck.service-report-commit-region-modifications-contract` | Adapter-backed standard C++ contract for the successful region-commit report |
| `cpp-tck.service-report-delete-region` | Standard-MIM successful region-deletion report with the region-designator argument and inherited federate identity |
| `cpp-tck.service-report-delete-region-contract` | Adapter-backed standard C++ contract for the successful region-deletion report |
| `cpp-tck.service-report-create-region` | Standard-MIM successful region-creation report with the dimension-set argument, returned region designator, and inherited federate identity |
| `cpp-tck.service-report-create-region-contract` | Adapter-backed standard C++ contract for the successful region-creation report |
| `cpp-tck.region-range-validation` | Standard `SetRangeBounds` accepts the inclusive dimension upper bound and rejects equal and reversed ranges with `InvalidRangeBound` |
| `cpp-tck.region-range-validation-contract` | Adapter-backed standard C++ contract for region range-bound validation |
| `cpp-tck.region-range-dimension-validation` | Standard `GetRangeBounds` and `SetRangeBounds` reject a dimension absent from the region with `RegionDoesNotContainSpecifiedDimension` |
| `cpp-tck.region-range-dimension-validation-contract` | Adapter-backed standard C++ contract for region range dimension membership |
| `cpp-tck.service-report-dimension-name` | Standard-MIM `GetDimensionName` result and typed success report, including reliable delivery and reporter identity |
| `cpp-tck.service-report-dimension-name-contract` | Adapter-backed standard C++ contract for dimension-name lookup and its standard-MIM success report |
| `cpp-tck.service-report-dimension-upper-bound` | Standard-MIM `GetDimensionUpperBound` result and typed success report |
| `cpp-tck.service-report-dimension-upper-bound-contract` | Adapter-backed standard C++ contract for dimension upper-bound lookup and its standard-MIM success report |
| `cpp-tck.service-report-interaction-dimensions` | Standard-MIM available interaction dimensions and typed success report |
| `cpp-tck.service-report-interaction-dimensions-contract` | Adapter-backed standard C++ contract for available interaction dimensions and their typed report |
| `cpp-tck.service-report-object-dimensions` | Standard-MIM available object-class dimensions and typed success report |
| `cpp-tck.service-report-object-dimensions-contract` | Adapter-backed standard C++ contract for available object-class dimensions and their typed report |
| `cpp-tck.service-report-federate-object-class-lookups` | Standard MOM service reports for federate and object-class lookup return values, typed arguments, reliable metadata, and serial progression |
 | `cpp-tck.service-report-federate-object-class-lookups-contract` | Standard adapter-backed federate/object-class lookup service-report contract using the official MIM and public APIs |
 | `cpp-tck.service-report-interaction-parameter-lookups` | Standard MOM service reports for interaction-class and parameter lookup return values, typed arguments, reliable metadata, and serial progression |
 | `cpp-tck.service-report-interaction-parameter-lookups-contract` | Standard adapter-backed interaction/parameter lookup service-report contract using the official MIM and public APIs |
 | `cpp-tck.service-report-object-attribute-update-rate-lookups` | Standard MOM service reports for object, attribute, and update-rate lookup return values, typed arguments, reliable metadata, and serial progression |
 | `cpp-tck.service-report-object-attribute-update-rate-lookups-contract` | Standard adapter-backed object/attribute/update-rate lookup service-report contract using the official MIM and public APIs |
 | `cpp-tck.service-report-federate-object-class-lookup-failures` | Standard MOM failure reports for invalid federate and object-class lookups, including typed Null returns, exception classes, recovery, and serial progression |
 | `cpp-tck.service-report-federate-object-class-lookup-failures-contract` | Standard adapter-backed federate/object-class lookup failure-report contract using provider-neutral invalid inputs |
 | `cpp-tck.service-report-interaction-parameter-lookup-failures` | Standard MOM failure reports for invalid interaction-class and parameter lookups, including typed Null returns, exception classes, recovery, and serial progression |
 | `cpp-tck.service-report-interaction-parameter-lookup-failures-contract` | Standard adapter-backed interaction/parameter lookup failure-report contract using provider-neutral invalid inputs |
 | `cpp-tck.service-report-object-attribute-update-rate-lookup-failures` | Standard MOM failure reports for invalid object, attribute, and update-rate lookups, including typed Null returns, exception classes, recovery, and serial progression |
 | `cpp-tck.service-report-object-attribute-update-rate-lookup-failures-contract` | Standard adapter-backed object/attribute/update-rate lookup failure-report contract using provider-neutral invalid inputs |
 | `cpp-tck.federation-mom-save-conditionals-contract` | Pure standard C++ contract for federation MOM save-conditionals, timed save initiation/completion, and adapter-supplied logical-time/MIM inputs |
 | `cpp-tck.joined-federate-mom-federate-state-save-restore` | Standard joined-federate MOM `HLAfederateState` transitions across save initiation/completion and restore initiation/completion, with callback and reflection metadata checks |
 | `cpp-tck.joined-federate-mom-federate-state-save-restore-contract` | Pure standard C++ contract for joined-federate MOM save/restore state transitions using adapter-supplied provider, FOM, endpoint, callback, and logical-time configuration |
 | `cpp-tck.joined-federate-mom-galt-lits-periodic-contract` | Pure standard C++ contract for joined-federate MOM `HLAGALT`/`HLALITS` direct and periodic reporting, active-regulator values, and undefined-value cleanup |
 | `cpp-tck.joined-federate-mom-tso-length-periodic-contract` | Pure standard C++ contract for joined-federate MOM `HLATSOlength` reporting around queued timestamped delivery, periodic reporting, and post-grant cleanup |
 | `cpp-tck.joined-federate-mom-ro-length-periodic` | Standard MOM test for direct and periodic `HLAROlength` values while receive-order interactions remain queued in both callback models |
 | `cpp-tck.joined-federate-mom-ro-length-periodic-contract` | Pure standard C++ contract for direct/periodic receive-order queue counts and post-delivery cleanup |
 | `cpp-tck.joined-federate-mom-removed-object-count` | Direct and `HLAsetTiming`-periodic joined-federate `HLAobjectInstancesRemoved` values after receive-order object removal, plus RTI ownership and service-report behavior |
 | `cpp-tck.joined-federate-mom-removed-object-count-contract` | Pure standard C++ contract for direct and periodic RTI-owned joined-federate removal counts, object-removal callbacks, service reporting, and cleanup |
 | `cpp-tck.joined-federate-mom-registered-object-count` | Direct and periodic joined-federate MOM `HLAobjectInstancesRegistered` counts after zero, one, and two successful adapter-supplied registrations |
 | `cpp-tck.joined-federate-mom-registered-object-count-contract` | Pure standard C++ contract for direct/periodic registration counts, reliable reflections, and standard teardown |
 | `cpp-tck.joined-federate-mom-object-lifecycle` | Standard `HLAmanager.HLAfederate` discovery, reflected identity and non-empty `HLAfederateHost`, Join-scoped FOM-designator reflection, known-object value requests, and removal on resign |
 | `cpp-tck.joined-federate-mom-object-lifecycle-contract` | Pure standard C++ contract for the joined-federate MOM object lifecycle using adapter-supplied provider, FOM, MIM, additional FOM, endpoint, and callback configuration |
  | `cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report` | Standard joined-federate MOM request/report for deletable object-instance counts grouped by object class, verified at two, one, and zero live instances |
  | `cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report-contract` | Pure standard C++ contract for the joined-federate MOM deletable-object report, including public data-element decoding and standard teardown |
  | `cpp-tck.joined-federate-mom-object-instances-updated-report` | Standard joined-federate MOM request/report for updated object-instance counts, with repeated updates counted once per instance |
  | `cpp-tck.joined-federate-mom-object-instances-updated-report-contract` | Pure standard C++ contract for decoding the MIM's class-grouped update-count records |
  | `cpp-tck.joined-federate-mom-updated-object-count-periodic` | Direct and periodic MOM counts for successful object registrations, distinct updated instances, and update invocations |
  | `cpp-tck.joined-federate-mom-updated-object-count-periodic-contract` | Pure standard contract for direct and periodic registration/update-count values and MOM reflection metadata |
  | `cpp-tck.joined-federate-mom-updates-sent-mim-attribute` | Portable direct and periodic `HLAupdatesSent` count, including repeated updates to one object |
  | `cpp-tck.joined-federate-mom-updates-sent-mim-attribute-contract` | Pure standard contract for successful update invocations, callback delivery, and periodic values |
  | `cpp-tck.joined-federate-mom-object-instances-updated-mim-attribute` | Direct and periodic `HLAobjectInstancesUpdated` count, proving repeated updates count once per distinct object |
  | `cpp-tck.joined-federate-mom-object-instances-updated-mim-attribute-contract` | Pure standard contract for distinct-object counts and periodic MOM reflection |
  | `cpp-tck.joined-federate-mom-object-instances-reflected-mim-attribute` | Direct and periodic `HLAobjectInstancesReflected` count once per distinct application object receiving a reflection |
  | `cpp-tck.joined-federate-mom-object-instances-reflected-mim-attribute-contract` | Pure standard contract for distinct reflected-object counts, callback delivery, and periodic MOM reflection |
   | `cpp-tck.joined-federate-mom-reflections-received-mim-attribute` | Direct and periodic `HLAreflectionsReceived` increments once per ordinary reflection callback, including repeated updates to one object |
   | `cpp-tck.joined-federate-mom-reflections-received-mim-attribute-contract` | Pure standard contract for baseline-relative invocation counts, callback metadata, and periodic MOM reflection |
  | `cpp-tck.joined-federate-mom-object-instances-discovered-mim-attribute` | Direct and periodic `HLAobjectInstancesDiscovered` counts, including rediscovery after local deletion |
  | `cpp-tck.joined-federate-mom-object-instances-discovered-mim-attribute-contract` | Pure-standard contract for discovery-count deltas, local-delete rediscovery, and reflection metadata |
  | `cpp-tck.joined-federate-mom-object-instances-removed-mim-attribute` | Recipient `HLAobjectInstancesRemoved` counts correlated with committed receive-order removal callbacks and periodic reporting |
  | `cpp-tck.joined-federate-mom-object-instances-removed-mim-attribute-contract` | Pure-standard contract for recipient removal-count deltas, callback metadata, and periodic reflection |
  | `cpp-tck.joined-federate-mom-object-instances-deleted-mim-attribute` | Direct and periodic `HLAobjectInstancesDeleted` counts correlated with receive-order removal callbacks |
  | `cpp-tck.joined-federate-mom-object-instances-deleted-mim-attribute-contract` | Pure standard contract for deletion-call counts, periodic reflection, and exact removal metadata |
  | `cpp-tck.joined-federate-mom-discovered-object-count-periodic` | Direct and periodic MOM discovery count deltas, including rediscovery after local deletion |
  | `cpp-tck.joined-federate-mom-discovered-object-count-periodic-contract` | Pure standard contract for discovery-count delta, local-delete rediscovery, typed values, and reflection metadata |
  | `cpp-tck.joined-federate-mom-deleted-object-count-periodic` | Direct and periodic MOM Delete Object Instance invocation counts correlated with receiver-side removals |
  | `cpp-tck.joined-federate-mom-deleted-object-count-periodic-contract` | Pure standard contract for direct and periodic deletion counts, removal callbacks, typed values, and reflection metadata |
  | `cpp-tck.joined-federate-mom-object-instances-updated-timestamped-report` | Standard MOM updated-instance count after two timestamped updates are reflected to a constrained observer |
  | `cpp-tck.joined-federate-mom-object-instances-updated-timestamped-report-contract` | Pure standard contract for timestamped update/reflect metadata, time grants, and class-grouped updated-object report decoding |
  | `cpp-tck.joined-federate-mom-object-instances-reflected-report` | Standard MOM reflected-instance count query from a distinct requester targeting an active subscriber, with repeated reflections counted once per instance |
  | `cpp-tck.joined-federate-mom-object-instances-reflected-report-contract` | Pure standard C++ contract for target-handle selection, reflection callbacks, and class-grouped reflected-instance count decoding |
  | `cpp-tck.joined-federate-mom-object-instances-reflected-timestamped-report` | Standard MOM reflected-instance count after three timestamped reflections to two instances |
  | `cpp-tck.joined-federate-mom-object-instances-reflected-timestamped-report-contract` | Pure standard contract for timestamped reflection metadata, time-grant ordering, and class-grouped report decoding |
  | `cpp-tck.joined-federate-mom-deletable-object-count` | Direct and periodic `HLAobjectInstancesThatCanBeDeleted` counts at zero, one, and two live objects and after deletion |
  | `cpp-tck.joined-federate-mom-deletable-object-count-contract` | Pure standard C++ contract for direct/periodic deletable-object counts, typed decoding, reflection metadata, and teardown |
  | `cpp-tck.joined-federate-mom-updates-sent-counts-contract` | Pure standard C++ contract for joined-federate `HLArequestUpdatesSent`/`HLAreportUpdatesSent` buckets, nested counts, and transportation-change confirmation |
 | `cpp-tck.joined-federate-mom-interactions-received-counts-contract` | Pure standard C++ contract for joined-federate `HLArequestInteractionsReceived`/`HLAreportInteractionsReceived` buckets, nested counts, and transportation-change confirmation |
 | `cpp-tck.joined-federate-mom-interactions-received-mim-attribute` | Direct and periodic standard MIM `HLAinteractionsReceived` values tied to exact application interaction callback counts in both callback models |
 | `cpp-tck.joined-federate-mom-interactions-received-mim-attribute-contract` | Pure standard contract for initial, direct, periodic, and post-disable joined-federate interactions-received attribute values |
 | `cpp-tck.joined-federate-mom-interactions-sent-mim-attribute` | Direct and periodic standard MIM `HLAinteractionsSent` values; three accepted ordinary application sends add exactly three without claiming DDM coverage |
 | `cpp-tck.joined-federate-mom-interactions-sent-mim-attribute-contract` | Pure standard contract for initial, baseline-relative direct, and periodic joined-federate interactions-sent values |
 | `cpp-tck.joined-federate-mom-directed-interactions-received-mim-attribute` | Direct and periodic `HLAdirectedInteractionsReceived` checks distinguish an ordinary callback from directed callbacks and cross-check `HLAinteractionsReceived` |
 | `cpp-tck.joined-federate-mom-directed-interactions-received-mim-attribute-contract` | Pure standard contract for the directed receive subset, total interaction count, ordinary exclusion, periodic values, and teardown |
 | `cpp-tck.joined-federate-mom-directed-interactions-sent-mim-attribute` | Direct and periodic `HLAdirectedInteractionsSent` values distinguish ordinary from directed sends and cross-check `HLAinteractionsSent` |
 | `cpp-tck.joined-federate-mom-directed-interactions-sent-mim-attribute-contract` | Pure standard contract for the directed-send scalar, total-send cross-check, periodic reports, and no-region scope |
 | `cpp-tck.joined-federate-mom-directed-interactions-received-counts-contract` | Pure standard C++ contract for joined-federate `HLArequestDirectedInteractionsReceived`/`HLAreportDirectedInteractionsReceived` buckets, directed-versus-ordinary filtering, nested counts, and transportation-change confirmation |
 | `cpp-tck.joined-federate-mom-directed-interactions-sent-counts-contract` | Pure standard C++ contract for joined-federate `HLArequestDirectedInteractionsSent`/`HLAreportDirectedInteractionsSent` buckets, directed-versus-ordinary filtering, nested counts, and transportation-change confirmation |
 | `cpp-tck.joined-federate-mom-reflections-received-counts-contract` | Pure standard C++ contract for joined-federate `HLArequestReflectionsReceived`/`HLAreportReflectionsReceived` buckets, nested counts, and attribute-transportation confirmation |
 | `cpp-tck.joined-federate-mom-reflection-counts-contract` | Pure standard C++ contract for joined-federate direct and periodic `HLAobjectInstancesReflected`/`HLAreflectionsReceived` counts, timestamped reflections, and time-advance callbacks |
 | `cpp-tck.joined-federate-mom-time-state-durations-contract` | Pure standard C++ contract for joined-federate direct and periodic `HLAtimeGrantedTime`/`HLAtimeAdvancingTime` durations, `HLAinteger32BE` decoding, and MOM reflection metadata |
 | `cpp-tck.joined-federate-mom-interactions-sent-counts-contract` | Pure standard C++ contract for joined-federate `HLArequestInteractionsSent`/`HLAreportInteractionsSent` buckets, nested counts, and transportation-change confirmation |
  | `cpp-tck.service-report-interaction-failure` | Standard MOM failure reports for invalid ordinary interaction class, parameter, and publication inputs, including typed report arguments and no application callback |
  | `cpp-tck.service-report-interaction-failure-contract` | Standard adapter-backed ordinary interaction service-report failure contract using the official MIM and interaction callbacks |
  | `cpp-tck.service-report-interaction-contract` | Standard adapter-backed ordinary interaction service-report contract using the official MIM and interaction callbacks |
   | `cpp-tck.service-report-receive-order-interaction` | Standard MOM service report paired with ordinary receive-order interaction delivery, including callback-model timing and application metadata |
   | `cpp-tck.service-report-receive-order-interaction-contract` | Standard adapter-backed receive-order interaction service-report contract using the official MIM and separate application/report callbacks |
    | `cpp-tck.service-report-timestamped-directed-interaction` | Standard MOM service report paired with time-regulated timestamped directed-interaction delivery, typed retraction identity, target routing, and callback-before-grant ordering |
    | `cpp-tck.service-report-timestamped-directed-interaction-contract` | Standard adapter-backed timestamped directed-interaction service-report contract using the official MIM, logical-time API, and separate application/report callbacks |
    | `cpp-tck.service-report-timestamped-attribute-update` | Standard MOM service report paired with time-regulated timestamped `UpdateAttributeValues`, typed timestamp/retraction arguments, and reflection-before-grant ordering |
    | `cpp-tck.service-report-timestamped-attribute-update-contract` | Standard adapter-backed timestamped attribute-update service-report contract using the official MIM, logical-time API, and separate application/report callbacks |
    | `cpp-tck.service-report-timestamped-delete-object-instance` | Standard MOM service report paired with time-regulated timestamped `DeleteObjectInstance`, typed timestamp/retraction arguments, and removal-before-grant ordering |
    | `cpp-tck.service-report-timestamped-delete-object-instance-contract` | Standard adapter-backed timestamped object-deletion service-report contract using the official MIM, logical-time API, and separate application/report callbacks |
   | `cpp-tck.service-report-interlock` | Standard MOM service-reporting switch and HLAreportServiceInvocation active/passive subscription interlocks, including recovery after unsubscribe |
  | `cpp-tck.service-report-interlock-contract` | Standard adapter-backed service-reporting interlock contract using the official MIM and public switch/subscription APIs |
  | `cpp-tck.federate-service-reporting-mom-switch` | Standard federate-level HLAsetSwitches changes for Service Reporting, read back through the public switch getter |
  | `cpp-tck.federate-service-reporting-mom-switch-contract` | Pure standard C++ contract for the federate MOM Service Reporting switch |
  | `cpp-tck.federate-service-reporting-mim-attribute` | Direct requests and conditional MOM reflections for `HLAserviceReporting` after standard API change and restoration |
  | `cpp-tck.federate-service-reporting-mim-attribute-contract` | Pure standard contract for typed switch values and conditional reflection metadata |
  | `cpp-tck.federate-object-class-relevance-advisory-mim-attribute` | Direct requests and conditional MOM reflections for `HLAobjectClassRelevanceAdvisory` after standard API change and restoration |
  | `cpp-tck.federate-object-class-relevance-advisory-mim-attribute-contract` | Pure standard contract for typed object-class advisory values and conditional reflection metadata |
  | `cpp-tck.federate-attribute-relevance-advisory-mim-attribute` | Direct requests and conditional MOM reflections for `HLAattributeRelevanceAdvisory` after standard API change and restoration |
  | `cpp-tck.federate-attribute-relevance-advisory-mim-attribute-contract` | Pure standard contract for typed attribute advisory values and conditional reflection metadata |
  | `cpp-tck.federate-attribute-scope-advisory-mim-attribute` | Direct requests and conditional MOM reflections for `HLAattributeScopeAdvisory` after standard API change and restoration |
  | `cpp-tck.federate-attribute-scope-advisory-mim-attribute-contract` | Pure standard contract for typed scope advisory values and conditional reflection metadata |
  | `cpp-tck.federate-interaction-relevance-advisory-mim-attribute` | Direct requests and conditional MOM reflections for `HLAinteractionRelevanceAdvisory` after standard API change and restoration |
  | `cpp-tck.federate-interaction-relevance-advisory-mim-attribute-contract` | Pure standard contract for typed interaction advisory values and conditional reflection metadata |
  | `cpp-tck.federate-exception-reporting-mom-switch` | Standard federate-level HLAsetSwitches changes for Exception Reporting, with neighboring switch isolation |
  | `cpp-tck.federate-exception-reporting-mom-switch-contract` | Pure standard C++ contract for the federate MOM Exception Reporting switch |
  | `cpp-tck.federate-exception-reporting-mim-attribute` | Direct requests and conditional MOM reflections for `HLAexceptionReporting` after standard API change and restoration |
  | `cpp-tck.federate-exception-reporting-mim-attribute-contract` | Pure standard contract for typed switch values and conditional reflection metadata |
  | `cpp-tck.federate-exception-report-delivery` | Standard HLAreportException callback when enabled, suppression when disabled, exact member handle, typed payloads, and reliable RTI origin |
  | `cpp-tck.federate-exception-report-delivery-contract` | Pure standard C++ contract for standard MIM Exception Reporting delivery and switch gating |
  | `cpp-tck.federate-mom-exception-report-delivery` | Malformed HLAresignAction and HLAswitch values each report through standard HLAreportMOMexception with HLAparameterError true |
  | `cpp-tck.federate-mom-exception-report-delivery-contract` | Pure standard C++ contract for malformed-parameter HLAreportMOMexception cases |
  | `cpp-tck.federate-mom-exception-missing-parameter` | Omission of HLAsyncPointName from HLArequestSynchronizationPointStatus produces HLAreportMOMexception with HLAparameterError true |
  | `cpp-tck.federate-mom-exception-missing-parameter-contract` | Pure standard C++ contract for missing-parameter HLAreportMOMexception delivery |
  | `cpp-tck.federation-mom-exception-missing-fom-module-indicator` | Omission of HLAFOMmoduleIndicator from federation-level HLArequestFOMmoduleData produces HLAreportMOMexception with HLAparameterError true |
  | `cpp-tck.federation-mom-exception-missing-fom-module-indicator-contract` | Pure standard C++ contract for missing-indicator HLAreportMOMexception delivery |
  | `cpp-tck.federate-mom-fom-module-content-report` | Requests an adapter-supplied FOM module from a distinct joined federate and verifies the standard module indicator, XML content, and reliable report |
  | `cpp-tck.federate-mom-fom-module-content-report-contract` | Pure standard C++ contract for federate-scoped HLArequestFOMmoduleData/HLAreportFOMmoduleData |
  | `cpp-tck.federate-mom-object-instance-information` | Verifies owned, known-but-unowned, and NULL HLAreportObjectInstanceInformation response shapes, including nested attribute-handle-list decoding |
  | `cpp-tck.federate-mom-object-instance-information-contract` | Pure standard C++ contract for object-information MOM reports before and after local deletion |
  | `cpp-tck.federate-mom-publication-query` | Verifies published object class/attribute and interaction class reports plus the required empty directed-interaction publication report |
  | `cpp-tck.federate-mom-publication-query-contract` | Pure standard C++ contract for federate MOM publication reports and the directed-publication NULL response |
  | `cpp-tck.federate-mom-subscription-query` | Verifies an active object-attribute subscription, active interaction subscriptions, and the required empty directed-interaction subscription report |
  | `cpp-tck.federate-mom-subscription-query-contract` | Pure standard C++ contract for federate MOM subscription reports |
  | `cpp-tck.federate-mom-passive-subscription-query` | Verifies a passive object-attribute subscription, active interaction subscription, and required empty directed-interaction report |
  | `cpp-tck.federate-mom-passive-subscription-query-contract` | Pure standard C++ contract for passive federate MOM subscription reporting |
  | `cpp-tck.federate-mom-passive-interaction-subscription-query` | Verifies an active object-attribute subscription, passive interaction subscription, and required empty directed-interaction report |
  | `cpp-tck.federate-mom-passive-interaction-subscription-query-contract` | Pure standard C++ contract for passive interaction subscription reporting |
  | `cpp-tck.federate-mom-exception-report-service-precondition` | Service Reporting enablement failure while subscribed to HLAreportServiceInvocation reports HLAparameterError false |
  | `cpp-tck.federate-mom-exception-report-service-precondition-contract` | Pure standard C++ contract for the Service Reporting MOM precondition report |
  | `cpp-tck.federate-send-service-reports-to-file-mom-switch` | Standard federate-level HLAsetSwitches changes for the service-report destination switch without activating report emission |
  | `cpp-tck.federate-send-service-reports-to-file-mom-switch-contract` | Pure standard C++ contract for the federate MOM Send Service Reports to File switch |
  | `cpp-tck.federate-automatic-resign-action-mom-switch` | Standard federate-level HLAsetSwitches updates for HLAautomaticResignAction using official enum encoding and restoring the initial directive |
  | `cpp-tck.federate-automatic-resign-action-mom-switch-contract` | Pure standard C++ contract for the federate MOM Automatic Resign Action switch |
  | `cpp-tck.federate-automatic-resign-action-mim-attribute` | Direct requests and conditional MOM reflections for `HLAautomaticResignAction` after standard API change and restoration |
  | `cpp-tck.federate-automatic-resign-action-mim-attribute-contract` | Pure standard contract for typed resign-action values and conditional reflection metadata |
  | `cpp-tck.service-report-synchronization` | Standard MOM service reports for synchronization-point registration, confirmation, announcement, achievement, and completion callbacks |
| `cpp-tck.service-report-synchronization-contract` | Standard adapter-backed synchronization service-report contract using the official MIM and synchronization callbacks |
| `cpp-tck.service-report-regional-interaction` | Standard MOM service-report callback for regional `SendInteractionWithRegions`, paired with overlap-qualified regional application delivery and conveyed source-region metadata |
| `cpp-tck.service-report-regional-interaction-contract` | Pure standard C++ contract for successful regional interaction service reporting, typed MOM invocation metadata, and overlap-qualified delivery using adapter-supplied MIM/DDM inputs |
| `cpp-tck.service-report-regional-interaction-subscription` | Standard MOM service-report callbacks for regional `SubscribeInteractionClassWithRegions` and `UnsubscribeInteractionClassWithRegions`, including the passive-subscription indicator, typed association arguments, serial progression, and standard MIM/DDM setup |
| `cpp-tck.service-report-regional-interaction-subscription-contract` | Pure standard C++ contract for regional interaction subscription/unsubscription reports, passive state, typed associations, serial progression, and adapter-supplied MIM/DDM inputs |
| `cpp-tck.service-report-object-attribute-declaration` | Standard MOM service reports for failed and successful ordinary `SubscribeObjectClassAttributes`, `UnsubscribeObjectClassAttributes`, and whole-class `UnsubscribeObjectClass` declarations |
| `cpp-tck.service-report-object-attribute-declaration-contract` | Pure standard C++ contract for typed ordinary object-attribute declaration service reports, including invalid-handle failures, passive indicators, null returns, exceptions, and serial progression |
| `cpp-tck.service-report-directed-interaction-declaration` | Standard MOM service reports for failed and successful directed `SubscribeObjectClassDirectedInteractions` and `UnsubscribeObjectClassDirectedInteractions` declarations, including whole-class unsubscribe |
| `cpp-tck.service-report-directed-interaction-declaration-contract` | Pure standard C++ contract for typed directed object-class interaction declaration service reports, including invalid-handle failures, universal indicators, null returns, exceptions, and serial progression |
| `cpp-tck.service-report-directed-interaction-publication` | Standard MOM service reports for failed and successful directed `PublishObjectClassDirectedInteractions` and `UnpublishObjectClassDirectedInteractions` declarations, including whole-class unpublish |
| `cpp-tck.service-report-directed-interaction-publication-contract` | Pure standard C++ contract for typed directed object-class interaction publication service reports, including invalid-handle failures, selective sets, null returns, exceptions, and serial progression |
| `cpp-tck.service-report-interaction-publication` | Standard MOM service reports for failed and successful ordinary `PublishInteractionClass` and `UnpublishInteractionClass` declarations |
| `cpp-tck.service-report-interaction-publication-contract` | Pure standard C++ contract for typed ordinary interaction-class publication service reports, including invalid-handle failures, null returns, exceptions, and serial progression |
| `cpp-tck.service-report-interaction-subscription` | Standard MOM service reports for failed and successful ordinary `SubscribeInteractionClass` and `UnsubscribeInteractionClass` declarations, including the passive-subscription indicator |
| `cpp-tck.service-report-interaction-subscription-contract` | Pure standard C++ contract for typed ordinary interaction-class subscription service reports, including invalid-handle failures, passive indicators, null returns, exceptions, and serial progression |
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
| `cpp-tck.attribute-value-update-request-multi-requester` | Two active requesters preserve independent Request Attribute Value Update tags, receive separate provider callbacks without loopback, and receive the provider response fan-out |
| `cpp-tck.attribute-value-update-request-multi-requester-contract` | Standard adapter-backed contract for independent ordinary value requests and response fan-out across two active subscribers |
| `cpp-tck.service-report-delete-object-instance` | Standard MOM service-report callback for successful object deletion, paired with ordinary removal delivery and the same seven-parameter report contract |
| `cpp-tck.service-report-delete-object-instance-contract` | Standard adapter-backed ordinary object-deletion service-report contract using the official MIM and removal callback |
| `cpp-tck.service-report-delete-object-instance-failure` | Standard MOM failure reports for invalid and stale object deletion, including failure status, exception, returned-argument encoding, serial progression, and removal cleanup |
| `cpp-tck.service-report-delete-object-instance-failure-contract` | Standard adapter-backed object-deletion failure-report contract using the official MIM, object, and standard data-element callbacks |
| `java-tck.logical-time-factory` | Pre-connect and pre-join `getTimeFactory` lifecycle boundaries, standard `HLAinteger64Time`/`HLAfloat64Time` value and factory checks, concrete time/interval copy and assignment independence, zero/epsilon mutators, interval differences, direct interval encoding, default/named/unknown logical-time factory selection, adapter-selected logical-time values and zero/epsilon intervals, public `HLAlogicalTime`/`HLAlogicalTimeInterval` data-element round trips, nested-buffer boundaries, clone/copy independence, incompatible-type rejection, variable-length and direct-buffer encode/decode parity, encoded-length checks, truncated-buffer rejection, boundary transitions, comparison, interval arithmetic/order, and illegal underflow/overflow boundaries |
| `java-tck.time-advance` | Time-service `NotConnected`/`FederateNotExecutionMember` boundaries, Time Regulation/Constrained roles, pre-membership lookahead boundaries, logical-time and lookahead queries, deferred lookahead decrease, all alternate advance entry points, asynchronous-delivery controls, Time Advance Request/Grant, standard duplicate-role, in-progress, backward-time, and duplicate-disable failure boundaries, and role shutdown |
| `cpp-tck.time-advance-contract` | Standard adapter-backed time-role, logical-time query, advance, asynchronous-delivery, grant, and negative-boundary contract |
| `cpp-tck.modify-lookahead` | Query and modify the standard time-regulation lookahead, including pre-regulation failures, valid increase, incompatible-interval rejection, deferred decrease, and post-grant application |
| `cpp-tck.modify-lookahead-contract` | Pure standard C++ contract for Query Lookahead and Modify Lookahead across the deferred-decrease boundary |
| `java-tck.support-services` | Full public federate, object, attribute, interaction, parameter, object-instance, and dimension name/handle lookup in both directions with pre-connect/pre-join lifecycle boundaries, standard invalid-name/handle boundaries, order/transport/update-rate/normalization lookup lifecycle boundaries, all public handle decoder lifecycle boundaries, ordinary handle encode/decode, direct-buffer and `VariableLengthData&` handle encoding, encoded-length and truncated-buffer checks, copied-handle equality/hash/ordering stability, valid `AttributeHandleSet` copy/assignment/lookup/erase semantics, independent `AttributeHandleValueMap` and `ParameterHandleValueMap` value storage, normalization stability, available-dimension boundaries, and order/transportation handles |
| `cpp-tck.support-services-contract` | Standard adapter-backed support-service contract for public lookup, normalization, handle encoding/decoding, available dimensions, order, transportation, update-rate, and lifecycle-boundary behavior |
| `cpp-tck.handle-normalization-membership-boundaries` | For the standard service-group normalizer and all four handle-normalization services, checks `NotConnected` before connect and `FederateNotExecutionMember` before join and after resignation while connected; peer handles are valid (§§10.29–10.33) |
| `cpp-tck.handle-normalization-membership-boundaries-contract` | Standard contract twin for the five normalization-service call-state boundaries; does not assert provider-specific normalized values or encodings |
| `cpp-tck.standard-order-and-transportation-lookups` | Independently selectable portable mandatory order and transportation lookup lifecycle, round-trip, cross-federate stability, invalid-input, and cleanup slice |
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
| `cpp-tck.object-registration-service-boundaries` | Object-registration pre-connect and pre-join failures, standard discovery identity, local deletion, and post-deletion lookup boundaries |
| `cpp-tck.object-registration-service-boundaries-contract` | Pure standard C++ contract for object-registration service boundaries, discovery identity, local deletion, and lookup cleanup |
| `cpp-tck.object-registration-discovery-multi-recipient` | Two active subscribers discover two ordinary registered objects, preserve object name/handle/class identity, and exclude the registering owner from discovery callbacks |
| `cpp-tck.object-registration-discovery-multi-recipient-contract` | Standard adapter-backed ordinary object registration/discovery fan-out contract across two active subscribers with owner loopback suppression |
| `cpp-tck.named-registration` | Standard object-name reservation/release, multiple-name reservation, named registration, discovery and identity lookup, reservation contention, failed-registration reuse, and callback-model parity |
| `cpp-tck.named-registration-contract` | Standard adapter-backed named object-registration contract for reservation/release and reuse, multiple-name lifecycle, discovery and identity lookup, contention, and invalid-name boundaries |
| `cpp-tck.named-registration-multi-recipient` | Two active subscribers discover two explicitly reserved/named objects, preserve object name/handle/class identity, and exclude the registering owner from discovery callbacks |
| `cpp-tck.named-registration-multi-recipient-contract` | Standard adapter-backed named object-registration fan-out contract across two active subscribers with owner loopback suppression |
| `cpp-tck.object-attribute-subscription-lifecycle-contract` | Standard adapter-backed ordinary object-attribute subscription lifecycle for passive/active discovery, reflection, downgrade/reactivation, unsubscription, and stable identity lookups |
| `cpp-tck.local-delete-object-instance` | Standard local object deletion boundaries, ownership protection, pending-acquisition protection, fresh-session deletion, rediscovery, and continued ordinary attribute reflection |
| `cpp-tck.local-delete-object-instance-contract` | Standard adapter-backed local object deletion contract for service boundaries, ownership protection, fresh-session deletion, rediscovery, stable identity, and continued ordinary reflection |
| `java-tck.attribute-interaction` | Pre-connect and pre-join ordinary interaction-send boundaries plus active and passive attribute Update/Reflect and interaction Send/Receive, values, parameters, tags, producer, and transportation |
| `cpp-tck.attribute-interaction-contract` | Standard adapter-backed ordinary attribute Update/Reflect and interaction Send/Receive contract with values, parameters, tags, producer identity, transportation, and lifecycle boundaries |
| `cpp-tck.attribute-multi-recipient-fifo` | Two active subscribers each receive ordinary attribute updates in producer send order, with owner exclusion and preserved object, attribute, tag, producer, and transportation metadata |
| `cpp-tck.attribute-multi-recipient-fifo-contract` | Standard adapter-backed ordinary attribute fan-out and receive-order FIFO contract across two active subscribers |
| `cpp-tck.ordinary-multi-attribute-subscription-projection` | One ordinary update containing two attributes is projected to both-, first-only-, and second-only subscribers, with selective updates, values, tags, producer, transportation, and owner-loopback checks |
| `cpp-tck.ordinary-multi-attribute-subscription-projection-contract` | Standard adapter-backed ordinary multi-attribute subscription projection contract using only the official API and an adapter-supplied FOM |
| `cpp-tck.ordinary-multi-attribute-value-update-request-response` | Full and selective ordinary multi-attribute value requests reach the provider with exact request tags and return exact reflected values with producer and transportation metadata |
| `cpp-tck.ordinary-multi-attribute-value-update-request-response-contract` | Standard adapter-backed ordinary multi-attribute Request Attribute Value Update and provider response contract using only the official API and an adapter-supplied FOM |
| `cpp-tck.receive-order-multi-attribute-update-callback-cancellation` | A queued ordinary multi-attribute reflection is projected per subscriber, withdrawn before evoked callback servicing, and kept inactive for later updates |
| `cpp-tck.receive-order-multi-attribute-update-callback-cancellation-contract` | Standard adapter-backed receive-order multi-attribute callback-withdrawal contract using only the official API and an adapter-supplied FOM |
| `cpp-tck.object-removal-multi-recipient-fifo` | Two active subscribers each receive the owner’s ordinary object removal with preserved object, tag, and producer metadata, while the owner receives no loopback removal |
| `cpp-tck.object-removal-multi-recipient-fifo-contract` | Standard adapter-backed ordinary object-removal fan-out and owner-exclusion contract across two active subscribers |
| `cpp-tck.receive-order-object-removal-subscription-withdrawal` | A queued terminal object removal remains deliverable after one active subscriber withdraws before callback servicing, with immediate and evoked callback-model checks |
| `cpp-tck.receive-order-object-removal-subscription-withdrawal-contract` | Standard adapter-backed terminal object-removal delivery contract across subscription withdrawal and callback-model boundaries |
| `cpp-tck.resign-delete-objects-multi-recipient-fifo` | Publisher resignation with `DELETE_OBJECTS` delivers two ordinary removals to both active subscribers in producer order, with empty resign tags and no owner loopback |
| `cpp-tck.resign-delete-objects-multi-recipient-fifo-contract` | Standard adapter-backed resign-time object-removal fan-out and owner-exclusion contract across two active subscribers |
| `cpp-tck.order-type-controls-contract` | Standard adapter-backed prospective attribute and interaction order-control contract with logical-time delivery and receive-order callbacks |
| `cpp-tck.order-type-change` | Portable standard order-type controls: prospective defaults, per-object override, receive-order interaction control, timestamped delivery, order metadata, and retraction identity |
| `cpp-tck.order-type-change-contract` | Pure standard C++ contract for attribute and interaction order-type controls using only official API headers and the standard library |
  | `cpp-tck.receive-order-attribute-update-callback-cancellation` | In evoked mode, cancel a queued receive-order attribute reflection by unsubscribing before callback servicing; in immediate mode, verify delivery before the subscription is removed |
 | `cpp-tck.receive-order-attribute-update-callback-cancellation-contract` | Standard adapter-backed receive-order attribute-reflection cancellation contract across evoked and immediate callback boundaries |
 | `cpp-tck.receive-order-attribute-update` | Deliver two consecutive ordinary receive-order attribute updates in producer order, preserving reflection metadata and excluding publisher loopback |
 | `cpp-tck.receive-order-attribute-update-contract` | Pure standard C++ contract for ordinary receive-order attribute publication, discovery, reflection ordering, metadata, and teardown |
 | `cpp-tck.receive-order-interaction` | Deliver two consecutive ordinary receive-order interactions in producer order, preserving parameter, tag, producer, and transportation metadata without sender loopback |
 | `cpp-tck.receive-order-interaction-contract` | Pure standard C++ contract for ordinary receive-order interaction publication, subscription, delivery ordering, metadata, and teardown |
| `cpp-tck.receive-order-object-removal` | Deliver an ordinary object removal after discovery, preserving removal tag and producer metadata without publisher loopback |
| `cpp-tck.receive-order-object-removal-contract` | Pure standard C++ contract for ordinary object registration/discovery, deletion, removal metadata, and teardown |
| `cpp-tck.object-deletion-service-boundaries` | Object-deletion pre-connect and pre-join failures, unknown-object rejection, local deletion, remote removal metadata, resign-time deletion, and lookup cleanup |
| `cpp-tck.object-deletion-service-boundaries-contract` | Pure standard C++ contract for object-deletion service boundaries, removal metadata, and resignation cleanup |
| `cpp-tck.object-registration-service-boundaries` | Object-registration pre-connect and pre-join failures, publication/subscription boundaries, discovery, stable handle lookups, and standard cleanup |
| `cpp-tck.object-registration-service-boundaries-contract` | Pure standard C++ contract for object-registration service boundaries, discovery, handle lookups, and lifecycle cleanup |
| `cpp-tck.attribute-update-service-boundaries` | Ordinary active/passive attribute Update/Reflect, payload/tag/producer/transport metadata, and standard cleanup |
| `cpp-tck.attribute-update-service-boundaries-contract` | Pure standard C++ contract for ordinary attribute Update/Reflect service boundaries |
| `cpp-tck.interaction-service-boundaries` | Ordinary active/passive interaction Send/Receive, invalid-class rejection, payload/tag/producer/transport metadata, and standard cleanup |
| `cpp-tck.interaction-service-boundaries-contract` | Pure standard C++ contract for ordinary interaction Send/Receive service boundaries |
| `cpp-tck.attribute-value-request-service-boundaries` | Object-instance and object-class Request Attribute Value Update overloads, invalid handles, request tags, provider callbacks, responses, and reflection |
| `cpp-tck.attribute-value-request-service-boundaries-contract` | Pure standard C++ contract for Request Attribute Value Update service boundaries |
 | `cpp-tck.receive-order-interaction-callback-cancellation` | In evoked mode, cancel a queued receive-order interaction by unsubscribing before callback servicing; in immediate mode, verify delivery before the subscription is removed |
| `cpp-tck.receive-order-interaction-callback-cancellation-contract` | Standard adapter-backed receive-order interaction cancellation contract across evoked and immediate callback boundaries |
| `cpp-tck.interaction-subscription-lifecycle` | Passive ordinary interaction subscriptions suppress delivery, active replacement enables it without replay, downgrade suppresses later messages, and unsubscribe removes the declaration |
| `cpp-tck.interaction-subscription-lifecycle-contract` | Standard adapter-backed ordinary interaction subscription lifecycle contract for passive/active declarations, downgrade, reactivation, delivery, and unsubscription |
| `cpp-tck.interaction-publication-send-fence` | Whole-class unpublication makes `sendInteraction` fail with `InteractionClassNotPublished`; republication restores parameter delivery and standard metadata |
| `cpp-tck.interaction-publication-send-fence-contract` | Standard adapter-backed ordinary interaction publication, unpublication, send, republication, parameter delivery, and lifecycle contract |
| `cpp-tck.interaction-multi-recipient-fifo` | Two active subscribers each receive ordinary interactions in producer send order, with sender exclusion and preserved parameter, tag, producer, and transportation metadata |
| `cpp-tck.interaction-multi-recipient-fifo-contract` | Standard adapter-backed ordinary interaction fan-out and receive-order FIFO contract across two active subscribers |
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
| `cpp-tck.timestamped-regional-attribute-update-contract` | Standard adapter-backed timestamped regional attribute update and retraction contract |
| `cpp-tck.timestamped-regional-attribute-alternate-advances` | Timestamped regional Update/Reflect through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with grant/query bounds and backward-time failures |
| `cpp-tck.timestamped-regional-attribute-alternate-advances-contract` | Standard adapter-backed timestamped regional attribute alternate-advance contract |
| `cpp-tck.timestamped-regional-attribute-association-replacement` | Queued timestamped regional updates retain their original source association while later updates use an explicitly replaced source region |
| `cpp-tck.timestamped-regional-attribute-association-replacement-contract` | Standard adapter-backed timestamped regional attribute source-association replacement contract |
| `cpp-tck.timestamped-regional-attribute-regulation-reenable` | Queued timestamped regional Update/Reflect survives Time Regulation disable/re-enable with changed lookahead, preserving the original source-region snapshot, grant ordering, and retraction |
| `cpp-tck.timestamped-regional-attribute-regulation-reenable-contract` | Standard adapter-backed timestamped regional attribute Time Regulation re-enable contract |
| `cpp-tck.timestamped-regional-attribute-source-resignation` | A queued timestamped regional Update/Reflect remains deliverable after its producer resigns, preserving object, producer, source-region, payload, time, order, and retraction metadata |
| `cpp-tck.timestamped-regional-attribute-source-resignation-contract` | Standard adapter-backed timestamped regional attribute source-resignation contract |
| `cpp-tck.timestamped-default-region-attribute-alternate-advances` | Timestamped default-region Update/Reflect through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with supplied-empty region metadata and grant ordering |
| `cpp-tck.timestamped-default-region-attribute-alternate-advances-contract` | Standard adapter-backed timestamped default-region attribute alternate-advance contract |
| `cpp-tck.timestamped-default-region-attribute-reenable` | Queued timestamped default-region Update/Reflect survives Time Constrained disable/re-enable, with supplied-empty source metadata, grant ordering, and retraction |
| `cpp-tck.timestamped-default-region-attribute-reenable-contract` | Standard adapter-backed timestamped default-region attribute Time Constrained re-enable contract |
| `cpp-tck.timestamped-default-region-attribute-regulation-reenable` | Queued timestamped default-region Update/Reflect survives Time Regulation disable/re-enable with changed lookahead, including Query Lookahead, grant ordering, supplied-empty source metadata, and retraction |
| `cpp-tck.timestamped-default-region-attribute-regulation-reenable-contract` | Standard adapter-backed timestamped default-region attribute Time Regulation re-enable contract |
| `cpp-tck.timestamped-default-region-attribute-mixed-fanout` | Timestamped default-region Update/Reflect splits between an immediate and a constrained regional recipient, with delivered-copy Request Retraction and pending-copy suppression |
| `cpp-tck.timestamped-default-region-attribute-mixed-fanout-contract` | Standard adapter-backed timestamped default-region attribute mixed-fanout contract |
| `cpp-tck.timestamped-default-region-interaction` | Timestamped default-region interaction delivery carries supplied-empty source-region metadata through a constrained grant and terminal retraction boundary |
| `cpp-tck.timestamped-default-region-interaction-contract` | Standard adapter-backed timestamped default-region interaction delivery contract |
| `cpp-tck.timestamped-default-region-interaction-alternate-advances` | Timestamped default-region interactions are delivered through Flush Queue Request, Time Advance Request Available, and Next Message Request Available with supplied-empty region metadata and grant/query bounds |
| `cpp-tck.timestamped-default-region-interaction-alternate-advances-contract` | Standard adapter-backed timestamped default-region interaction alternate-advance contract |
| `cpp-tck.timestamped-default-region-interaction-mixed-fanout` | Timestamped default-region interaction delivery splits between an immediate and a constrained regional recipient, with delivered-copy Request Retraction and recipient-local callback ordering |
| `cpp-tck.timestamped-default-region-interaction-mixed-fanout-contract` | Standard adapter-backed timestamped default-region interaction mixed-fanout contract |
| `cpp-tck.timestamped-default-region-interaction-source-resignation` | A queued timestamped default-region interaction remains deliverable to each constrained regional recipient after producer resignation, preserving supplied-empty region, producer, payload, time, order, and retraction metadata |
| `cpp-tck.timestamped-default-region-interaction-source-resignation-contract` | Standard adapter-backed timestamped default-region interaction source-resignation contract |
| `cpp-tck.timestamped-default-region-interaction-reenable` | Queued timestamped default-region interaction survives Time Constrained disable/re-enable, with supplied-empty source metadata, callback-before-grant ordering, and terminal retraction |
| `cpp-tck.timestamped-default-region-interaction-reenable-contract` | Standard adapter-backed timestamped default-region interaction Time Constrained re-enable contract |
| `cpp-tck.timestamped-default-region-interaction-regulation-reenable` | Queued timestamped default-region interaction survives Time Regulation disable/re-enable with changed lookahead, Query Lookahead, supplied-empty source metadata, and terminal retraction |
| `cpp-tck.timestamped-default-region-interaction-regulation-reenable-contract` | Standard adapter-backed timestamped default-region interaction Time Regulation re-enable contract |
| `cpp-tck.timestamped-object-deletion` | Timestamped Delete/Remove Object Instance, unknown-object boundary, object identity removal and reconstitution, timestamp/order metadata, and deletion retraction |
| `cpp-tck.timestamped-object-deletion-contract` | Standard adapter-backed timestamped ordinary object-deletion contract for Delete/Remove, retraction, identity, time-role servicing, and invalid-object boundaries |
| `cpp-tck.timestamped-object-deletion-no-fanout-contract` | Standard adapter-backed no-recipient timestamped object-deletion contract for exact-boundary retraction, name/identity restoration, ownership restoration, and no retraction fan-out |
| `cpp-tck.timestamped-object-deletion-tombstone-contract` | Standard adapter-backed timestamped object-deletion tombstone contract for terminal deletion, non-retractability, named registration reuse, and identity boundaries |
| `cpp-tck.timestamped-object-deletion-regulation-reenable-contract` | Standard adapter-backed timestamped object-deletion Time Regulation re-enable contract for changed lookahead, removal-before-grant ordering, metadata, identity cleanup, and terminal retraction |
| `cpp-tck.timestamped-object-deletion-source-resignation-fanout-contract` | Standard adapter-backed timestamped object-deletion source-resignation fan-out contract for independent recipient grants, removal metadata, and retraction boundaries |
| `cpp-tck.timestamped-object-deletion-retraction-joined-owners-contract` | Standard adapter-backed timestamped object-deletion joined-owner retraction contract for ownership cleanup, departed-recipient suppression, identity restoration, and retraction boundaries |
| `cpp-tck.timestamped-object-deletion-mixed-advances-contract` | Standard adapter-backed timestamped object-deletion contract for Flush Queue, Time Advance Request Available, and Next Message Request Available delivery paths |
| `cpp-tck.timestamped-local-delete-object` | Local deletion suppresses one recipient's queued timestamped Remove Object Instance callback while an independent constrained recipient receives the original removal and grant |
| `cpp-tck.timestamped-local-delete-object-contract` | Standard adapter-backed timestamped local object-deletion contract for local suppression, surviving fan-out, removal metadata, and retraction |
| `cpp-tck.timestamped-local-delete-attribute` | Local deletion suppresses one recipient's queued timestamped attribute reflection, while re-subscription restores discovery and a later update reaches both recipients |
| `cpp-tck.timestamped-local-delete-attribute-contract` | Standard adapter-backed timestamped local attribute-deletion contract for local suppression, surviving reflection, metadata, and retraction |
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
| `cpp-tck.directed-interaction-multi-recipient-fifo` | Two active universal subscribers receive two ordinary targeted interactions in producer order, preserving target, parameter, tag, producer, and transportation metadata while excluding the sender |
| `cpp-tck.directed-interaction-multi-recipient-fifo-contract` | Standard adapter-backed directed-interaction fan-out and receive-order FIFO contract across two universal subscribers |
| `cpp-tck.directed-interaction-mixed-subscription-fanout` | One by-ownership subscriber and two universal subscribers receive the shared target's ordinary directed interactions, while only the universal subscribers receive a second target |
| `cpp-tck.directed-interaction-mixed-subscription-fanout-contract` | Standard adapter-backed mixed directed-interaction subscription fan-out contract with target, parameter, tag, producer, transportation, and filtering metadata |
| `cpp-tck.directed-interaction-subscription-kind-contract` | Standard adapter-backed directed-interaction subscription-kind contract for by-ownership/universal declarations, target routing, delivery metadata, and cleanup |
| `cpp-tck.timestamped-directed-interactions` | Timestamped directed Send/Receive, invalid class/target/retraction-handle boundaries, selective and universal target routing, constrained grant delivery, timestamp/order metadata, and retraction lifecycle |
| `cpp-tck.timestamped-directed-interactions-contract` | Standard adapter-backed timestamped directed-interaction and retraction contract using official target-routing, time-role, and callback APIs |
| `cpp-tck.timestamped-directed-interaction-retraction` | Retract a queued timestamped directed interaction before delivery and preserve subsequent delivery |
| `cpp-tck.timestamped-directed-interaction-retraction-contract` | Pure standard C++ contract for timestamped directed-interaction retraction, delivery metadata, and terminal retraction |
| `cpp-tck.timestamped-directed-interaction-retraction-fanout` | Retract one queued timestamped directed interaction across multiple constrained recipients, then preserve later directed delivery |
| `cpp-tck.timestamped-directed-interaction-retraction-fanout-contract` | Pure standard C++ contract for timestamped directed-interaction retraction fan-out and the delivered-handle terminal boundary |
| `cpp-tck.region-lifecycle` | Pre-connect and pre-join region and region-qualified service boundaries, including timestamped regional send, two-dimensional region creation, dimension metadata and bounds, commit/query, and invalid/in-use/delete boundaries |
| `cpp-tck.region-lifecycle-contract` | Standard adapter-backed region and dimension lifecycle contract for region-qualified service boundaries, metadata and bounds, range commit/query, validation, and deletion |
| `cpp-tck.regional-unpublish-region-release` | Region-qualified object registration keeps a region in use until unpublishing the associated attribute releases it synchronously |
| `cpp-tck.regional-unpublish-region-release-contract` | Standard adapter-backed regional publication and region-release dependency contract, including synchronous region deletion after unpublication |
| `cpp-tck.regional-object-update` | Region-qualified publication/subscription, named registration, regional discovery and Update/Reflect, regional value requests, and region association changes |
| `cpp-tck.regional-object-update-contract` | Standard adapter-backed regional object publication, subscription, discovery, Update/Reflect, value-request, and region-reassociation contract |
| `java-tck.ddm` | Java-parity view of regional object declaration/association, value requests, Update/Reflect, discovery, and adapter-supplied DDM filtering |
| `cpp-tck.regional-attribute-value-request-filtering` | Regional Request Attribute Value Update filtering across foreign, incompatible, uncommitted, empty, disjoint, overlapping, and callback-time moved regions |
| `cpp-tck.regional-attribute-value-request-filtering-contract` | Standard adapter-backed regional Request Attribute Value Update filtering contract, including validation, source-region scope, default-region eligibility, and callback-time re-evaluation |
| `cpp-tck.regional-attribute-value-update-response-recheck` | Rechecks current regional overlap when a provider's attribute-value response is reflected, then verifies restored-overlap value, tag, transport, and producer metadata |
| `cpp-tck.regional-attribute-value-update-response-recheck-contract` | Standard adapter-backed regional attribute-value response eligibility and restored reflection metadata contract |
| `cpp-tck.regional-attribute-update-callback-ddm-recheck` | Rechecks direct ordinary regional Update/Reflect eligibility after a callback-time region mutation, suppresses disjoint delivery, and verifies restored overlap |
| `cpp-tck.regional-attribute-update-callback-ddm-recheck-contract` | Standard adapter-backed callback-time regional attribute-update DDM recheck contract using only official C++ APIs and adapter-owned configuration |
| `cpp-tck.default-region-object-routing` | Ordinary and explicit regional subscriptions, default-source discovery/reflection, association replacement/restoration, and supplied-empty default-region metadata |
| `cpp-tck.default-region-object-routing-contract` | Standard adapter-backed ordinary/default-region object routing and association replacement contract |
| `cpp-tck.default-region-registration-names` | Default-source registration through the regional and ordinary overloads, generated-name identity round trips, regional discovery, and supplied-empty reflection metadata |
| `cpp-tck.default-region-registration-names-contract` | Standard adapter-backed default-region registration and generated-name contract |
| `cpp-tck.passive-regional-subscription` | Passive regional subscription suppression, activation-triggered discovery, and ordinary regional Update/Reflect with conveyed source-region metadata |
| `cpp-tck.passive-regional-subscription-contract` | Standard adapter-backed passive regional subscription suppression and activation contract |
| `cpp-tck.passive-regional-interaction-transition` | Active and passive regional interaction subscription transitions, retained passive delivery while another route is active, last-active-route suppression, empty-region no-op behavior, and reactivation |
| `cpp-tck.passive-regional-interaction-transition-contract` | Standard adapter-backed passive regional interaction transition contract |
| `cpp-tck.auto-provide` | Adapter-supplied Auto Provide FOM, switch verification, provider-owned object discovery, and grouped `provideAttributeValueUpdate` solicitation |
| `cpp-tck.auto-provide-contract` | Standard adapter-backed Auto Provide switch and grouped solicitation contract |
| `cpp-tck.auto-provide-disabled-discovery-only` | Standard switch-disabled Auto Provide boundary: discovery remains visible while provider solicitation remains absent |
| `cpp-tck.auto-provide-disabled-discovery-only-contract` | Pure standard C++ contract for disabled Auto Provide discovery and callback suppression |
| `cpp-tck.auto-provide-disabled-explicit-request` | Standard object-instance and object-class `requestAttributeValueUpdate` overloads still solicit the provider when Auto Provide is disabled |
| `cpp-tck.auto-provide-disabled-explicit-request-contract` | Pure standard C++ contract for explicit attribute-value requests with Auto Provide disabled |
| `cpp-tck.federation-auto-provide-mom-switch` | Standard federation MOM `HLAsetSwitches` false/true/false transitions observed through object discovery and provider callbacks |
| `cpp-tck.federation-auto-provide-mom-switch-contract` | Pure standard C++ contract for federation-wide Auto Provide changes through the standard MOM |
| `cpp-tck.allow-relaxed-ddm` | Adapter-supplied `Allow Relaxed DDM` switch composition, touching-region admission for ordinary regional object updates and interactions, strict positive-gap suppression, and conveyed source-region metadata |
| `cpp-tck.allow-relaxed-ddm-contract` | Pure standard contract for Allow Relaxed DDM switch composition, touching-region admission, and strict positive-gap suppression |
| `cpp-tck.regional-multi-attribute-update` | Adapter-supplied multi-attribute DDM FOM, independent per-attribute source regions, X-only/Y-only filtering, restoration, and conveyed source-region metadata |
| `cpp-tck.regional-multi-attribute-update-contract` | Standard adapter-backed independent regional multi-attribute filtering contract |
| `cpp-tck.regional-three-dimensional-overlap` | Adapter-supplied three-dimensional DDM FOM, complete-overlap discovery/reflection, one-dimension-at-a-time suppression, restoration, and conveyed source-region metadata |
| `cpp-tck.regional-three-dimensional-overlap-contract` | Standard adapter-backed complete three-dimensional regional-overlap contract |
| `cpp-tck.ownership-transfer-regional-update-contract` | Pure standard contract for ownership transfer and regional update association state |
| `cpp-tck.attribute-scope-advisories` | Attribute In/Out Of Scope callbacks, regional source and subscription transitions, switch suppression, and stale evoked-callback handling |
| `cpp-tck.attribute-scope-advisories-contract` | Standard adapter-backed attribute-scope switch, transition, and stale-callback contract |
| `cpp-tck.regional-declaration-relevance-advisories` | Active and passive regional object and interaction subscriptions, with standard start/stop-registration and turn-interactions-on/off advisories |
| `cpp-tck.regional-declaration-relevance-advisories-contract` | Standard adapter-backed regional declaration-relevance advisory contract |
| `cpp-tck.regional-interaction-routing` | Region-qualified ordinary interaction publication/subscription, overlap routing, disjoint suppression, conveyed region designators, and declaration changes |
| `cpp-tck.regional-interaction-routing-contract` | Standard adapter-backed regional interaction routing contract |
| `cpp-tck.default-region-interaction-routing` | Ordinary interaction routing across explicit regional subscriptions and the implicit default region, including disjoint suppression, restoration, and conveyed empty region metadata |
| `cpp-tck.default-region-interaction-routing-contract` | Standard adapter-backed explicit and implicit default-region interaction routing contract |
| `cpp-tck.zero-dimensional-regional-interaction` | Explicit zero-dimensional interaction realizations do not overlap an ordinary/default subscription after an ordinary baseline delivery |
| `cpp-tck.zero-dimensional-regional-interaction-contract` | Standard adapter-backed zero-dimensional regional interaction non-overlap contract |
| `cpp-tck.multi-region-interaction-routing` | Multiple explicit source regions are union-routed without duplicate callbacks, while matching and disjoint subscriptions receive only eligible interactions |
| `cpp-tck.multi-region-interaction-routing-contract` | Standard adapter-backed multi-region interaction routing contract |
| `cpp-tck.regional-interaction-empty-subscription-sets` | Empty regional interaction subscription and unsubscription sets are no-ops, while non-empty unsubscription removes the route |
| `cpp-tck.regional-interaction-empty-subscription-sets-contract` | Standard adapter-backed regional interaction empty subscription-set contract |
| `cpp-tck.regional-interaction-source-region-snapshot` | Send-time source-region capture for queued ordinary regional interactions, disjoint suppression after source mutation, restored-overlap routing, and conveyed source-region metadata |
| `cpp-tck.regional-interaction-source-region-snapshot-contract` | Standard adapter-backed regional interaction source-region snapshot contract |
| `cpp-tck.regional-interaction-subscription-filtering` | Ordinary receive-order regional interaction filtering, explicit empty-region no-op behavior, overlap and disjoint delivery, callback-time subscription movement, conveyed source-region metadata, and standard region failures |
| `cpp-tck.regional-interaction-subscription-filtering-contract` | Standard adapter-backed regional interaction subscription-filtering contract |
| `cpp-tck.regional-interaction-region-validation` | Atomic rejection of region-qualified interaction sends and subscriptions that contain dimensions unavailable to the interaction class, followed by valid delivery |
| `cpp-tck.regional-interaction-region-validation-contract` | Standard adapter-backed regional interaction region-context validation contract |
| `cpp-tck.timestamped-regional-interaction` | Timestamped region-qualified interaction delivery, constrained grants, retraction, timestamp/order metadata, and conveyed region designators |
| `cpp-tck.timestamped-regional-interaction-contract` | Standard adapter-backed timestamped regional interaction delivery and retraction contract |
| `cpp-tck.timestamped-regional-interaction-regulation-reenable` | Queued timestamped regional interaction survives Time Regulation disable/re-enable with changed lookahead, preserving the source-region snapshot, Query Lookahead, grant ordering, metadata, and terminal retraction |
| `cpp-tck.timestamped-regional-interaction-regulation-reenable-contract` | Standard adapter-backed timestamped regional interaction Time Regulation re-enable contract with changed lookahead |
| `cpp-tck.timestamped-regional-interaction-alternate-advances` | Timestamped regional interaction delivery through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with source-region metadata, grant/query bounds, callback ordering, and backward-time failures |
| `cpp-tck.timestamped-regional-interaction-alternate-advances-contract` | Standard adapter-backed timestamped regional interaction alternate-advance contract |
| `cpp-tck.timestamped-regional-interaction-no-overlap` | Timestamped regional interaction retraction handles remain valid and terminalize correctly when the published source region has no overlapping subscriber |
| `cpp-tck.timestamped-regional-interaction-no-overlap-contract` | Standard adapter-backed timestamped regional interaction no-overlap and retraction contract |
| `cpp-tck.timestamped-regional-interaction-subscription-replacement` | Queued timestamped regional interaction is suppressed rather than retargeted when the receiver replaces region A with disjoint region B, then a later source-B interaction is delivered once with replacement region metadata |
| `cpp-tck.timestamped-regional-interaction-subscription-replacement-contract` | Standard adapter-backed timestamped regional interaction subscription-replacement contract |
| `cpp-tck.timestamped-regional-interaction-source-resignation` | A queued timestamped regional interaction remains deliverable after its producer resigns, preserving producer, source-region, payload, time, order, and retraction metadata |
| `cpp-tck.timestamped-regional-interaction-source-resignation-contract` | Standard adapter-backed timestamped regional interaction source-resignation contract |
| `cpp-tck.timestamped-regional-interaction-tar-nmr` | Timestamped regional interactions are delivered before the matching ordinary TAR and NMR grants, with source-region metadata and independent recipient query times |
| `cpp-tck.timestamped-regional-interaction-tar-nmr-contract` | Standard adapter-backed timestamped regional interaction TAR/NMR contract |
| `cpp-tck.regional-boundaries` | Zero-dimensional and partial regions, wrong-context and foreign-region failures, and region-in-use cleanup boundaries |
| `cpp-tck.regional-boundaries-contract` | Pure standard contract for incompatible, incomplete, foreign, and in-use regional DDM inputs |
| `java-tck.synchronization` | Standard synchronization-point registration, announcement, late-join participation, achievement, completion metadata, and invalid-member/duplicate-label boundaries shared with the Java TCK |
| `cpp-tck.synchronization-points` | Pre-connect and pre-join synchronization-service boundaries, global and explicit-set registration, late-join announcement, invalid-member failure, duplicate-label failure, achievement, and federation synchronization completion |
| `cpp-tck.synchronization-point-contract` | Standard adapter-backed federation synchronization-point contract for global and explicit-set registration, announcement, achievement, completion, callback delivery, and lifecycle boundaries |
| `cpp-tck.synchronization-point-explicit-set-late-join` | Reject `synchronizationPointAchieved` by a federate that joins after an explicit set is registered and is not included in that set |
| `cpp-tck.synchronization-point-explicit-set-late-join-contract` | Standard API contract for explicit-set late-join achievement rejection and completion by the included member |
| `cpp-tck.callback-controls` | Callback disable/enable gating around a delivered interaction under both callback models |
| `cpp-tck.callback-controls-contract` | Standard adapter-backed callback enable and disable contract for interaction delivery, callback servicing, and lifecycle cleanup |
| `cpp-tck.callback-controls-attribute-update` | Callback disable/enable gating around ordinary attribute reflection under both callback models |
| `cpp-tck.callback-controls-attribute-update-contract` | Standard adapter-backed callback enable and disable contract for ordinary attribute reflection and callback servicing |
| `cpp-tck.callback-controls-object-removal` | Callback disable/enable gating around ordinary object removal under both callback models |
| `cpp-tck.callback-controls-object-removal-contract` | Standard adapter-backed callback enable and disable contract for ordinary object removal and callback servicing |
| `cpp-tck.callback-controls-object-discovery` | Callback disable/enable gating around ordinary object discovery under both callback models |
| `cpp-tck.callback-controls-object-discovery-contract` | Standard adapter-backed callback enable and disable contract for ordinary object discovery and callback servicing |
| `cpp-tck.callback-controls-object-name-reservation` | Callback disable/enable gating around ordinary object-name reservation under both callback models |
| `cpp-tck.callback-controls-object-name-reservation-contract` | Standard adapter-backed callback enable and disable contract for ordinary object-name reservation and callback servicing |
| `cpp-tck.callback-controls-object-name-reservation-failure` | Callback disable/enable gating around a contended ordinary object-name reservation under both callback models |
| `cpp-tck.callback-controls-object-name-reservation-failure-contract` | Standard adapter-backed callback enable and disable contract for a failed ordinary object-name reservation and callback servicing |
| `cpp-tck.callback-controls-multiple-object-name-reservation` | Callback disable/enable gating around successful and mixed multiple object-name reservations under both callback models |
| `cpp-tck.callback-controls-multiple-object-name-reservation-contract` | Standard adapter-backed callback enable and disable contract for successful and mixed multiple object-name reservations and callback servicing |
| `cpp-tck.callback-controls-declaration-advisories` | Callback disable/enable gating around start/stop registration and interaction turn-on/turn-off advisories under both callback models |
| `cpp-tck.callback-controls-declaration-advisories-contract` | Standard adapter-backed callback enable and disable contract for declaration-relevance advisories and callback servicing |
| `cpp-tck.callback-controls-attribute-relevance-advisories` | Callback disable/enable gating around unnamed and named per-object attribute turn-up/turn-down advisories under both callback models |
| `cpp-tck.callback-controls-attribute-relevance-advisories-contract` | Standard adapter-backed callback enable and disable contract for per-object attribute-relevance advisories and callback servicing |
| `cpp-tck.callback-controls-attribute-value-request` | Callback disable/enable gating around ordinary attribute-value requests under both callback models |
| `cpp-tck.callback-controls-attribute-value-request-contract` | Standard adapter-backed callback enable and disable contract for ordinary attribute-value requests and callback servicing |
| `cpp-tck.callback-controls-auto-provide` | Callback disable/enable gating around the standard Auto Provide callback under both callback models |
| `cpp-tck.callback-controls-auto-provide-contract` | Standard adapter-backed callback enable and disable contract for Auto Provide and callback servicing |
| `cpp-tck.callback-controls-ownership-assumption` | Callback disable/enable gating around the standard ownership-assumption callback and acquisition handoff under both callback models |
| `cpp-tck.callback-controls-ownership-assumption-contract` | Standard adapter-backed callback enable and disable contract for ownership assumption, acquisition, and callback servicing |
| `cpp-tck.callback-controls-ownership-unavailable` | Callback disable/enable gating around the standard ownership-unavailable callback and unchanged ownership state under both callback models |
| `cpp-tck.callback-controls-ownership-unavailable-contract` | Standard adapter-backed callback enable and disable contract for ownership-unavailable denial and callback servicing |
| `cpp-tck.callback-controls-ownership-release-request` | Callback disable/enable gating around the standard ownership-release request callback and acquisition handoff under both callback models |
| `cpp-tck.callback-controls-ownership-release-request-contract` | Standard adapter-backed callback enable and disable contract for ownership release requests, acquisition, and callback servicing |
| `cpp-tck.callback-controls-ownership-acquisition-cancellation` | Callback disable/enable gating around the standard ownership-acquisition cancellation callback and unchanged ownership state under both callback models |
| `cpp-tck.callback-controls-ownership-acquisition-cancellation-contract` | Standard adapter-backed callback enable and disable contract for ownership-acquisition cancellation and callback servicing |
| `cpp-tck.callback-controls-ownership-release-denied` | Callback disable/enable gating around the standard ownership-unavailable denial callback and unchanged ownership state under both callback models |
| `cpp-tck.callback-controls-ownership-release-denied-contract` | Standard adapter-backed callback enable and disable contract for ownership-release denial and callback servicing |
| `cpp-tck.callback-controls-ownership-acquisition-notification` | Callback disable/enable gating around the standard ownership-acquisition notification and ownership handoff under both callback models |
| `cpp-tck.callback-controls-ownership-acquisition-notification-contract` | Standard adapter-backed callback enable and disable contract for ownership-acquisition notification and callback servicing |
| `cpp-tck.callback-controls-ownership-divestiture-confirmation` | Callback disable/enable gating around the standard negotiated-divestiture confirmation callback and cancellation cleanup under both callback models |
| `cpp-tck.callback-controls-ownership-divestiture-confirmation-contract` | Standard adapter-backed callback enable and disable contract for negotiated-divestiture confirmation and callback servicing |
| `cpp-tck.callback-controls-ownership-query` | Callback disable/enable gating around mixed ordinary ownership-query results and the RTI-owned joined-federate MOM query callback under both callback models |
| `cpp-tck.callback-controls-ownership-query-contract` | Standard adapter-backed callback enable and disable contract for ordinary and RTI-owned ownership-query results, MOM discovery, and callback servicing |
| `cpp-tck.callback-controls-synchronization` | Callback disable/enable gating around synchronization-point announcement and Federation Synchronized completion under both callback models |
| `cpp-tck.callback-controls-synchronization-contract` | Standard adapter-backed callback enable and disable contract for synchronization-point callbacks, callback servicing, and completion metadata |
| `cpp-tck.callback-controls-time-advance` | Callback disable/enable gating around time-regulation, time-constrained, and time-advance grant callbacks under both callback models |
| `cpp-tck.callback-controls-time-advance-contract` | Standard adapter-backed callback enable and disable contract for time-advance grants, callback servicing, and time-management cleanup |
| `cpp-tck.callback-controls-save-restore` | Callback disable/enable gating around federation save/restore lifecycle, status, and completion callbacks under both callback models |
| `cpp-tck.callback-controls-save-restore-contract` | Standard adapter-backed callback-control contract for federation save/restore callbacks, status responses, and lifecycle cleanup |
| `cpp-tck.callback-controls-save-restore-failures` | Callback disable/enable gating around standard federation save and restore failure callbacks under both callback models |
| `cpp-tck.callback-controls-save-restore-failures-contract` | Standard adapter-backed callback-control contract for federation save and restore failure reasons, callback servicing, and lifecycle cleanup |
| `cpp-tck.callback-controls-restore-request-failure` | Callback disable/enable gating around standard failed restore requests and their failure callback under both callback models |
| `cpp-tck.callback-controls-restore-request-failure-contract` | Standard adapter-backed callback-control contract for failed restore-request delivery, exact labels, and lifecycle cleanup |
| `cpp-tck.callback-controls-timestamped-attribute-update` | Callback disable/enable gating around timestamped attribute reflection and its time-advance grant under both callback models |
| `cpp-tck.callback-controls-timestamped-attribute-update-contract` | Standard adapter-backed callback-control contract for timestamped attribute reflection, exact delivery metadata, and grant ordering |
| `cpp-tck.callback-controls-timestamped-object-removal` | Callback disable/enable gating around timestamped object removal and its time-advance grant under both callback models |
| `cpp-tck.callback-controls-timestamped-object-removal-contract` | Standard adapter-backed callback-control contract for timestamped object removal, exact removal metadata, and grant ordering |
| `cpp-tck.callback-controls-timestamped-retraction` | Callback disable/enable gating around timestamped interaction retraction callbacks under both callback models |
| `cpp-tck.callback-controls-timestamped-retraction-contract` | Standard adapter-backed callback-control contract for timestamped interaction retraction and exact handle delivery |
| `cpp-tck.callback-controls-transportation` | Callback disable/enable gating around standard attribute and interaction transportation reports and confirmations under both callback models |
| `cpp-tck.callback-controls-transportation-contract` | Standard adapter-backed callback-control contract for transportation reports, confirmations, and committed state |
| `cpp-tck.callback-controls-directed-interaction` | Callback disable/enable gating around standard directed-interaction delivery, target routing, and exact callback metadata under both callback models |
| `cpp-tck.callback-controls-directed-interaction-contract` | Standard adapter-backed callback-control contract for directed-interaction target routing, metadata, suppression, release, and cleanup |
| `cpp-tck.callback-controls-federation-reports` | Callback disable/enable gating around standard federation execution, member, and missing-execution reports under both callback models |
| `cpp-tck.callback-controls-federation-reports-contract` | Standard adapter-backed callback-control contract for federation report content, suppression, release, and cleanup |
| `cpp-tck.callback-controls-attribute-scope-advisories` | Callback disable/enable gating around regional `attributesInScope` and `attributesOutOfScope` advisories under both callback models |
| `cpp-tck.callback-controls-attribute-scope-advisories-contract` | Standard adapter-backed callback-control contract for regional attribute-scope transitions, suppression, release, and cleanup |
| `cpp-tck.callback-controls-flush-queue-grant` | Callback disable/enable gating around queued timestamped interaction delivery and the standard `flushQueueGrant` callback under both callback models |
| `cpp-tck.callback-controls-flush-queue-grant-contract` | Standard adapter-backed callback-control contract for Flush Queue grant ordering, time metadata, suppression, release, and cleanup |
| `cpp-tck.callback-controls-available-time-advance-callbacks` | Callback disable/enable gating around standard Time Advance Request Available and Next Message Request Available delivery under both callback models |
| `cpp-tck.callback-controls-available-time-advance-callbacks-contract` | Standard adapter-backed callback-control contract for available time-advance delivery, interaction-before-grant ordering, time metadata, suppression, release, and cleanup |
| `cpp-tck.callback-controls-time-role-enablement` | Callback disable/enable gating around standard time-regulation and time-constrained enablement callbacks under both callback models |
| `cpp-tck.callback-controls-time-role-enablement-contract` | Standard adapter-backed callback-control contract for time-role enablement, initial-time metadata, suppression, release, and cleanup |
| `cpp-tck.asynchronous-delivery` | Asynchronous-delivery enable/disable boundaries, receive-order callback gating, `evokeCallback`, `evokeMultipleCallbacks`, and time-advance release |
| `cpp-tck.asynchronous-delivery-contract` | Standard adapter-backed asynchronous-delivery and callback-servicing contract for enable/disable, callback gating, explicit servicing, and time-advance release |
| `cpp-tck.federate-asynchronous-delivery-mim-attribute` | Standard-MIM `HLAboolean` state and conditional reflection across Enable/Disable Asynchronous Delivery |
| `cpp-tck.federate-asynchronous-delivery-mim-attribute-contract` | Pure standard contract for asynchronous-delivery MOM values without assuming a default or public getter |
| `cpp-tck.federate-time-constrained-mim-attribute` | Standard-MIM `HLAtimeConstrained` state, conditional reflection, direct request, and the asynchronous enable callback across Enable/Disable Time Constrained |
| `cpp-tck.federate-time-constrained-mim-attribute-contract` | Pure standard contract for time-constrained MOM values without assuming an initial state |
| `cpp-tck.federate-time-regulating-mim-attribute` | Standard-MIM `HLAtimeRegulating` state, conditional reflection, direct request, and the asynchronous enable callback across Enable/Disable Time Regulation |
| `cpp-tck.federate-time-regulating-mim-attribute-contract` | Pure standard contract for time-regulating MOM values with adapter-selected logical-time lookahead |
| `cpp-tck.federate-lookahead-mim-periodic` | Standard-MIM periodic `HLAlookahead` reports decoded as `HLAtimeInterval` before and after standard Modify/Query Lookahead |
| `cpp-tck.federate-lookahead-mim-periodic-contract` | Pure standard contract for `HLAsetTiming` report cadence and periodic lookahead value changes |
| `cpp-tck.federate-logical-time-mim-periodic` | Compare periodic standard-MIM `HLAlogicalTime` reports for a constrained federate with its successful Time Advance Grants and Query Logical Time |
| `cpp-tck.federate-logical-time-mim-periodic-contract` | Pure standard contract for periodic logical-time reports across two grants, using the adapter-selected logical-time factory |
| `cpp-tck.federation-save-restore` | Pre-connect and pre-join save/restore-service boundaries, including timestamped save request, untimed federation save/restore lifecycle, status responses, completion and failure boundaries, abort, and post-restore handle rebinding |
| `cpp-tck.federation-save-restore-contract` | Standard adapter-backed untimed federation save/restore contract for admission, lifecycle, status, completion/failure, abort, restore callbacks, handle rebinding, and lifecycle boundaries |
| `cpp-tck.federation-restore-abort` | Start a standard federation restore, abort it, and verify `RESTORE_ABORTED` callbacks and terminal status under both callback models |
| `cpp-tck.federation-restore-abort-contract` | Pure standard C++ contract for the save prerequisite, restore initiation, abort service, terminal failure callbacks, status, and cleanup |
| `cpp-tck.federation-restore-work-item-ownership-assumption` | Preserve a pending standard ownership-assumption work item across federation save/restore under both callback models |
| `cpp-tck.federation-restore-work-item-ownership-assumption-contract` | Pure standard C++ contract for restored ownership-assumption delivery, callback gating, metadata, and non-premature ownership transfer |
| `java-tck.save-restore` | Java-parity view of standard federation save/restore admission, status, completion/failure, abort, restore callbacks, and handle rebinding |
| `cpp-tck.federation-save-restore-interlocks` | Representative declaration, object, interaction, ownership, time, DDM, synchronization, and advisory services rejected with `SaveInProgress` and `RestoreInProgress` |
| `cpp-tck.federation-save-restore-interlocks-contract` | Standard adapter-backed contract for representative declaration, object, interaction, ownership, time, DDM, synchronization, and advisory service interlocks during federation save and restore |
| `cpp-tck.timed-federation-save-restore` | Timestamped federation save/restore, queued timestamped interaction recovery, Flush Queue delivery, and retraction |
| `cpp-tck.timed-federation-save-restore-contract` | Standard adapter-backed contract for timestamped federation save/restore, queued interaction recovery, Flush Queue delivery, and retraction |
| `cpp-tck.timed-regional-interaction-save-restore` | Timestamped save/restore of a queued regional interaction with source-region metadata and retraction state |
| `cpp-tck.timed-regional-interaction-save-restore-contract` | Standard adapter-backed contract for timestamped regional interaction save/restore, conveyed-region metadata, Flush Queue delivery, and retraction |
| `cpp-tck.timed-default-region-interaction-save-restore` | Timestamped save/restore of a queued default-region interaction, including empty source-region metadata and retraction state |
| `cpp-tck.timed-default-region-interaction-save-restore-contract` | Standard adapter-backed contract for timestamped default-region interaction save/restore and empty source-region metadata |
| `cpp-tck.timed-default-region-attribute-save-restore` | Timestamped save/restore of a queued default-region attribute update, including empty source-region metadata, Flush Queue delivery, and retraction state |
| `cpp-tck.timed-default-region-attribute-save-restore-contract` | Standard adapter-backed contract for timestamped default-region attribute save/restore, reflection metadata, Flush Queue delivery, and retraction |
| `java-tck.transport-order` | Receive/timestamp order controls, default and per-instance order and transport controls, request/confirmation boundaries, invalid class/object/attribute/transport boundaries, transport queries, and delivered transport identity |
| `cpp-tck.transport-order` | Independently selectable portable ordinary order and transportation controls, invalid-handle boundaries, transport queries, and delivered transport identity |
| `cpp-tck.transport-order-contract` | Standard adapter-backed ordinary order and transportation contract for default and per-instance controls, queries, invalid-handle boundaries, and delivered transport identity |
| `cpp-tck.transportation-type-change` | Ordinary attribute and interaction transportation-type changes, pending versus immediate confirmations, before/after delivery, and committed query reports |
| `cpp-tck.transportation-type-change-contract` | Pure standard C++ contract for ordinary attribute and interaction transportation-type changes and confirmation/report callbacks |
| `java-tck.relevance-advisories` | Pre-connect and pre-join support-switch accessor boundaries, advisory/support-switch state, active/passive declaration relevance, registration and interaction turn-on/turn-off callbacks, named update-rate callbacks, and active per-attribute update-rate queries |
| `cpp-tck.relevance-advisories-contract` | Standard adapter-backed advisory and support-switch contract for lifecycle accessors, declaration relevance, registration/interaction callbacks, and update-rate queries |
| `cpp-tck.attribute-relevance-known-class` | Adapter-FOM-driven known-class policy, inherited object discovery, known-attribute turn-up/turn-down advisories, and suppression of derived-only declarations |
| `cpp-tck.attribute-relevance-known-class-contract` | Standard adapter-backed contract for known-class filtering of inherited attribute-relevance advisories |
| `cpp-tck.attribute-relevance-known-class-disabled` | Adapter-FOM-driven disabled known-class policy, inherited object discovery, and subscription-driven advisories for both known and derived-only attributes |
| `cpp-tck.attribute-relevance-known-class-disabled-contract` | Standard adapter-backed contract for subscription-driven advisories with known-class policy disabled |
| `cpp-tck.delay-subscription-evaluation-interaction` | Standard Delay Subscription Evaluation switch composition, ordinary interaction retention across a late subscription, callback-boundary subscription rechecking, and suppression after unsubscribe under both callback models |
| `cpp-tck.delay-subscription-evaluation-directed-interaction` | Standard Delay Subscription Evaluation switch composition, directed interaction retention across a late target subscription, callback-boundary selector rechecking, and suppression after unsubscribe under both callback models |
| `cpp-tck.delay-subscription-evaluation-attribute-update` | Standard Delay Subscription Evaluation switch composition, known-object attribute-update retention across a late declaration, callback-boundary subscription rechecking, and suppression after unsubscribe under both callback models |
| `cpp-tck.delay-subscription-evaluation-timestamped-interaction` | Standard Delay Subscription Evaluation switch composition, timestamped interaction retention until a time-constrained grant, timestamp/order/retraction metadata, and suppression after unsubscribe at the next grant |
| `cpp-tck.delay-subscription-evaluation-interaction-contract` | Pure standard C++ contract twin for ordinary interaction subscription re-evaluation at the callback boundary |
| `cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction` | Standard Delay Subscription Evaluation switch composition, timestamped directed-interaction retention until a time-constrained grant, target/order metadata, and suppression after unsubscribe at the next grant |
| `cpp-tck.delay-subscription-evaluation-directed-interaction-contract` | Pure standard C++ contract twin for directed interaction subscription re-evaluation at the callback boundary |
| `cpp-tck.delay-subscription-evaluation-timestamped-attribute-update` | Standard Delay Subscription Evaluation switch composition, timestamped attribute-update retention until a time-constrained grant, timestamp/order/retraction metadata, and suppression after unsubscribe at the next grant |
| `cpp-tck.delay-subscription-evaluation-timestamped-regional-attribute-update` | Timestamped regional attribute re-evaluation at the constrained grant, including source-region metadata and suppression after unsubscribe |
| `cpp-tck.delay-subscription-evaluation-timestamped-regional-interaction` | Timestamped regional interaction re-evaluation at the constrained grant, including source-region metadata and suppression after unsubscribe |
| `cpp-tck.delay-subscription-evaluation-attribute-update-contract` | Pure standard C++ contract twin for ordinary attribute subscription re-evaluation at the callback boundary |
| `cpp-tck.delay-subscription-evaluation-timestamped-interaction-contract` | Pure standard C++ contract twin for timestamped interaction subscription re-evaluation at the time-constrained grant boundary |
| `cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction-contract` | Pure standard C++ contract twin for timestamped directed interaction subscription re-evaluation at the time-constrained grant boundary |
| `cpp-tck.delay-subscription-evaluation-timestamped-attribute-update-contract` | Pure standard C++ contract twin for timestamped attribute subscription re-evaluation at the time-constrained grant boundary |
| `cpp-tck.delay-subscription-evaluation-timestamped-regional-attribute-update-contract` | Pure standard C++ contract twin for timestamped regional attribute subscription re-evaluation |
| `cpp-tck.delay-subscription-evaluation-timestamped-regional-interaction-contract` | Pure standard C++ contract twin for timestamped regional interaction subscription re-evaluation |
| `cpp-tck.update-rate-queries` | Named-rate lookup, active/passive/default subscription effects, unsubscribe reset, per-federate isolation, and invalid rate/object/attribute boundaries |
| `cpp-tck.update-rate-queries-contract` | Standard adapter-backed update-rate query contract |
| `cpp-tck.federation-teardown-isolation` | Two similarly named live executions keep independent named update-rate admission history when one execution is resigned and destroyed |
| `cpp-tck.federation-teardown-isolation-contract` | Standard adapter-backed contract for update-rate isolation across federation teardown |
| `cpp-tck.mixed-update-rate-subscriptions` | Ordinary mixed-rate attribute delivery: an active named best-effort subscription is reduced while a default-rate reliable attribute remains deliverable, with transport-aware callback aggregation |
| `cpp-tck.mixed-update-rate-subscriptions-contract` | Standard adapter-backed contract for independent named and default update-rate subscriptions |
| `cpp-tck.attribute-relevance-rate-reissue` | Ordinary Attribute Relevance Advisory rate transitions: a lower-rate peer is silent, active-maximum changes reissue Turn Updates On with the new designator, peer removal refreshes the surviving rate, and the final unsubscribe turns updates off |
| `cpp-tck.attribute-relevance-rate-reissue-contract` | Standard adapter-backed contract for update-rate-driven Attribute Relevance Advisory reissue and final turn-off |
| `cpp-tck.timestamped-attribute-update-rate-reduction` | Adapter-FOM-driven timestamped rate reduction: reliable attributes remain deliverable, an active named best-effort subscription suppresses excess passels, and suppressed retraction handles reach the standard terminal boundary |
| `cpp-tck.timestamped-attribute-update-rate-reduction-contract` | Standard adapter-backed contract for timestamped update-rate reduction and retraction |
| `cpp-tck.handle-wire-formats` | Dimension, region, and message-retraction handle encoding/decoding, direct-buffer and `VariableLengthData&` parity, encoded-length and truncated-buffer checks, copied-handle value semantics, and DDM/timestamped-service boundaries |
| `cpp-tck.handle-wire-formats-contract` | Standard adapter-backed handle encoding and decoding contract |
| `cpp-tck.public-handle-decoding` | Every official federation-scoped handle decoder: lifecycle boundaries, live handle round trips, timestamped message-retraction decoding, and malformed-input rejection |
| `cpp-tck.public-handle-decoding-contract` | Standard adapter-backed public handle-decoding contract |
| `java-tck.ownership` | Pre-connect and pre-join ownership-service boundaries, ownership queries (including unowned reports), invalid object/attribute boundaries, assumption offers, negotiated and If Wanted acquisition/divestiture, If Available acquisition/unavailability, unconditional divestiture, denial, and cancellation |
| `cpp-tck.ownership-management-contract` | Standard adapter-backed ordinary attribute-ownership contract for queries, assumption offers, negotiated and If Wanted acquisition/divestiture, If Available acquisition, unconditional divestiture, denial, cancellation, callbacks, and lifecycle boundaries |
| `cpp-tck.ownership-service-boundaries` | Portable standard ownership-service slice covering queries, acquisition, divestiture, cancellation, callback boundaries, and resignation cleanup |
| `cpp-tck.ownership-service-boundaries-contract` | Pure standard C++ contract for ordinary ownership-service boundaries |
| `cpp-tck.partial-attribute-ownership-transfer` | Multi-attribute ownership acquisition and cancellation with an If Wanted transfer of only the uncanceled attribute, including release, cancellation, acquisition, tag, and ownership-state assertions |
| `cpp-tck.partial-attribute-ownership-transfer-contract` | Standard adapter-backed partial attribute ownership-transfer contract for acquisition cancellation, Divestiture If Wanted, ownership callbacks, and lifecycle cleanup |
| `cpp-tck.ownership-acquisition-publication-fence` | Publication, pending-state, and partial-result boundaries for regular and If Available ownership acquisition, including direct acquisition of an unowned attribute, release denial, and callback metadata |
| `cpp-tck.ownership-acquisition-publication-fence-contract` | Standard adapter-backed ownership-acquisition publication-fence contract using only the official API and adapter-supplied multi-attribute FOM |
| `cpp-tck.ownership-acquisition-if-available` | Dedicated If Available ownership-acquisition unavailable callback and post-divestiture transfer boundary |
| `cpp-tck.ownership-acquisition-if-available-contract` | Pure standard C++ contract for If Available acquisition, unavailable metadata, ownership transfer, and lifecycle cleanup |
| `cpp-tck.attribute-ownership-acquisition-cancellation` | Cancel a pending ordinary attribute ownership acquisition without transferring ownership |
| `cpp-tck.attribute-ownership-acquisition-cancellation-contract` | Pure standard C++ contract for ordinary attribute ownership-acquisition cancellation, confirmation metadata, and stable ownership |
| `cpp-tck.attribute-ownership-release-denied` | Deny a pending ordinary ownership-acquisition request and verify the unavailable callback, exact tags, and unchanged ownership |
| `cpp-tck.attribute-ownership-release-denied-contract` | Pure standard C++ contract for ownership-release denial and unavailable notification |
| `cpp-tck.attribute-ownership-divestiture-if-wanted` | Complete a one-attribute pending regular acquisition through Divestiture If Wanted and verify its returned set, callback tag, and ownership transfer |
| `cpp-tck.attribute-ownership-divestiture-if-wanted-contract` | Pure standard C++ contract for Divestiture If Wanted and its acquisition handoff |
| `cpp-tck.attribute-ownership-query-owner-report` | Query one known owned attribute and verify the standard owner handle in Inform Attribute Ownership |
| `cpp-tck.attribute-ownership-query-owner-report-contract` | Pure standard C++ contract for the owner-report path of Attribute Ownership Query |
| `cpp-tck.attribute-ownership-query-unowned-result` | Query a mixed-ownership object and verify the owned and not-owned callback partitions |
| `cpp-tck.attribute-ownership-query-unowned-result-contract` | Pure standard C++ contract for the ordinary owned and not-owned query-result partitions |
| `cpp-tck.attribute-ownership-query-rti-owned-result` | Query the standard joined-federate MIM handle attribute and verify the RTI-owned callback |
| `cpp-tck.attribute-ownership-query-rti-owned-result-contract` | Pure standard C++ contract for the RTI-owned ownership-query result |
| `cpp-tck.unconditional-attribute-ownership-divestiture` | Standard unconditional divestiture with regular acquisition, initial If Available rejection, ownership-assumption eligibility, exact tags, and post-divestiture If Available retry |
| `cpp-tck.unconditional-attribute-ownership-divestiture-contract` | Pure standard C++ contract for multi-recipient unconditional attribute ownership divestiture |
| `cpp-tck.ownership-query-partition-cleanup` | Partition mixed owned/unowned attribute-query results, reject invalid query handles, and suppress stale query results after object removal |
| `cpp-tck.ownership-query-partition-cleanup-contract` | Standard adapter-backed ownership-query partition and cleanup contract using only the official API and adapter-supplied multi-attribute FOM |
| `cpp-tck.ownership-query-save-restore-results` | Restore the saved federate owner after a post-save negotiated transfer, then distinguish federate-owned, unowned, and RTI-owned query results |
| `cpp-tck.ownership-query-save-restore-results-contract` | Pure standard API contract twin for all three ownership-query result kinds after federation restore |
| `cpp-tck.divestiture-if-wanted-mixed-acquirers` | Transfers independently pending attributes to mixed regular and If Available acquirers with Divestiture If Wanted; immediate mode uses regular pending acquisition to preserve the standard pending boundary |
| `cpp-tck.divestiture-if-wanted-mixed-acquirers-contract` | Standard adapter-backed mixed-acquirer Divestiture If Wanted contract with exact attribute-set transfer, callback tags, ownership state, and cleanup |
| `cpp-tck.ownership-acquisition-cancellation-transfer-race` | Standard two-model ownership transfer: evoked cancellation begins from the owner release callback and Divestiture If Wanted wins the terminal race; immediate mode verifies the same transfer through a deterministic pending acquisition |
| `cpp-tck.ownership-acquisition-cancellation-transfer-race-contract` | Pure standard C++ contract for acquisition cancellation and Divestiture If Wanted transfer under both callback models; the adapter owns provider, FOM, endpoint, and callback configuration |
| `cpp-tck.negotiated-divestiture-partial-acquisition-cancellation` | Cancels one attribute of a negotiated multi-attribute divestiture, confirms only the retained attribute, and verifies exact transfer, tags, cancellation, and ownership state under both callback models |
| `cpp-tck.negotiated-divestiture-partial-acquisition-cancellation-contract` | Standard adapter-backed partial negotiated-divestiture cancellation contract for exact retained-attribute transfer, tags, callbacks, ownership state, and cleanup |
| `cpp-tck.negotiated-divestiture-cancellation` | Negotiated divestiture confirmation for a pending acquisition, cancellation back to the ordinary release path, ownership preservation, and acquisition-cancellation confirmation |
| `cpp-tck.negotiated-divestiture-cancellation-contract` | Standard adapter-backed negotiated divestiture cancellation contract |
| `cpp-tck.negotiated-divestiture-confirmation-flow` | Ordinary negotiated divestiture request, acquisition-tagged owner confirmation request, owner confirmation, confirmation-tagged acquisition callback, and before/after ownership queries |
| `cpp-tck.negotiated-divestiture-confirmation-flow-contract` | Pure standard C++ contract for the negotiated-divestiture confirmation and acquisition handoff |
| `cpp-tck.negotiated-divestiture-pre-delivery-cancellation` | Cancels a pending acquisition before evoked negotiated-divestiture callback delivery, suppressing stale owner callbacks and preserving ownership |
| `cpp-tck.negotiated-divestiture-pre-delivery-cancellation-contract` | Standard adapter-backed pre-delivery negotiated cancellation contract |
| `cpp-tck.negotiated-willing-to-acquire-continuation` | Negotiated divestiture continuation to the second candidate after the first candidate cancels, preserving tags and ownership state; evoked mode uses Willing-to-Acquire reservations and immediate mode verifies the standard unavailable boundary with a regular pending continuation |
| `cpp-tck.negotiated-willing-to-acquire-continuation-contract` | Pure standard C++ contract for negotiated willing-to-acquire continuation under both callback models; the adapter owns provider, FOM, endpoint, and callback configuration |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore` | Two-model timed regional ownership continuation: evoked delivery retains the regular candidate through negotiated divestiture, while immediate delivery verifies the standard If Available boundary and synchronous ordinary ownership-release/divestiture after queued delivery |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore-contract` | Pure standard C++ contract for the timed regional ownership continuation; provider, FOM, endpoint, logical-time, and callback configuration remain adapter inputs under both callback models |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore` | Promoted two-model cancellation of a regular negotiated ownership transfer before confirmation callback delivery after timed regional attribute save/restore: evoked mode preserves publisher ownership and cancels the surviving acquisition reservation, while immediate mode verifies the standard unavailable cancellation window and synchronous ordinary ownership handoff |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore-contract` | Promoted pure standard C++ contract for pre-delivery negotiated cancellation after timed regional restore; evoked mode covers the cancellation route and immediate mode covers the standard callback-window boundary, with provider, FOM, endpoint, logical-time, and callback configuration remaining adapter inputs |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore` | Promoted two-model regular-to-regular negotiated ownership cancellation after Request Divestiture Confirmation: evoked mode preserves publisher ownership, rejects stale Confirm Divestiture, and cancels the surviving acquisition reservation, while immediate mode verifies the standard unavailable cancellation window and synchronous ordinary ownership handoff |
| `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore-contract` | Promoted pure standard C++ contract for post-confirmation negotiated cancellation after timed regional restore; evoked mode covers the full cancellation route and immediate mode covers the standard callback-window boundary, with provider, FOM, endpoint, logical-time, and callback configuration remaining adapter inputs |
| `cpp-tck.resign-pending-acquisition-rejection` | Rejects unconditional resignation while ownership acquisition is pending, then cancels that work during standard cancel-then-delete-then-divest resignation |
| `cpp-tck.resign-pending-acquisition-rejection-contract` | Standard adapter-backed pending-acquisition resignation rejection contract |
| `cpp-tck.resign-cancel-pending-acquisition` | Cancels pending ownership-acquisition work during standard resignation and prevents a stale owner-release callback under both callback models |
| `cpp-tck.resign-cancel-pending-acquisition-contract` | Standard adapter-backed pending-acquisition cancellation contract |
| `cpp-tck.resign-cancel-if-available-pending` | Cancels a pending If Available ownership-acquisition reservation during standard resignation and suppresses stale acquisition/unavailable delivery under both callback models |
| `cpp-tck.resign-cancel-if-available-pending-contract` | Standard adapter-backed If Available cancellation contract |
| `cpp-tck.resign-cancel-negotiated-pending` | Cancels a pending negotiated ownership transfer during standard resignation and suppresses stale owner divestiture/release callbacks under both callback models |
| `cpp-tck.resign-cancel-negotiated-pending-contract` | Standard adapter-backed negotiated cancellation contract |
| `cpp-tck.ownership-transfer-regional-update` | Three-federate regional ownership transfer, former-owner update rejection, default-source delivery after transfer, and explicit replacement update-region association |
| `cpp-tck.ownership-transfer-deferred-regional-update` | Non-owner deferred update-region association is promoted at regular ownership transfer, with owner-release, acquisition, ownership, update, and conveyed source-region assertions |
| `cpp-tck.ownership-transfer-deferred-regional-update-contract` | Standard adapter-backed deferred ownership/update-region promotion contract |
| `java-tck.ordinary-edges` | Passive-delivery boundaries, idempotent object/interaction declarations, unsubscribed delivery suppression, and standard invalid-class/object/attribute/publication/parameter failures |
| `cpp-tck.ordinary-edges-contract` | Standard adapter-backed ordinary delivery, declaration, unsubscription, unpublication, and negative-boundary contract |
| `cpp-tck.fom-model` | Rich valid FOM hierarchy, inheritance, dimensions, update rates, transportation, advisory switches, declarations, and representative typed delivery |
| `cpp-tck.fom-model-contract` | Standard adapter-backed FOM model contract for hierarchy, inheritance, dimensions, update rates, transportation, declarations, and representative typed delivery |
| `cpp-tck.inherited-object-attribute-projection` | Derived object discovery through base and derived subscriptions, base-only inherited-attribute projection, derived-attribute retention, and ordinary reflection metadata |
| `cpp-tck.inherited-object-attribute-projection-contract` | Standard adapter-backed inherited object-attribute projection contract |
| `cpp-tck.fom-empty-module-validation-contract` | Standard adapter-backed empty-module validation contract for valid and invalid FOM module boundaries |
| `cpp-tck.custom-transportation-interaction-delivery` | Adapter-declared custom transportation lookup, ordinary interaction publication/subscription/send delivery, received transportation identity, and standard transportation query reporting |
| `cpp-tck.custom-transportation-interaction-delivery-contract` | Standard adapter-backed contract for custom FOM transportation lookup, ordinary interaction delivery, received transportation identity, and transportation query reporting |
| `cpp-tck.custom-transportation-attribute-delivery` | Adapter-declared custom transportation with ordinary object publication/subscription, registration/discovery, attribute update/reflection, name round-trips, and transportation identity |
| `cpp-tck.custom-transportation-attribute-delivery-contract` | Standard adapter-backed contract for ordinary custom FOM transportation attribute delivery and discovery/reflection metadata |
| `cpp-tck.custom-transportation-timestamped-attribute-delivery` | Adapter-declared custom transportation with timestamped ordinary object publication/subscription, registration/discovery, Update/Reflect delivery, time/order/retraction metadata, and transportation identity |
| `cpp-tck.custom-transportation-timestamped-attribute-delivery-contract` | Standard adapter-backed contract for timestamped custom FOM transportation attribute delivery, constrained grants, retraction, and reflection metadata |
| `cpp-tck.custom-transportation-timestamped-attribute-alternate-advances` | Adapter-declared custom transportation through timestamped ordinary attribute delivery with Flush Queue, Time Advance Request Available, and Next Message Request Available |
| `cpp-tck.custom-transportation-timestamped-attribute-alternate-advances-contract` | Standard adapter-backed contract for custom transportation through timestamped ordinary attribute alternate advances, grant boundaries, and retraction |
| `cpp-tck.custom-transportation-timestamped-interaction-alternate-advances` | Adapter-declared custom transportation through timestamped ordinary interaction delivery with Flush Queue, Time Advance Request Available, and Next Message Request Available |
| `cpp-tck.custom-transportation-timestamped-interaction-alternate-advances-contract` | Standard adapter-backed contract for custom transportation through timestamped ordinary interaction alternate advances, grant boundaries, and retraction |
| `cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances` | Adapter-declared custom transportation through timestamped directed-interaction delivery to a registered target with Flush Queue, Time Advance Request Available, and Next Message Request Available |
| `cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances-contract` | Standard adapter-backed contract for custom transportation through timestamped directed-interaction alternate advances, target identity, grant boundaries, and retraction |
| `cpp-tck.custom-transportation-regional-attribute-delivery` | Adapter-declared custom transportation with ordinary regional attribute publication/subscription/update delivery, conveyed source-region metadata, overlap filtering, and both callback models |
| `cpp-tck.custom-transportation-regional-attribute-delivery-contract` | Standard adapter-backed contract for custom FOM transportation regional attribute delivery, region metadata, overlap filtering, and value-update requests |
| `cpp-tck.custom-transportation-regional-interaction-delivery` | Adapter-declared custom transportation with ordinary regional interaction publication/subscription/send delivery, conveyed source-region metadata, parameter delivery, and both callback models |
| `cpp-tck.custom-transportation-regional-interaction-delivery-contract` | Standard adapter-backed contract for custom FOM transportation regional interaction delivery, region metadata, overlap filtering, and parameter/tag identity |
| `cpp-tck.custom-transportation-timestamped-delivery` | Adapter-declared custom transportation with timestamped interaction publication/subscription/send delivery, constrained grants, payload/tag/time/order/retraction metadata, and no region metadata |
| `cpp-tck.custom-transportation-timestamped-delivery-contract` | Standard adapter-backed contract for timestamped custom FOM transportation interaction delivery, constrained grants, retraction, and transportation metadata |
| `cpp-tck.custom-transportation-timestamped-directed-delivery` | Adapter-declared custom transportation with timestamped directed-interaction publication/subscription/send delivery to a registered target, constrained grants, payload/tag/target/time/order/retraction metadata, and no region metadata |
| `cpp-tck.custom-transportation-timestamped-directed-delivery-contract` | Standard adapter-backed contract for timestamped custom FOM transportation directed-interaction delivery, target discovery, constrained grants, retraction, and transportation metadata |
| `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery` | Adapter-declared custom transportation with timestamped regional attribute publication/subscription/update delivery, region metadata, retraction, DDM overlap, and both callback models |
| `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery-contract` | Standard adapter-backed contract for timestamped custom FOM transportation regional attribute delivery, region metadata, overlap filtering, retraction, and value-update requests |
| `cpp-tck.custom-transportation-timestamped-regional-interaction-delivery` | Adapter-declared custom transportation with timestamped regional interaction publication/subscription/delivery, region metadata, retraction, DDM overlap, and both callback models |
| `cpp-tck.custom-transportation-timestamped-regional-interaction-delivery-contract` | Standard adapter-backed contract for timestamped custom FOM transportation regional interaction delivery, region metadata, overlap filtering, retraction, and time-advance grants |
| `cpp-tck.fom-module-composition` | Create-time FOM module composition and join-time module addition with shared declaration handles |
| `cpp-tck.fom-module-composition-contract` | Standard adapter-backed FOM module-composition contract for create-time and join-time module addition with shared declaration handles |
| `cpp-tck.fom-additional-module-join-atomicity` | Reject invalid additional FOM modules without mutating membership or declarations, then compose shared handles on a valid follow-up join |
| `cpp-tck.fom-additional-module-join-atomicity-contract` | Standard adapter-backed atomic additional-FOM join rejection contract |
| `cpp-tck.fom-transportation-handle-stability` | Preserve existing and newly composed transportation handle identity across an additional-FOM join |
| `cpp-tck.fom-transportation-handle-stability-contract` | Standard adapter-backed transportation handle/name stability contract for additional-FOM composition |
| `cpp-tck.fom-dimension-handle-stability` | Preserve dimension handle identity, metadata, and object/interaction class associations across an additional-FOM join; the adapter supplies the rich and additional FOM names |
| `cpp-tck.fom-dimension-handle-stability-contract` | Standard adapter-backed dimension handle/name, upper-bound, and class-association contract for additional-FOM composition |
| `cpp-tck.fom-update-rate-value-stability` | Preserve a rich-FOM update-rate value and compose a new adapter-supplied update-rate designator across an additional-FOM join |
| `cpp-tck.fom-update-rate-value-stability-contract` | Standard adapter-backed update-rate value composition and base-value stability contract for additional-FOM joins |
| `cpp-tck.interaction-class-lookup-lifecycle` | Standard interaction-class handle/name lookup across pre-connect, pre-join, invalid-input, and additional-FOM composition boundaries |
| `cpp-tck.interaction-class-lookup-lifecycle-contract` | Pure standard C++ contract for stable base/extension interaction handles, name round trips, and standard lookup exceptions using adapter-supplied FOM modules |
| `cpp-tck.object-class-lookup-lifecycle` | Standard object-class handle/name lookup across pre-connect, pre-join, invalid-input, and additional-FOM composition boundaries |
| `cpp-tck.object-class-lookup-lifecycle-contract` | Pure standard C++ contract for stable base/extension object-class handles, name round trips, and standard lookup exceptions using adapter-supplied FOM modules |
| `cpp-tck.attribute-lookup-lifecycle` | Standard attribute handle/name lookup across pre-connect, pre-join, inherited object-class definitions, invalid-input, and cross-federate identity boundaries |
| `cpp-tck.attribute-lookup-lifecycle-contract` | Pure standard C++ contract for inherited base/derived attribute handles, name round trips, and standard lookup exceptions using an adapter-supplied rich FOM model |
| `cpp-tck.parameter-lookup-lifecycle` | Standard parameter handle/name lookup across pre-connect, pre-join, inherited interaction-class definitions, invalid-input, and cross-federate identity boundaries |
| `cpp-tck.parameter-lookup-lifecycle-contract` | Pure standard C++ contract for inherited base/derived parameter handles, name round trips, and standard lookup exceptions using an adapter-supplied rich FOM model |
| `cpp-tck.dimension-lookup-lifecycle` | Standard adapter-supplied dimension handle/name/upper-bound and available-dimension lookup across pre-connect, pre-join, invalid-input, resign, and cross-federate identity boundaries |
| `cpp-tck.dimension-lookup-lifecycle-contract` | Pure standard C++ contract for dimensional metadata and class-association lookup using an adapter-supplied dimensional FOM |
| `cpp-tck.dimension-lookup-valid-handles-after-resignation` | After resigning while connected, checks `FederateNotExecutionMember` for dimension name/upper-bound and available object/interaction-class dimensions using handles proven valid in the adapter-supplied FOM (§§10.21–10.25) |
| `cpp-tck.dimension-lookup-valid-handles-after-resignation-contract` | Standard contract twin for the four dimension lookup services after resignation; adapter-supplied dimensional FOM only |
| `cpp-tck.update-rate-lookup-valid-inputs-after-resignation` | After resigning while connected, checks `FederateNotExecutionMember` for valid update-rate designator and object/attribute queries (§§10.11–10.12) |
| `cpp-tck.update-rate-lookup-valid-inputs-after-resignation-contract` | Standard contract twin for both update-rate lookup services after resignation, using adapter-supplied rate FOM and designator |
| `cpp-tck.support-lookup-valid-inputs-after-resignation` | After resigning while connected, checks valid object/interaction class, attribute, and parameter handle/name lookups (§§10.4–10.5, 10.9–10.10, 10.13–10.16) |
| `cpp-tck.support-lookup-valid-inputs-after-resignation-contract` | Standard contract twin for the eight class/member lookup directions using adapter-supplied FOM and names |
| `cpp-tck.support-switch-getter-lifecycle-boundaries` | Calls all 13 standard support-switch getters while joined, then checks `FederateNotExecutionMember` after resignation and `NotConnected` after disconnect; no switch defaults are assumed (§§10.34, 10.36, 10.38, 10.40, 10.42, 10.46, 10.48, 10.50, 10.52–10.56) |
| `cpp-tck.support-switch-getter-lifecycle-boundaries-contract` | Standard contract twin for every boolean support-switch getter lifecycle boundary |
| `cpp-tck.support-switch-setter-lifecycle-boundaries` | Calls all eight writable support-switch setters with each getter's observed value while joined, then checks `FederateNotExecutionMember` after resignation and `NotConnected` after disconnect (§§10.35, 10.37, 10.39, 10.41, 10.43, 10.47, 10.49, 10.51) |
| `cpp-tck.support-switch-setter-lifecycle-boundaries-contract` | Standard contract twin for all writable support-switch setter lifecycle boundaries |
| `cpp-tck.advisory-switch-value-round-trips` | Toggles the four relevance/scope advisory switches from their observed values and verifies each value and restoration (§§10.34–10.41) |
| `cpp-tck.advisory-switch-value-round-trips-contract` | Standard contract twin for relevance/scope advisory switch Boolean behavior |
| `cpp-tck.automatic-resign-directive-lifecycle-boundaries` | Round-trips valid automatic-resign directive values while joined, then checks `FederateNotExecutionMember` after resignation and `NotConnected` after disconnect (§§10.44–10.45) |
| `cpp-tck.automatic-resign-directive-lifecycle-boundaries-contract` | Standard contract twin for automatic-resign directive getter/setter lifecycle boundaries |
| `cpp-tck.automatic-resign-directive-enum-values` | Round-trips all six standard `ResignAction` values and requires `InvalidResignAction` for the next enum value (§10.45) |
| `cpp-tck.automatic-resign-directive-enum-values-contract` | Standard contract twin for automatic-resign directive value validation |
| `cpp-tck.object-attribute-declarations-valid-inputs-after-resignation` | After resigning while connected, checks valid object/attribute publication and subscription declaration services (§§5.2–5.3, 5.8–5.9) |
| `cpp-tck.object-attribute-declarations-valid-inputs-after-resignation-contract` | Standard contract twin for the six object/attribute declaration services using adapter-supplied FOM handles |
| `cpp-tck.interaction-declarations-valid-inputs-after-resignation` | After resigning while connected, checks valid interaction publication and subscription declaration services (§§5.4–5.5, 5.10–5.11) |
| `cpp-tck.interaction-declarations-valid-inputs-after-resignation-contract` | Standard contract twin for interaction publish/unpublish/subscribe/unsubscribe using adapter-supplied FOM handles |
| `cpp-tck.directed-interaction-declarations-valid-inputs-after-resignation` | After resigning while connected, checks valid class-scoped directed-interaction publication/subscription set and whole-class overloads (§§5.6–5.7, 5.12–5.13); excludes region-qualified forms |
| `cpp-tck.directed-interaction-declarations-valid-inputs-after-resignation-contract` | Standard contract twin for all six non-region directed-interaction declaration overloads using an adapter-FOM object-class/interaction-class association |
| `cpp-tck.interaction-transportation-query-valid-inputs-after-resignation` | Checks the standard interaction transportation report for a valid active peer and FOM interaction class, then `FederateNotExecutionMember` after requester resignation (§§6.32–6.33) |
| `cpp-tck.interaction-transportation-query-valid-inputs-after-resignation-contract` | Standard contract twin for the interaction transportation query and report callback, using the adapter-supplied FOM and active peer |
| `cpp-tck.standard-order-transport-lookups-after-resignation` | After resigning while connected, checks mandatory Receive/TimeStamp and HLAreliable/HLAbestEffort lookups with valid names and handles (§§10.17–10.20) |
| `cpp-tck.standard-order-transport-lookups-after-resignation-contract` | Standard contract twin for mandatory order and transportation lookups after resignation |
| `cpp-tck.fom-invalid-create-atomicity` | Reject invalid single-module FOM creates without reserving the federation name, then recover through a valid lifecycle |
| `cpp-tck.fom-invalid-create-atomicity-contract` | Standard adapter-backed invalid-FOM create atomicity contract |
| `cpp-tck.fom-invalid-composite-join-atomicity` | Reject mixed valid and invalid additional FOM modules atomically, then recover through a valid follow-up join |
| `cpp-tck.fom-invalid-composite-join-atomicity-contract` | Standard adapter-backed mixed additional-FOM join atomicity contract |
| `cpp-tck.fom-invalid-mim-create-atomicity` | Reject invalid MIM modules atomically during federation creation, then recover through a valid FOM/MIM lifecycle |
| `cpp-tck.fom-invalid-mim-create-atomicity-contract` | Standard adapter-backed invalid-MIM create atomicity contract |
| `cpp-tck.fom-invalid-composite-mim-create-atomicity` | Reject mixed valid and invalid FOM modules atomically during FOM/MIM federation creation, then recover through a valid lifecycle |
| `cpp-tck.fom-invalid-composite-mim-create-atomicity-contract` | Standard adapter-backed mixed FOM-module and MIM create atomicity contract |
| `cpp-tck.fom-empty-mim-create-atomicity` | Reject an empty FOM module vector atomically during FOM/MIM federation creation, then recover through a valid lifecycle |
| `cpp-tck.fom-empty-mim-create-atomicity-contract` | Standard adapter-backed empty-FOM-vector MIM create atomicity contract |
| `cpp-tck.connection-loss-cleanup` | Adapter-triggered Connection Lost callback, fault description, survivor service, and cleanup |
| `cpp-tck.connection-loss-cleanup-contract` | Pure standard C++ contract for the adapter-managed Connection Lost callback, lost-member cleanup, survivor service, and standard disconnect boundary |
| `cpp-tck.callback-reentrancy` | Callback-service re-entry is rejected while a standard federation-listing callback is executing |
| `cpp-tck.callback-reentrancy-contract` | Pure standard C++ contract for callback-service re-entry rejection under both callback models |

The shared ordinary-service cases reuse the Java TCK scenario IDs. The
federation-list, federate-lookup, object-name-reservation, object-registration-discovery, Allow Relaxed DDM, multi-attribute and three-dimensional regional object update, timestamped interaction, timestamped regional attribute, timestamped object-management, alternate-time,
Next Message Request, Query GALT/LITS, FOM, DDM, synchronization-point,
asynchronous-delivery, save/restore, and connection-loss cases are C++ adapter
extensions. Time-dependent scenarios are skipped when the
adapter does not select a logical-time implementation; connection loss is
skipped when no adapter trigger is supplied.
The promoted `cpp-tck.public-handle-decoding` scenario and its contract twin
exercise all nine official federation-scoped handle decoders, including
timestamped message-retraction handles, lifecycle boundaries, and malformed
encodings. The focused installed-package lane passed 4/4 callback-model cases;
the earlier merged installed-package checkpoint recorded 632 promoted
scenario IDs and 1,264 callback-model cases, including the adapter-managed
connection-loss lane. The current checkpoint is recorded in
`compliance/requirements-lab/cpp-tck-verification-checkpoint-2026-09-18.json`.
The promoted `cpp-tck.custom-transportation-attribute-delivery` scenario and
its pure standard contract twin exercise the ordinary object
publication/subscription, registration/discovery, and Update/Reflect route
with an adapter-declared custom FOM transportation type. The source makes no
explicit DDM or provider calls; the adapter supplies the rich FOM, provider,
endpoint, and callback configuration. Its focused installed-package lane
passed 4/4 callback-model cases; the earlier merged installed-package
checkpoint recorded all 632 promoted scenario IDs (1,264 callback-model cases
when both models were selected).
The promoted `cpp-tck.custom-transportation-timestamped-attribute-delivery`
scenario and its pure standard contract twin exercise timestamped ordinary
object publication/subscription, registration/discovery, Update/Reflect
delivery before and after a constrained time advance, logical-time and order
metadata, retraction callbacks, and adapter-declared transportation identity.
The source makes no explicit DDM or provider calls; the adapter supplies the
rich FOM, logical-time implementation, provider, endpoint, and callback
configuration. Its focused installed-package lane passed 4/4 callback-model
cases; the earlier merged installed-package checkpoint recorded all 632
promoted scenario IDs (1,264 callback-model cases when both models were
selected).
The promoted `cpp-tck.custom-transportation-timestamped-attribute-alternate-advances`
scenario and its pure standard contract twin reuse the verified alternate-time
oracle for Flush Queue, Time Advance Request Available, and Next Message Request
Available, then assert that the adapter-declared custom transportation identity
survives each timestamped ordinary attribute reflection. The source makes no
explicit DDM or provider calls; the adapter supplies the rich FOM, logical-time
implementation, provider, endpoint, and callback configuration. Its focused
installed-package lane passed 4/4 callback-model cases; the earlier verified
installed-package checkpoint recorded all 632 promoted scenario IDs (1,264
callback-model cases when both models were selected).
The promoted `cpp-tck.custom-transportation-timestamped-interaction-alternate-advances`
scenario and its pure standard contract twin reuse the verified alternate-time
oracle for Flush Queue, Time Advance Request Available, and Next Message Request
Available, then assert that the adapter-declared custom transportation identity
survives each timestamped ordinary interaction delivery. The source makes no
explicit DDM or provider calls; the adapter supplies the rich FOM, logical-time
implementation, provider, endpoint, and callback configuration. Its focused
installed-package lane passed 4/4 callback-model cases; the earlier verified
installed-package checkpoint recorded all 632 promoted scenario IDs (1,264
callback-model cases when both models were selected).
The promoted `cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances`
scenario and its pure standard contract twin reuse the verified directed
timestamped-delivery oracle for Flush Queue, Time Advance Request Available, and
Next Message Request Available, then assert target identity, parameter/tag/time/
order metadata, custom transportation identity, and retraction behavior. The
source uses no DDM or provider-specific calls; the adapter supplies the rich FOM,
logical-time implementation, provider, endpoint, and callback configuration. Its
focused installed-package lane passed 4/4 callback-model cases; the earlier
verified installed-package checkpoint recorded all 632 promoted scenario IDs
(1,264 callback-model cases when both models were selected).
The promoted `cpp-tck.custom-transportation-timestamped-regional-interaction-delivery`
scenario and its pure standard contract twin reuse the complete timestamped
regional-interaction oracle, then verify the adapter-declared custom
transportation on the same timestamped `sendInteractionWithRegions` path. The
adapter supplies the rich FOM, logical-time implementation, DDM dimensions,
endpoint, and callback configuration; the source uses only the official C++
API and standard library. The focused installed-package lane passed all 4/4
callback-model cases, and the merged verified installed-package gate passed
all 632 promoted scenario IDs (1,264 callback-model cases when both models
were selected). At the 2026-09-18 checkpoint, the catalog contained 651
promoted IDs and 651 inventory entries; that dated all-catalog
installed-package matrix passed all 1,302 callback-model cases with the
adapter-managed current-process connection-loss fixture and zero failures.
The historical evidence is recorded in
`compliance/requirements-lab/cpp-tck-verification-checkpoint-2026-09-18.json`.
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
adapter fixture with argument lists, runs both callback models independently
for the base scenario and its pure standard contract twin, and records combined evidence in
`.build/cpp-tck-all/connection-loss-current-process.json`. That adapter lane
passed 4/4 cases and is now the promoted adapter-required connection-loss
pair. Other adapters must provide the same fixture contract to run either ID.

The promoted `cpp-tck.callback-reentrancy` scenario and its pure standard
contract twin use only the official C++ API and standard library. They pass
under both evoked and immediate callback models against the installed package;
the adapter supplies only the provider package, endpoint, and callback
configuration.

The automatic connection-loss ownership-cleanup scenarios are adapter-required
and are covered by the same process-loss fixture contract; the portable TCK
still observes only standard callbacks and services.

Fresh installed-package evidence from 2026-09-26 expands that adapter lane:
the cleanup, automatic unconditional-divestiture, and pending-acquisition-
cancellation base/contract pairs passed 12/12 cases across both callback
models. The `automatic-resign-directive-delete-objects` pair failed 4/4 cases:
post-resign object-name lookup returned `RTIinternalError` instead of the
standard `ObjectInstanceNotKnown`. Its JSON/JUnit evidence is retained per
scenario in `.build/`.

The same date's fresh in-process `verified` sweep covered all 772 promoted
scenario IDs under both callback models (1,544 CTest cases). Eight cases
failed in two service-report families: timestamped delete-failure reporting
could not reserve its report interaction, and the reporting interlock returned
`RTIinternalError` instead of `FederateServiceInvocationsAreBeingReportedViaMOM`.
These failures are provider-conformance evidence from the standard API tests;
adapter-managed connection-loss scenarios are covered separately above.

The promoted `cpp-tck.federation-mom-content-reports` scenario and its pure
standard contract twin request and decode the standard federation MOM
`HLAreportFOMmoduleData` and `HLAreportMIMdata` reports. They check typed
indicator/data parameters, reliable transport, empty tags, producer and region
metadata, callback-boundary subscription revalidation, and the standard
malformed-request exception. The focused installed-package lane passed 4/4
callback-model cases; the source uses only the official IEEE C++ API and the
standard library, with provider, FOM, endpoint, and callback configuration
supplied by the adapter.

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

The promoted `cpp-tck.ownership-acquisition-publication-fence` scenario and
its pure standard contract twin use the adapter-supplied two-attribute FOM to
verify that subscription does not replace publication, unpublished and
undefined acquisition failures, partial `If Available` and regular acquisition,
pending-acquisition overlap, release denial, exact callback attribute subsets
and tags, ownership splits, and the `OwnershipAcquisitionPending` unpublish
boundary. The focused installed-package lane passed 4/4 callback-model cases;
the complete verified matrix passed 828/830 cases, with only the two
adapter-required connection-loss cases skipped and zero failures. The portable
source uses only the official IEEE C++ API and standard library; provider, FOM,
endpoint, and callback configuration remain adapter inputs.

The promoted `cpp-tck.ownership-acquisition-if-available` scenario and its
pure standard contract twin isolate the standard If Available unavailable and
post-divestiture acquisition callbacks. They verify exact object, attribute,
and tag metadata, ownership-state transitions, repeat-acquisition rejection,
and lifecycle cleanup under both callback models while taking provider, FOM,
endpoint, and callback configuration from the adapter.

The promoted `cpp-tck.attribute-ownership-acquisition-cancellation` scenario
and its pure standard contract twin cover the ordinary cancellation boundary:
pre-request and already-owned cancellation failures, a pending acquisition,
`cancelAttributeOwnershipAcquisition`, exact confirmation metadata, suppressed
acquisition delivery, unchanged ownership, and rejection of a repeated
cancellation under both callback models. The portable source uses only the
official IEEE C++ API and standard library; provider, model FOM, endpoint, and
callback configuration remain adapter inputs.

The promoted `cpp-tck.ownership-query-partition-cleanup` scenario and its
contract twin isolate mixed owned/unowned `queryAttributeOwnership` results,
invalid object and attribute-handle failures, and suppression of stale pending
query results after object removal. The focused installed-package lane passed
4/4 callback-model cases, and the complete verified matrix passed 828/830
cases with two expected adapter-managed connection-loss skips and zero
failures. The portable source uses only the official IEEE C++ API and standard
library; provider, multi-attribute FOM, endpoint, and callback configuration
remain adapter inputs.
The promoted `cpp-tck.ownership-query-save-restore-results` pair carries that
query surface across federation save/restore. After saving a federate-owned
attribute beside an unowned one, it transfers ownership in the live execution,
restores, and verifies the exact federate-owned and unowned callback subsets.
It also queries the restored joined-federate MOM object to assert the distinct
RTI-owned callback. The installed-package run passed 4/4 CTest cases and 4/4
direct callback-model results; artifacts are
`.build/cpp-tck-ownership-query-restore-all-results-20260926.json` and
`.build/cpp-tck-ownership-query-restore-all-results-20260926.xml`. Its source uses only the
official IEEE C++ API and standard library, with provider, multi-attribute FOM,
MIM, endpoint, and callback configuration supplied by the adapter.

The promoted `cpp-tck.default-region-interaction-routing` scenario and its
pure standard contract twin cover ordinary interaction routing across explicit
regional subscriptions and the implicit default region. They verify overlap
delivery, disjoint suppression, restoration after regional unsubscription,
ordinary default-region delivery, and conveyed empty source-region metadata.
The focused installed-package lane passed 4/4 callback-model cases; the current
verified matrix passed 832/834 cases with only the two expected adapter-managed
connection-loss cases skipped and zero failures. The portable source uses only
the official IEEE C++ API and standard library; provider, dimensional FOM,
endpoint, and callback configuration remain adapter inputs.

The newly promoted `cpp-tck.default-region-registration-names` scenario and its
pure standard contract twin exercise the public default-source registration
forms: a regional overload with an empty region set, an empty pair collection,
and ordinary registration. They verify distinct valid handles, generated-name
lookup round trips, discovery through an overlapping regional subscription, and
ordinary Update/Reflect with supplied-empty source-region metadata. Their
focused installed-package lane passed 4/4 callback-model cases. The portable
source uses only the official IEEE C++ API and standard library; provider,
dimensional FOM, endpoint, and callback configuration remain adapter inputs.

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

The promoted `cpp-tck.timestamped-regional-interaction-alternate-advances-contract`
runner exposes that same alternate-advance path as an independently selectable
pure standard C++ contract. It retains the source-region metadata,
callback-before-grant, grant/query-bound, backward-time, retraction, and cleanup
assertions while taking provider, dimensional FOM, endpoint, callback, and
logical-time configuration from the adapter.

The promoted `cpp-tck.timestamped-regional-interaction-no-overlap-contract`
runner exposes the non-overlap and retraction boundary as an independently
selectable pure standard C++ contract. It retains the valid-handle,
pre-delivery-retraction, terminal-handle, no-callback, and lifecycle-cleanup
assertions while taking provider, dimensional FOM, endpoint, callback, and
logical-time configuration from the adapter.

The promoted `cpp-tck.timestamped-regional-interaction-subscription-replacement-contract`
runner exposes the callback-time subscription replacement boundary as an
independently selectable pure standard C++ contract. It retains the suppressed
queued passel, replacement-source delivery, conveyed-region metadata,
callback-before-grant, and lifecycle-cleanup assertions while taking provider,
dimensional FOM, endpoint, callback, and logical-time configuration from the
adapter.

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

The promoted `cpp-tck.timestamped-local-delete-object-contract` and
`cpp-tck.timestamped-local-delete-attribute-contract` runners expose those same
recipient-local deletion boundaries as independently selectable pure standard
C++ contracts. They retain the local suppression, surviving delivery, exact
metadata, callback ordering, logical-time, and cleanup assertions while taking
provider, FOM, endpoint, callback, and logical-time configuration from the
adapter.

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

The promoted `cpp-tck.named-registration-multi-recipient` scenario reserves two
explicit object names, registers both named objects, and verifies that two active
subscribers each discover both objects with stable name, instance, and class
identity; the owner receives no discovery loopback. The corresponding
`cpp-tck.named-registration-multi-recipient-contract` runner exposes the same
fan-out behavior as an independently selectable pure standard C++ contract
using only adapter-supplied provider, FOM, endpoint, and callback configuration.

The promoted `cpp-tck.object-registration-discovery-multi-recipient` scenario
registers two ordinary objects and verifies that two active subscribers each
discover both objects with stable object name, instance, and class identity;
the owner receives no discovery loopback. The corresponding
`cpp-tck.object-registration-discovery-multi-recipient-contract` runner exposes
the same two-recipient behavior as an independently selectable pure standard
C++ contract using only adapter-supplied provider, FOM, endpoint, and callback
configuration.

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

The promoted `cpp-tck.order-type-change` scenario and its pure contract twin
make that order-control surface independently selectable from the portable
source. They verify prospective default attributes, a per-object timestamp
override, receive-order interaction delivery, timestamp/order metadata, and
retraction identity while taking provider, FOM, endpoint, callback, and
logical-time configuration from the adapter.

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

The promoted `cpp-tck.passive-regional-interaction-transition` scenario and its
contract twin extend the same declaration lifecycle to region-qualified
interactions. They verify active delivery, downgrade to a retained passive
regional pair while another active route remains, suppression after the last
active route is removed, empty-region no-op behavior, and reactivation of the
same pair. The focused installed-package lane passed 4/4 callback-model cases;
the source uses only official IEEE C++ headers and the standard library, with
the provider, dimensional FOM, endpoint, and callback configuration supplied by
the adapter.

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

The Attribute Relevance Advisory rate-reissue case is promoted after passing
both callback models with the adapter-supplied rich FOM and distinct higher and
lower update-rate designators. It verifies that a lower-rate peer is silent,
that lowering and restoring the active maximum reissues Turn Updates On with
the selected designator, that removing the higher-rate peer refreshes the
surviving rate, and that the final unsubscribe delivers Turn Updates Off. The
source uses only the official C++ API and standard library; the focused portable
lane passed 4/4 callback-model cases, and the matching native oracle passed.

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
 callback enable/disable gating for interaction and ordinary attribute
 reflection, asynchronous-delivery gating with both callback-servicing
 operations, and the
untimed save/restore lifecycle with
status responses, abort/failure paths, and post-restore federate-handle
rebinding, and timed save/restore initiation with its restored queued-delivery
boundary. These checks stay on the official public API;
 they do not require a provider-managed registry, private headers, or a
 process-control fixture.

The promoted `cpp-tck.callback-controls-attribute-update` scenario and its
contract twin extend callback gating to ordinary attribute reflection. They
verify baseline delivery, suppression while callbacks are disabled, release
after re-enable, callback-model servicing, payload/tag/producer identity, and
standard federation cleanup. Their focused installed-package CTest lane passed
4/4 callback-model cases using only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-object-removal` scenario and its
contract twin extend callback gating to ordinary object removal. They verify
baseline discovery/removal, suppression while callbacks are disabled, release
after re-enable, object/tag/producer identity, callback-model servicing, and
standard federation cleanup. Their focused installed-package CTest lane passed
4/4 callback-model cases using only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-object-discovery` scenario and its
contract twin extend callback gating to ordinary object discovery. They verify
registration while callbacks are disabled, suppression before re-enable, release
after re-enable, object/class/name/producer identity, callback-model servicing,
and standard federation cleanup. Their focused installed-package CTest lane
passed 4/4 callback-model cases using only the official C++ API and standard
library.

The promoted `cpp-tck.callback-controls-object-name-reservation` scenario and
its contract twin extend callback gating to ordinary object-name reservation.
They verify reservation-callback suppression, release after re-enable,
callback-model servicing, name cleanup, and standard federation lifecycle. Their
focused installed-package CTest lane passed 4/4 callback-model cases using only
the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-object-name-reservation-failure`
scenario and its contract twin extend callback gating to contended ordinary
object-name reservation. They verify the baseline reservation, failure-callback
suppression while callbacks are disabled, release after re-enable, callback-model
servicing, name cleanup, and standard federation lifecycle. Their focused
installed-package CTest lane passed 4/4 callback-model cases using only the
official C++ API and standard library.

The promoted `cpp-tck.callback-controls-multiple-object-name-reservation`
scenario and its contract twin extend callback gating to successful and mixed
multiple object-name reservations. They verify success and failure callback
sets, suppression while callbacks are disabled, release after re-enable,
callback-model servicing, name cleanup, and standard federation lifecycle. Their
focused installed-package CTest lane passed 4/4 callback-model cases using only
the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-declaration-advisories` scenario and
its contract twin extend callback gating to standard declaration-relevance
advisories. They verify suppression and release for start/stop registration and
interaction turn-on/turn-off callbacks, callback-model servicing, handle
identity, declaration cleanup, and standard federation lifecycle. Their focused
installed-package CTest lane passed 4/4 callback-model cases using only the
official C++ API and standard library.

The promoted `cpp-tck.callback-controls-attribute-relevance-advisories` scenario
and its contract twin extend callback gating to standard per-object
attribute-relevance advisories. They verify unnamed and named turn-up/turn-down
suppression and release, callback-model servicing, object/attribute/update-rate
identity, declaration cleanup, and standard federation lifecycle. Their focused
installed-package CTest lane passed 4/4 callback-model cases using only the
official C++ API and standard library.

The promoted `cpp-tck.callback-controls-attribute-value-request` scenario and
its contract twin extend callback gating to ordinary attribute-value requests.
They verify discovery, provider-callback suppression, release after re-enable,
callback-model servicing, object/attribute/tag identity, and standard cleanup.
Their focused installed-package CTest lane passed 4/4 callback-model cases using
only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-auto-provide` scenario and its contract
twin extend callback gating to the standard Auto Provide callback. They verify
provider-callback suppression, release after re-enable, callback-model servicing,
object/attribute identity, the empty standard tag, the Auto Provide switch, and
standard federation cleanup. Their focused installed-package CTest lane passed
4/4 callback-model cases using only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-ownership-assumption` scenario and its
contract twin extend callback gating to the standard ownership-assumption and
acquisition handoff. They verify unconditional divestiture, ownership-assumption
suppression, release after re-enable, callback-model servicing, acquisition tag
and ownership state, and standard federation cleanup. Their focused
installed-package CTest lane passed 4/4 callback-model cases using only the
official C++ API and standard library.

The promoted `cpp-tck.callback-controls-ownership-unavailable` scenario and its
contract twin extend callback gating to the standard If Available denial path.
They verify ownership-unavailable suppression, release after re-enable,
callback-model servicing, object/attribute/tag identity, unchanged ownership,
and standard federation cleanup. Their focused installed-package CTest lane
passed 4/4 callback-model cases using only the official C++ API and standard
library.

The promoted `cpp-tck.callback-controls-ownership-release-request` scenario and
its contract twin extend callback gating to the standard ownership-release
request and acquisition handoff. They verify regular acquisition,
ownership-release request suppression, release after re-enable, callback-model
servicing, acquisition/divestiture tags, ownership transfer, and standard
federation cleanup. Their focused installed-package CTest lane passed 4/4
callback-model cases using only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-ownership-acquisition-cancellation`
scenario and its contract twin extend callback gating to the standard
ownership-acquisition cancellation confirmation. They verify a pending regular
acquisition, cancellation callback suppression, release after re-enable,
callback-model servicing, request metadata, unchanged ownership, and standard
federation cleanup. Their focused installed-package CTest lane passed 4/4
callback-model cases using only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-ownership-release-denied` scenario and
its contract twin extend callback gating to the standard ownership-unavailable
denial callback. They verify a pending regular acquisition, release-request
metadata, denial callback suppression, release after re-enable, callback-model
servicing, denial metadata, unchanged ownership, and standard federation
cleanup. Their focused installed-package CTest lane passed 4/4 callback-model
cases using only the official C++ API and standard library.

The promoted `cpp-tck.callback-controls-ownership-acquisition-notification`
scenario and its contract twin extend callback gating to the standard
ownership-acquisition notification after a successful Divestiture If Wanted
handoff. They verify a pending regular acquisition, divestiture metadata,
acquisition-notification suppression, release after re-enable, callback-model
servicing, ownership transfer, and standard federation cleanup. Their focused
installed-package CTest lane passed 4/4 callback-model cases using only the
official C++ API and standard library.

The promoted `cpp-tck.callback-controls-ownership-divestiture-confirmation`
scenario and its contract twin extend callback gating to the standard negotiated
divestiture confirmation. They verify a pending regular acquisition, negotiated
divestiture callback suppression, release after re-enable, callback-model
servicing, acquisition-request metadata, unchanged ownership, restoration of
the ordinary release request after cancellation, and standard federation
cleanup. Their focused installed-package CTest lane passed 4/4 callback-model
  cases using only the official C++ API and standard library.

The newly promoted callback-control ownership-query slice adds
`cpp-tck.callback-controls-ownership-query` and its pure contract twin. It
verifies suppression and release of the mixed ordinary ownership-query
callbacks, then repeats the callback-control boundary for the standard
RTI-owned joined-federate MOM query callback. It checks exact attribute
partitions, owning-federate and object metadata, callback-model servicing,
adapter-supplied MIM setup, and federation cleanup. Its focused installed-
package lane passed 4/4 callback-model cases across three repeats using only
the official C++ API and standard library.

The newly promoted callback-control synchronization slice adds
`cpp-tck.callback-controls-synchronization` and its pure contract twin. It
verifies suppression and release of synchronization-point announcements and
Federation Synchronized completion, exact label/tag and failure-set metadata,
callback-model servicing, and federation cleanup. Its focused installed-
package lane passed 4/4 callback-model cases across three repeats using only
the official C++ API and standard library.

The newly promoted callback-control time-advance slice adds
`cpp-tck.callback-controls-time-advance` and its pure contract twin. It verifies
time-regulation and time-constrained enablement, suppression and release of
time-advance grants, callback-model servicing, exact grant times, and time-
management/federation cleanup. Its focused installed-package lane passed 4/4
callback-model cases across three repeats using only the official C++ API and
standard library.

The newly promoted callback-control save/restore slice adds
`cpp-tck.callback-controls-save-restore` and its pure contract twin. It verifies
suppression and release of save initiation/completion, save-status, restore
initiation/status, and restore-completion callbacks, exact labels and identity
metadata, callback-model servicing, and federation cleanup. Its focused
installed-package lane passed 4/4 callback-model cases across three repeats
using only the official C++ API and standard library.

The newly promoted callback-control save/restore-failure slice adds
`cpp-tck.callback-controls-save-restore-failures` and its pure contract twin. It
verifies suppression and release of `federationNotSaved` and
`federationNotRestored`, exact standard failure reasons, callback-model
servicing, and federation cleanup. Its focused installed-package lane passed
4/4 callback-model cases across three repeats using only the official C++ API
and standard library.

The newly promoted callback-control restore-request-failure slice adds
`cpp-tck.callback-controls-restore-request-failure` and its pure contract twin.
It verifies suppression and release of the requesting federate's failed restore
request callback, exact save-label delivery, absence of restore-start callbacks,
callback-model servicing, and federation cleanup. Its focused installed-package
lane passed 4/4 callback-model cases across three repeats using only the official
C++ API and standard library.

The newly promoted callback-control timestamped-attribute slice adds
`cpp-tck.callback-controls-timestamped-attribute-update` and its pure contract
twin. It verifies suppression and release of timestamped reflection and the
time-advance grant, exact value/tag/time/order/transport/retraction metadata,
reflection-before-grant ordering, callback-model servicing, and federation
cleanup. Its focused installed-package lane passed 4/4 callback-model cases
across three repeats using only the official C++ API and standard library.

The newly promoted callback-control timestamped-object-removal slice adds
`cpp-tck.callback-controls-timestamped-object-removal` and its pure contract
twin. It verifies suppression and release of timestamped object removal and the
time-advance grant, exact object/name/tag/time/order/retraction metadata,
removal-before-grant ordering, callback-model servicing, and federation cleanup.
Its focused installed-package lane passed 4/4 callback-model cases across three
repeats using only the official C++ API and standard library.

The newly promoted callback-control timestamped-retraction slice adds
`cpp-tck.callback-controls-timestamped-retraction` and its pure contract twin.
It verifies timestamped interaction delivery and retraction, suppression and
release of the request-retraction callback, exact interaction and retraction
handle metadata, callback ordering, callback-model servicing, and federation
cleanup. Its focused installed-package lane passed 4/4 callback-model cases
across three repeats using only the official C++ API and standard library.

The newly promoted callback-control transportation slice adds
`cpp-tck.callback-controls-transportation` and its pure contract twin. It
verifies suppression and release of standard attribute and interaction
transportation reports and confirmations, exact object/class/attribute/federate
and transportation metadata, committed transportation state, callback-model
servicing, and federation cleanup. Its focused installed-package lane passed
4/4 callback-model cases across three repeats using only the official C++ API and
standard library.

The newly promoted callback-control directed-interaction slice adds
`cpp-tck.callback-controls-directed-interaction` and its pure contract twin. It
verifies target routing, exact parameter/tag/transport/producer metadata,
suppression and release of `receiveDirectedInteraction`, callback-model
servicing, and federation cleanup. Its focused installed-package lane passed
4/4 callback-model cases across three repeats using only the official C++ API
and standard library.

The newly promoted callback-control federation-report slice adds
`cpp-tck.callback-controls-federation-reports` and its pure contract twin. It
verifies exact federation execution and member report content, the
federation-does-not-exist report, callback suppression and release, callback-model
servicing, and federation cleanup. Its focused installed-package lane passed
4/4 callback-model cases across three repeats using only the official C++ API
and standard library.

The newly promoted callback-control attribute-scope slice adds
`cpp-tck.callback-controls-attribute-scope-advisories` and its pure contract
twin. It verifies explicit standard advisory-switch setup, exact object and
attribute sets for in-scope and out-of-scope transitions, callback suppression
and release, callback-model servicing, and region/federation cleanup. Its focused
installed-package lane passed 4/4 callback-model cases across three repeats using
only the official C++ API and standard library.

The newly promoted callback-control Flush Queue slice adds
`cpp-tck.callback-controls-flush-queue-grant` and its pure contract twin. It
verifies suppression and release of a queued timestamped interaction and
`flushQueueGrant`, interaction-before-grant ordering, exact payload/tag/producer/
time/order/retraction metadata, callback-model servicing, and time/federation
cleanup. Its focused installed-package lane passed 4/4 callback-model cases
across three repeats using only the official C++ API and standard library.

The newly promoted callback-control available-time-advance slice adds
`cpp-tck.callback-controls-available-time-advance-callbacks` and its pure
contract twin. It verifies callback suppression and release around standard
Time Advance Request Available and Next Message Request Available delivery,
interaction-before-grant ordering, exact timestamped interaction and retraction
metadata, callback-model servicing, and time/federation cleanup. Its focused
installed-package lane passed 4/4 callback-model cases across three repeats using
only the official C++ API and standard library.

The newly promoted callback-control time-role slice adds
`cpp-tck.callback-controls-time-role-enablement` and its pure contract twin. It
verifies suppression and release around standard time-regulation and
time-constrained enablement callbacks, exact initial-time metadata,
callback-model servicing, and time/federation cleanup. Its focused
installed-package lane passed 4/4 callback-model cases across three repeats using
only the official C++ API and standard library.

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
so they are not force-mapped into the portable TCK. The catalog now promotes the
standard-API base case `cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore`:
its focused installed-package lane passed 4/4 callback-model cases. Evoked mode
retains the regular candidate through negotiated divestiture; immediate mode
verifies the standard unavailable boundary before queued delivery and a
synchronous ordinary ownership-release/divestiture handoff afterward.
The promoted regular-to-regular confirmation-cancellation case
`cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore`:
passed 4/4 callback-model cases in its focused lane. Evoked mode covers the
full post-confirmation cancellation route; immediate mode verifies the standard
unavailable cancellation window before queued delivery and a synchronous
ordinary ownership-release/divestiture handoff afterward.
The pre-delivery cancellation case
`cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore`
also passed 4/4 callback-model cases. Evoked mode covers the full pre-delivery
cancellation route; immediate mode verifies the same standard callback-window
boundary and ordinary handoff.

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

The installed-package adapter defaults to the promoted scenario set. The
current catalog has 774 promoted IDs and no candidates; with
`--callback-model both`, that selects 1,548 callback-model runs.
`--scenario-set all` selects the same 774 inventory IDs. The latest
per-scenario installed-package audit has complete two-model evidence: 768 IDs
pass both callback models and six known provider failures fail both models, so
the full gate is not green. There are no unverified promoted IDs. See the
current checkpoint in `docs/testing/CPP-RTI-CONFORMANCE-TCK.md` for the failure
oracles and evidence details. A fresh `hla_rti_cpp_tck` source build succeeded
on 2026-09-26 after explicitly selecting the installed Windows SDK root/version
and target SDK path while retaining normal Visual Studio toolset discovery.
On that rebuilt executable, the connection/callback and
configuration/authorization baselines plus their Java-parity IDs passed both
callback models (8/8). The artifacts are
`.build/cpp-tck-current-credential-baseline.json` and
`.build/cpp-tck-current-credential-parity.json`. Fresh split evidence now covers
all 774 IDs, including the verified explicit-set late-join and ownership
save/restore pairs, the standard-MIM dimension query/report set, the
QueryAttributeOwnership/CancelAttributeOwnershipAcquisition report-and-callback
pairs, and the resignation service-report pair. The new
`GetDimensionHandleSet` base/contract pair passed all four CTest cases and all
four direct callback-model results against `.build/package-tck-current`; the
consolidated six-operation dimension query/report slice passed 24/24 CTest and
24/24 direct callback-model runs. The `GetRangeBounds` base/contract pair also
passed all four focused CTest cases and all four direct callback-model results
against `.build/package-tck-current`; see
`.build/cpp-tck-service-report-range-bounds-corrected-20260926.json` and its
JUnit sibling. Together these seven dimension/range query-report operations
cover 14 scenario IDs, 28 focused CTest cases, and 28 direct callback-model
results. The new `SetRangeBounds` validation pair compiled against
`.build/package-tck-current` and passed all four focused CTest cases and all
four direct callback-model results; it verifies the valid inclusive upper-bound
case and the standard `InvalidRangeBound` exception for equal and reversed
bounds without asserting MOM failure payloads. Evidence is in
`.build/cpp-tck-region-range-validation-candidate-20260926.json` and its JUnit
sibling. The range-dimension membership pair passed its focused installed-
package CTest lane (4/4) and direct callback-model run (4/4), requiring the
standard `RegionDoesNotContainSpecifiedDimension` exception from both query and
setter calls. It makes no MOM failure-payload assertion; evidence is in
`.build/cpp-tck-region-range-dimension-validation-candidate-20260926.json` and
its JUnit sibling. The
successful `SetRangeBounds` service-report base/contract pair passed all four
focused CTest cases and all four direct callback-model runs against
`.build/package-tck-current`. It checks the standard region service type,
argument records, null return marker, serial, reliable delivery, and inherited
`HLAfederate` identity; failure-report details remain excluded. Evidence is in
`.build/cpp-tck-service-report-set-range-bounds-candidate-20260926.json` and its
JUnit sibling. The successful `CommitRegionModifications` service-report
base/contract pair passed all four focused CTest cases and all four direct
callback-model runs against `.build/package-tck-current`. It verifies the
standard region-set argument, null return marker, success state, serial zero,
reliable delivery, and inherited `HLAfederate` identity; it makes no
region-routing or failure-payload assertion. Evidence is in
`.build/cpp-tck-service-report-commit-region-modifications-candidate-20260926.json`
and its JUnit sibling. The successful `DeleteRegion` service-report
base/contract pair passed all four focused CTest cases and all four direct
callback-model runs against `.build/package-tck-current`. It verifies the
standard region-designator argument, null return marker, success state, serial
zero, reliable delivery, inherited `HLAfederate` identity, and post-delete
invalid-region behavior through the standard API; failure-payload assertions
remain excluded. Evidence is in
`.build/cpp-tck-service-report-delete-region-candidate-20260926.json` and its
JUnit sibling. The successful `CreateRegion` service-report base/contract pair
passed all four focused CTest cases and all four direct callback-model runs
against `.build/package-tck-current`. It verifies the type-11 dimension-set
argument, type-42 returned region designator, success state, serial zero,
reliable delivery, inherited `HLAfederate` identity, and standard-API cleanup;
failure-payload assertions remain excluded. Evidence is in
`.build/cpp-tck-service-report-create-region-candidate-20260926.json` and its
JUnit sibling. The
registered installed-package CTest matrix ran
720 IDs (1,440 callback-model cases): 716 IDs passed in both models and four
existing service-report IDs failed in both. The Python process adapter covered
the eight connection-loss IDs omitted by CTest: six passed both models (12/12),
while the two automatic-resign deletion IDs failed both (4/4). See the CTest
transcript at `.build/cpp-tck-current-registered-matrix-20260926.log` and the
connection-loss result artifacts listed in the main TCK checkpoint. The prior
combined evidence had 726 IDs passing both models and six known failures. Three
additional ownership save/restore base/contract pairs—pending negotiated
ownership, acquisition cancellation, and If Available unavailable-callback
delivery—then passed both callback models, bringing the evidence set to 734
passing IDs and six known failures, with no unverified promoted IDs. The
combined four-pair ownership lane passed 16/16 CTest and direct cases; see
`.build/cpp-tck-ownership-save-restore-slices-20260926.json`. A later focused
rerun after adding a BMP
non-ASCII `HLAplainTextPassword` round-trip rebuilt the installed-package TCK
consumer and passed `cpp-tck.configuration-and-authorization-contract` in both
callback models (2/2); see
`.build/cpp-tck-current-password-unicode-20260926.json`. A header/catalog
cross-check also corrected the callback name for four ownership save/restore
scenarios to the official
`FederateAmbassador.confirmAttributeOwnershipAcquisitionCancellation`; the
four IDs passed both models (8/8) in
`.build/cpp-tck-current-ownership-callback-labels-20260926.json`. The ordinary
acquisition-cancellation base/contract pair now maps to `java-tck.ownership`
and passed both models (4/4) in
`.build/cpp-tck-current-acquisition-cancellation-parity-20260926.json`. The
directed-interaction overload audit made whole-class and explicit-set
unpublication and unsubscription signatures explicit in the service-report
catalog rows and their contract twins. All 15 overloaded official
`RTIambassador` methods now have distinct catalog variants. The ordinary
directed-interaction pair passed both models (4/4), and the four directed
service-report base/contract scenarios passed both models (8/8), recorded in
`.build/cpp-tck-current-directed-declaration-overloads-20260926.json` and
`.build/cpp-tck-current-directed-overloads-service-reports-20260926.json`.
The pure C++ synchronization-point explicit-set late-join scenario verifies
that an excluded late member receives the standard
`SynchronizationPointLabelNotAnnounced` exception and that the registered
member can still complete the point. Its base/contract pair passed both callback
models (4/4) against the installed package in
`.build/cpp-tck-current-sync-explicit-late-join-20260926.json`. It is not yet
classified as Java parity: the Java scenario covers an already-joined excluded
member, and this workspace has no current 2025 Java API JAR to verify the exact
late-join companion.

The pure ownership-state save/restore base and contract cases preserve a
completed negotiated transfer across the standard federation save/restore
lifecycle, then resolve the restored object by name and verify its owner through
`isAttributeOwnedByFederate` and `queryAttributeOwnership`. All four focused
CTest cases and both direct callback-model runs passed against the installed
package; evidence is in
`.build/cpp-tck-ownership-state-save-restore-20260926.json` and
`.build/cpp-tck-ownership-state-save-restore-20260926.xml`.
The pending negotiated-ownership save/restore base and contract pair preserve
an outstanding acquisition request and owner confirmation across federation
save/restore, then complete the transfer after restore and verify its callback,
ownership state, and query result. The evoked model uses
`attributeOwnershipAcquisitionIfAvailable`; the immediate model uses the
regular pending acquisition service because the unavailable If Available
callback is reported inline there. Both IDs passed both callback models (4/4
CTest and direct cases) against the installed package; evidence is in
`.build/cpp-tck-pending-ownership-save-restore-20260926.json` and
`.build/cpp-tck-pending-ownership-save-restore-20260926.xml`.
The ownership-acquisition-cancellation pair verifies the cancellation
confirmation is delivered exactly once across federation save/restore without
transferring ownership. The If Available restore pair checks the standard
`attributeOwnershipUnavailable` callback when the owner retains the attribute.
Both pairs passed 4/4 callback-model cases against the installed package. The
combined four-pair ownership lane passed 16/16 cases; its JSON/XML evidence is
`.build/cpp-tck-ownership-save-restore-slices-20260926.json` and
`.build/cpp-tck-ownership-save-restore-slices-20260926.xml`.
Focused follow-ups passed six
update-rate/advisory scenarios under both callback models (12/12), recorded
in `.build/cpp-tck-current-update-rate-callback.json` and
`.build/cpp-tck-current-update-rate-callback-followup.json`; this does not add
IDs. Without the optional connection-loss fixture, those cases remain explicit
expected skips rather than passes. The aggregate `hla_rti_cpp_tck_installed`
CTest remains available for a single full-run check.

The newly promoted `cpp-tck.zero-dimensional-regional-interaction` scenario
and its pure standard contract twin establish ordinary interaction delivery,
then verify that an explicit committed region with no dimensions does not
overlap the ordinary/default subscription. Their focused installed-package
lane passed 4/4 callback-model cases. The source uses only official IEEE C++
API headers and the standard library; provider, FOM, endpoint, and callback
configuration remain adapter inputs.

The newly promoted `cpp-tck.inherited-object-attribute-projection` scenario
and its pure standard contract twin verify discovery of a derived object through
both base and derived subscriptions, base-only inherited-attribute projection,
and derived-subscription delivery of inherited plus declared attributes with
ordinary reflection metadata. Their focused installed-package lane passed 4/4
callback-model cases. The source uses only official IEEE C++ API headers and the
standard library; the rich model FOM, provider, endpoint, and callback
configuration remain adapter inputs.

The newly promoted `cpp-tck.multi-region-interaction-routing` scenario and its
pure standard contract twin verify union routing for multiple explicit source
regions: each overlapping subscription receives one callback with the complete
source-region set, while a disjoint subscription remains silent. Their focused
installed-package lane passed 4/4 callback-model cases. The source uses only
official IEEE C++ API headers and the standard library; provider, FOM, endpoint,
and callback configuration remain adapter inputs.

The newly promoted `cpp-tck.regional-interaction-empty-subscription-sets`
scenario and its pure standard contract twin verify that empty regional
subscription and unsubscription sets are no-ops, while a corresponding
non-empty unsubscription removes the route. Their focused installed-package
lane passed 4/4 callback-model cases. The source uses only official IEEE C++
API headers and the standard library; provider, FOM, endpoint, and callback
configuration remain adapter inputs.

The promoted `cpp-tck.fom-empty-module-validation` scenario checks the
standard empty-FOM rejection boundary, then creates and joins the same
federation name with the adapter-supplied FOM to prove that the rejected
request did not reserve partial state. It uses only the official C++ API.
The promoted `cpp-tck.fom-additional-module-join-atomicity` scenario and its
contract twin reject each adapter-supplied invalid additional FOM with a
standard exception, verify that membership and declaration lookups remain
unchanged, and then verify a valid follow-up join composes shared handles. The
focused installed-package lane passed 4/4 callback-model cases using only the
official C++ API and standard library.
The promoted `cpp-tck.interaction-class-lookup-lifecycle` scenario and its
contract twin cover standard interaction-class handle/name lookup across
pre-connect, pre-join, invalid-name/invalid-handle, and additional-FOM
composition boundaries. Their focused installed-package lane passed 8/8
callback-model cases with the adapter-supplied base and extension FOMs. The
source uses only official IEEE C++ API headers and the standard library.
The promoted `cpp-tck.object-class-lookup-lifecycle` scenario and its
contract twin cover standard object-class handle/name lookup across
pre-connect, pre-join, invalid-name/invalid-handle, and additional-FOM
composition boundaries. The object-class pair passed 4/4 callback-model cases
within the focused six-scenario lane, which passed 12/12 overall. The source
uses only official IEEE C++ API headers and the standard library.
The promoted `cpp-tck.attribute-lookup-lifecycle` scenario and its contract
twin cover standard attribute handle/name lookup across pre-connect, pre-join,
inherited base/derived object classes, invalid-name/invalid-handle boundaries,
and cross-federate handle identity. The attribute pair passed 4/4
callback-model cases within the focused eight-scenario rich-FOM/lookup lane,
which passed 16/16 overall. The source uses only official IEEE C++ API headers
and the standard library.
The promoted `cpp-tck.parameter-lookup-lifecycle` scenario and its contract
twin cover standard parameter handle/name lookup across pre-connect, pre-join,
inherited base/derived interaction classes, invalid-name/invalid-handle
boundaries, and cross-federate handle identity. The parameter pair passed 4/4
callback-model cases within the focused ten-scenario rich-FOM/lookup lane,
which passed 20/20 overall. The source uses only official IEEE C++ API headers
and the standard library.
The promoted `cpp-tck.dimension-lookup-lifecycle` scenario and its contract
twin cover standard dimension handle/name/upper-bound and available-dimension
lookup before connection, before membership, after resign, for invalid inputs,
and across federates. The dimension pair passed 4/4 callback-model cases in its
focused installed-package lane using only official IEEE C++ API headers and the
standard library; the dimensional FOM and dimension names remain adapter inputs.
The separately selectable `cpp-tck.dimension-lookup-valid-handles-after-resignation`
pair adds valid-input post-resignation checks for both class-dimension queries,
dimension-name lookup, and upper-bound lookup. Both IDs passed evoked and
immediate callback models (4/4) against the installed package.
The promoted `cpp-tck.fom-update-rate-value-stability` scenario and its contract
twin verify that a positive base update-rate value remains stable while a new
adapter-supplied update-rate designator is composed through an additional-FOM
join. The focused pair passed 4/4 callback-model cases, and the surrounding
ten-scenario FOM-composition regression fence passed 20/20 CTest and direct
cases. The source uses only official IEEE C++ API headers and the standard
library; the rich FOM, additional FOM, update-rate names, provider, endpoint,
and callback configuration remain adapter inputs.
The promoted `cpp-tck.fom-invalid-create-atomicity` scenario and its contract
twin reject every adapter-supplied invalid single-module create, then reuse the
same federation name for a valid create/join/lookup/resign/destroy lifecycle.
Its focused installed-package lane passed 4/4 callback-model cases using only
the official C++ API and standard library.

The promoted `cpp-tck.fom-invalid-composite-join-atomicity` scenario and its
contract twin reject a mixed valid-plus-invalid additional-FOM join atomically,
verify no membership or declaration mutation, and complete a valid follow-up
join with shared handles. Its focused installed-package CTest lane passed 4/4
callback-model cases using only the official C++ API and standard library.

The promoted `cpp-tck.fom-invalid-mim-create-atomicity` scenario and its
contract twin reject adapter-supplied invalid MIM modules atomically, then
reuse the same federation name for a valid FOM/MIM create, join, lookup,
resign, and destroy lifecycle. Its focused installed-package CTest lane
passed 4/4 callback-model cases using only the official C++ API and standard
library.

The promoted `cpp-tck.fom-invalid-composite-mim-create-atomicity` scenario and
its contract twin reject a mixed valid-plus-invalid FOM module vector atomically
when creating with the standard MIM, then reuse the same federation name for a
valid FOM/MIM create, join, lookup, resign, and destroy lifecycle. Its focused
installed-package CTest lane passed 4/4 callback-model cases using only the
official C++ API and standard library.

The promoted `cpp-tck.fom-empty-mim-create-atomicity` scenario and its contract
twin reject an empty FOM module vector with standard `InvalidFOM`, then reuse the
same federation name for a valid FOM/MIM create, join, lookup, resign, and destroy
lifecycle. Its focused installed-package CTest lane passed 4/4 callback-model
cases using only the official C++ API and standard library.

The promoted `cpp-tck.explicit-mim-creation-contract` and
`cpp-tck.federation-mom-current-fdd-contract` runners expose the standard MIM
composition and federation-MOM current-FDD surfaces as pure C++ contracts.
Their focused four-scenario lane passed 8/8 callback-model cases; provider,
FOM/MIM, endpoint, callback, and logical-time configuration remain adapter
inputs.

The promoted `cpp-tck.custom-transportation-interaction-delivery` scenario and
its pure standard contract twin use the adapter-declared rich FOM to verify
custom transportation handle/name round-trips, ordinary interaction
publication/subscription/send delivery, received transportation identity, and
the standard transportation-type query report. Their focused portable lane
passed 4/4 callback-model cases; the catalog now routes this existing pair
through the portable adapter entry point, and the reusable scenario source uses
only the official C++ API and standard library.

The promoted `cpp-tck.custom-transportation-directed-interaction-delivery`
scenario and its pure standard contract twin use the same adapter-declared rich
FOM to verify ordinary directed publication and subscription, target
registration/discovery, parameter and tag preservation, producer identity,
publisher loopback suppression, universal versus by-ownership delivery, and
custom transportation identity. Their focused portable lane passed 4/4
callback-model cases; the reusable scenario source uses only the official C++
API and standard library, with provider, FOM, endpoint, and callback
configuration remaining adapter inputs.

The promoted `cpp-tck.custom-transportation-regional-attribute-delivery`
scenario and its pure standard contract twin use the adapter-declared rich FOM
and DDM dimensions to verify ordinary regional attribute
publication/subscription/update delivery, conveyed source-region metadata,
overlap filtering, and custom transportation identity. Their focused portable
lane passed 4/4 callback-model cases, and the matching native oracle passed 51
assertions; the reusable scenario source uses only the official C++ API and
standard library.

The promoted `cpp-tck.custom-transportation-regional-interaction-delivery`
scenario and its pure standard contract twin use the same adapter-declared rich
FOM and DDM dimensions to verify ordinary regional interaction
publication/subscription/send delivery, parameter, tag, producer, conveyed
source-region, and custom transportation metadata. Their focused portable lane
passed 4/4 callback-model cases, and the matching native oracle passed 40
assertions; the reusable scenario source uses only the official C++ API and
standard library.

The promoted `cpp-tck.custom-transportation-timestamped-delivery` scenario
and its pure standard contract twin use the adapter-declared rich FOM to
verify timestamped interaction publication/subscription/send delivery,
constrained grant timing, payload/tag/producer/time/order/retraction metadata,
and custom transportation identity without region metadata. Their focused
portable lane passed 4/4 callback-model cases; the catalog now routes this
existing pair through the portable adapter entry point, and the reusable
scenario source uses only the official C++ API and standard library.

The promoted `cpp-tck.custom-transportation-timestamped-directed-delivery`
scenario and its pure standard contract twin use the adapter-declared rich FOM
to verify timestamped directed interaction publication/subscription/send
delivery to a registered target, constrained grant timing,
payload/tag/target/producer/time/order/retraction metadata, and custom
transportation identity. Their focused portable lane passed 4/4 callback-model
cases; the catalog now routes this existing pair through the portable adapter
entry point, and the reusable scenario source uses only the official C++ API
and standard library.

The promoted `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery`
scenario and its pure standard contract twin reuse the standard regional
timestamped-attribute oracle with adapter-declared object, attribute,
interaction, parameter, transportation, and DDM names. Their focused portable
lane passed 4/4 callback-model cases, and the matching native oracle passed 60
assertions; the reusable scenario source uses only the official C++ API and
standard library.

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
The promoted `cpp-tck.modify-lookahead` scenario and its pure contract twin
split the lookahead service into an independently selectable standard slice:
they verify the pre-regulation exception boundary, Query Lookahead after
enablement, a valid increase, InvalidLookahead rejection, deferred decrease,
and application of that decrease after a shared time-advance grant. The
provider package, FOM, endpoint, logical-time implementation, and callback
model remain adapter inputs.
The promoted `cpp-tck.exception-hierarchy-contract` runner exposes the complete
official C++ exception hierarchy as an independently selectable,
provider- and FOM-independent slice; its cross-language parity anchor is
`java-tck.overloads-and-exceptions`.
The promoted `cpp-tck.standard-exception-boundaries` scenario and its contract
twin add deterministic service-level exception outcomes to that API inventory,
using only the adapter-supplied baseline FOM and standard federation services;
there is no Java runtime claim for this C++-only extension.
The promoted `cpp-tck.post-resignation-service-boundaries` scenario and its
contract twin extend that boundary across standard lookups, declarations,
object and interaction services, time management, synchronization, and save
requests after resignation. The focused installed-package lane passed 4/4
callback-model cases; the source remains limited to official IEEE C++ headers
and the standard library.
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
The Java-parity `java-tck.factory-discovery` ID now dispatches through the same
portable factory/encoder source, so both factory discovery and its C++ contract
remain provider- and FOM-independent.
The promoted `cpp-tck.logical-time-factory-factory-contract` runner exposes the
official logical-time factory-factory default and integer selection, reference-
factory forwarding, unknown-name rejection, and initial-value construction as an
independently selectable, provider- and FOM-independent slice; its parity anchor
is `java-tck.logical-time-factory`.
The Java-parity `java-tck.logical-time-factory` ID now dispatches through the
portable logical-time factory source, keeping its lifecycle, encoding, arithmetic,
and boundary checks on the standard API with adapter-owned logical-time and FOM
configuration.
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
The promoted `cpp-tck.federation-restore-abort` and
`cpp-tck.federation-restore-abort-contract` runners add the successful restore-
start followed by `abortFederationRestore` lifecycle as an independently
selectable standard slice. Both use only the official C++ API and standard
library; the adapter supplies the provider package, FOM, endpoint, callback
model, and logical-time configuration.
The promoted `cpp-tck.federation-restore-work-item-ownership-assumption` and
`cpp-tck.federation-restore-work-item-ownership-assumption-contract` runners
extend that boundary to standard ownership-assumption work surviving a
save/restore boundary; provider, FOM, endpoint, callback, and logical-time
configuration remain adapter inputs.
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
The promoted `cpp-tck.timestamped-directed-interaction-retraction` scenario and
its contract twin cover the directed-message retraction boundary: a queued
timestamped directed interaction is retracted before delivery, the receiver
advances cleanly, and a later message proves normal delivery with target, tag,
producer, time, order, transportation, and retraction metadata. The source uses
only the official IEEE C++ API and standard library; provider, FOM, endpoint,
logical-time, and callback configuration remain adapter inputs.
The promoted `cpp-tck.timestamped-directed-interaction-retraction-fanout`
scenario and its contract twin extend that boundary to two independent
constrained recipients: one retraction suppresses both queued copies, each
recipient advances without a stale callback, and a later directed message
delivers to both with target, payload, producer, time/order, transportation,
and retraction metadata before the delivered handle reaches the standard
`MessageCanNoLongerBeRetracted` boundary.
The source remains limited to the official IEEE C++ API and standard library;
provider, FOM, endpoint, logical-time, and callback configuration remain
adapter inputs.
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
`cpp-tck.standard-order-and-transportation-lookups`,
`cpp-tck.standard-order-and-transportation-lookups-contract`,
`cpp-tck.transport-order`, `cpp-tck.transport-order-contract`, and
`cpp-tck.relevance-advisories-contract` runners similarly expose the standard
support lookup, ordinary order/transport, and advisory-switch surfaces as
independently selectable pure C++ slices.
The Java-parity `java-tck.transport-order` ID now dispatches through the same
portable ordinary order/transport implementation, keeping the language-parity
entry point on the standard-only source and adapter-owned configuration.
The promoted `java-tck.synchronization`, `cpp-tck.synchronization-points`, and
`cpp-tck.synchronization-point-contract` runners now expose the Java-parity
synchronization-point lifecycle through the portable standard source, including
global and explicit-set registration, late-join announcement, achievement
failure metadata, completion callbacks, and lifecycle boundaries.
The promoted `cpp-tck.transportation-type-change` scenario and its pure contract
twin similarly expose ordinary attribute and interaction transportation-type
changes as an independently selectable standard slice. They verify standard
reliable/best-effort lookup, pending versus immediate confirmation boundaries,
delivery before and after confirmation, and committed query reports while the
provider package, FOM, endpoint, and callback model remain adapter inputs.
The promoted `cpp-tck.order-type-change` scenario and its pure contract twin
similarly expose the remaining ordinary order-control route as a separate
portable standard slice, covering prospective defaults, per-object overrides,
receive-order interaction control, timestamped delivery, and order/retraction
metadata without adding provider-specific dependencies.
The promoted `cpp-tck.fom-model-contract`,
`cpp-tck.fom-module-composition-contract`,
`cpp-tck.fom-additional-module-join-atomicity-contract`, and
`cpp-tck.fom-empty-module-validation-contract` runners similarly expose the
standard FOM model, module-composition, and empty-module validation surfaces as
independently selectable pure C++ slices.
The promoted `cpp-tck.service-report-interaction-contract`,
`cpp-tck.service-report-attribute-update-contract`,
`cpp-tck.service-report-register-object-instance-contract`, and
`cpp-tck.service-report-delete-object-instance-contract` runners expose the
ordinary MOM service-report success routes as independently selectable pure C++
slices using adapter-supplied standard MIM/FOM inputs.
The promoted `cpp-tck.service-report-synchronization-contract` runner adds the
same pure standard boundary for synchronization-point registration,
confirmation, announcement, achievement, and completion reports. Its focused
installed-package lane passed 4/4 callback-model cases; the source uses only
official IEEE C++ API headers, standard MIM data elements, and the standard
library.
The promoted `cpp-tck.service-report-interlock-contract` runner adds the same
pure standard boundary for the service-reporting switch and
`HLAreportServiceInvocation` subscription interlocks. Its focused
installed-package lane passed 4/4 callback-model cases; the provider package,
FOM, endpoint, and callback configuration remain adapter-owned.
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

The promoted `cpp-tck.directed-interaction-declarations-valid-inputs-after-resignation`
scenario and contract twin cover all six non-region class-scoped directed-
interaction publication/subscription overloads. They resolve the valid
object-class/interaction-class association from the adapter FOM, verify each
declaration succeeds while joined, and then verify
`FederateNotExecutionMember` after resignation with the same handles under both
callback models. The direct installed-package JSON/JUnit evidence is in
`.build/cpp-tck-directed-interaction-declarations-candidate/`, and the focused
verified CTest pair passed 4/4.

The promoted `cpp-tck.interaction-transportation-query-valid-inputs-after-resignation`
scenario and contract twin query a valid active peer's published adapter-FOM
interaction class, verify the `reportInteractionTransportationType` callback
and resolvable type handle, then verify `FederateNotExecutionMember` after the
requester resigns while the peer remains active. The direct installed-package
JSON/JUnit evidence is in
`.build/cpp-tck-interaction-transportation-query-candidate/`, and the focused
verified CTest pair passed 4/4.

The promoted `cpp-tck.support-switch-getter-lifecycle-boundaries` scenario and
contract twin call all 13 standard boolean support-switch getters while joined,
then verify `FederateNotExecutionMember` after resignation and `NotConnected`
after the federation is destroyed and the connection is closed. They do not
assume any provider-specific getter defaults or exercise region behavior. The
direct installed-package JSON/JUnit evidence is in
`.build/cpp-tck-support-switch-getters-candidate/`; focused candidate and
verified CTest runs passed 4/4 each for both callback models. At that checkpoint,
the catalog contained 796 scenario IDs (794 promoted, 2 candidates).

The promoted `cpp-tck.automatic-resign-directive-lifecycle-boundaries` pair
round-trips `DELETE_OBJECTS` while joined, restores `NO_ACTION` before resigning,
then verifies the standard membership and connection exceptions for both the
getter and setter (§§10.44–10.45). Direct installed-package JSON/JUnit evidence
and the focused candidate and verified CTest runs each passed 4/4 across both
callback models.

At that checkpoint, the catalog contained 798 scenario IDs (796 promoted, 2 candidates). One
remaining candidate is `cpp-tck.federate-lost-mom-report`. An initial adapter
attempt used an older install prefix and failed during federation creation;
fresh-prefix follow-up and the current blocker are recorded in the handoff below.
The pair remains unpromoted. Separately, the
publication-dependent order/transportation change cases still need a
canonical-clause precedence check; the official API header lists their
publication and membership exceptions without specifying precedence, so no
membership-only assertion has been added.

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
removed-object counter, and the ordinary removal callback. It requests a
one-second `HLAsetTiming` period for the observer and verifies that the
periodic reflection preserves the count, then disables periodic reporting. It
also exercises the same RTI-owned MOM object through
`RequestAttributeValueUpdate` with a standard `HLAreportServiceInvocation`
report, checking the MIM-defined report parameters without treating the
`HLAfederate` routing dimension as a parameter. The scenario uses only the
adapter-supplied standard MIM/FOM and official IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-removed-object-count-contract` runner
exposes that RTI-owned joined-federate MOM object-removal route as an
independently selectable pure standard C++ contract. It keeps the
`HLAfederateHandle`/`HLAobjectInstancesRemoved` ownership and reflection checks,
the direct and periodic count values, receive-order removal callback, standard
service report, and cleanup while taking the provider, FOM, MIM, endpoint,
callback, and logical-time configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-registered-object-count` scenario uses
the standard joined-federate MOM object to request
`HLAobjectInstancesRegistered` directly. It verifies the `HLAinteger32BE`
count at zero before registration, one after the first successful adapter-supplied
object registration, and two after the second, along with standard handle/name
lookups, reliable RTI-originated reflection metadata, and provider-neutral
cleanup in both callback models. It also configures the standard `HLAsetTiming`
period for the represented owner and checks a periodic reflection of two after
the second registration before disabling periodic reporting.

The promoted `cpp-tck.joined-federate-mom-registered-object-count-contract`
runner exposes the direct and periodic MOM registration-count route as an
independently selectable pure standard C++ contract. It takes the provider,
FOM, standard MIM, endpoint, callback, logical-time, and application object
configuration from the adapter; periodic delivery is enabled and disabled
through standard `HLAsetTiming` interactions, without private headers,
registries, or diagnostics.

The promoted `cpp-tck.joined-federate-mom-object-lifecycle` pair observes the
standard `HLAmanager.HLAfederate` object through ordinary active subscription.
It checks discovery and public object-name/class lookups, verifies the reflected
federate handle/name/type, a non-empty required `HLAfederateHost` Unicode value,
and `HLAFOMmoduleDesignatorList` without asserting a platform-specific hostname
spelling, requests the values again through the discovered object handle, and
confirms removal when two joined-federate lifetimes resign. One Join supplies
the adapter's additional FOM module and the other supplies none, so the
expected designator lists are adapter-derived rather than provider-specific.
The scenario passes in both callback models against the installed package. It is currently a C++-only
extension; the Java TCK has no matching lifecycle scenario yet.

The promoted `cpp-tck.joined-federate-mom-time-state-durations` scenario uses
the standard joined-federate MOM object to observe `HLAtimeGrantedTime` and
`HLAtimeAdvancingTime` through direct AVU and one `HLAsetTiming` periodic
reflection. It verifies official four-octet nonnegative `HLAinteger32BE`
durations and reliable RTI-originated metadata in both callback models using
only the adapter-supplied standard MIM/FOM and official IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-time-state-durations-contract` runner
exposes that joined-federate MOM time-state route as an independently selectable
pure standard C++ contract. It keeps direct and periodic duration reporting,
official `HLAinteger32BE` decoding, RTI-originated reflection metadata,
adapter-supplied logical-time and standard MIM boundaries, and teardown without
provider-specific headers or diagnostics.

The promoted `cpp-tck.joined-federate-mom-galt-lits-periodic` scenario uses the
standard joined-federate MOM object to verify direct and periodic `HLAGALT` and
`HLALITS` values, including the undefined-value boundary after the sole time
regulator is disabled. It uses standard MIM/FOM lookup, `HLAsetTiming`, and
official logical-time/encoder types in both callback models.

The promoted `cpp-tck.joined-federate-mom-galt-lits-periodic-contract` runner
exposes that joined-federate MOM time-statistics route as an independently
selectable pure standard C++ contract. It keeps the direct and periodic
`HLAGALT`/`HLALITS` reflections, active-regulator and undefined-value
boundaries, and standard cleanup while taking the provider, FOM, MIM,
endpoint, callback, and logical-time configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-tso-length-periodic` scenario uses
the standard `HLATSOlength` MOM attribute to verify direct and periodic queued
timestamped-interaction counts, then verifies the count returns to zero after
the timestamp is granted. The interaction and parameter come from the adapter
FOM; all MOM, time, callback, and encoding behavior is exercised through the
official IEEE C++ API in both callback models.

The promoted `cpp-tck.joined-federate-mom-tso-length-periodic-contract` runner
exposes that joined-federate MOM queue-statistics route as an independently
selectable pure standard C++ contract. It keeps the direct and periodic
`HLATSOlength` reflections, queued timestamped interaction and post-grant
cleanup checks, and standard teardown while taking the provider, FOM, MIM,
endpoint, callback, and logical-time configuration from the adapter.

The TSO-length traceability follow-up maps the standard MIM declaration at
`HLAstandardMIM-2025.xml` lines 308-317 to IEEE 1516.1-2025 clause 11.4.1.
Two clean installed-package consumer builds passed the base and contract IDs
under both callback models; the second build also passed the corresponding
`HLAROlength` pair. This is repeated evidence against the same installed
provider, not independent-provider or conformance evidence.

The promoted `cpp-tck.joined-federate-mom-ro-length-periodic` scenario uses
the adapter-supplied ordinary interaction to hold receive-order deliveries,
checks direct and `HLAsetTiming`-periodic `HLAROlength` values, and verifies the
queue returns to zero after callback delivery. It exercises evoked and
immediate callback models using only the official C++ API. Two independent
installed-package consumer builds each passed all four focused CTest entries
(two scenario IDs across both callback models) and direct runs for both IDs
across both models. These are repeated runs against the same installed provider,
not independent-provider or conformance evidence.

The promoted `cpp-tck.joined-federate-mom-updates-sent-counts` scenario uses the
adapter-supplied ordinary object class and attribute with standard reliable and
best-effort transportation. It requests `HLArequestUpdatesSent` and verifies
the two `HLAreportUpdatesSent` transport buckets, nested standard
`HLAobjectClassBasedCounts` decoding, RTI-originated metadata, and the empty
response for a requester with no sent updates in both callback models.

The promoted `cpp-tck.joined-federate-mom-updates-sent-counts-contract` runner
exposes that joined-federate MOM update-accounting route as an independently
selectable pure standard C++ contract. It keeps the reliable and best-effort
transport buckets, nested standard `HLAobjectClassBasedCounts` decoding,
transportation-change confirmation, RTI-originated metadata, empty-response
boundary, and standard teardown while taking the provider, FOM, MIM, endpoint,
callback, and application class configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-interactions-received-counts` scenario
delivers one adapter-supplied ordinary interaction reliably and two after a
standard best-effort transport change. It requests
`HLArequestInteractionsReceived` and verifies the two
`HLAreportInteractionsReceived` transport buckets, nested standard
`HLAinteractionCounts` decoding, receiver metadata, and the empty response for
a requester with no received interactions in both callback models.

The promoted `cpp-tck.joined-federate-mom-interactions-received-counts-contract`
runner exposes that joined-federate MOM interaction-accounting route as an
independently selectable pure standard C++ contract. It keeps the reliable and
best-effort transport buckets, nested standard `HLAinteractionCounts` decoding,
transportation-change confirmation, receiver metadata, empty-response
boundary, and standard teardown while taking the provider, FOM, MIM, endpoint,
callback, and application interaction configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-interactions-sent-counts` scenario
sends one adapter-supplied ordinary interaction reliably and two after a
standard best-effort transport change. It requests
`HLArequestInteractionsSent` and verifies the two
`HLAreportInteractionsSent` transport buckets, nested standard
`HLAinteractionCounts` decoding, sender metadata, and the empty response for an
idle joined federate in both callback models.

The promoted `cpp-tck.joined-federate-mom-interactions-sent-counts-contract`
runner exposes that joined-federate MOM sent-interaction accounting route as an
independently selectable pure standard C++ contract. It keeps the reliable and
best-effort transport buckets, nested standard `HLAinteractionCounts` decoding,
transportation-change confirmation, sender metadata, empty-response boundary,
and standard teardown while taking the provider, FOM, MIM, endpoint, callback,
and application interaction configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-reflections-received-counts` scenario
delivers one adapter-supplied attribute reflection reliably and two after a
standard best-effort transport change. It requests
`HLArequestReflectionsReceived` and verifies the two
`HLAreportReflectionsReceived` transport buckets, nested standard
`HLAobjectClassBasedCounts` decoding, receiver metadata, and the empty response
for a requester with no received reflections in both callback models.

The promoted `cpp-tck.joined-federate-mom-reflections-received-counts-contract`
runner exposes that joined-federate MOM reflection-accounting route as an
independently selectable pure standard C++ contract. It keeps the reliable and
best-effort transport buckets, nested standard `HLAobjectClassBasedCounts`
decoding, receiver metadata, attribute-transportation confirmation,
empty-response boundary, and standard teardown while taking the provider, FOM,
MIM, endpoint, callback, and application object/attribute configuration from the
adapter.

The promoted `cpp-tck.joined-federate-mom-reflection-counts` scenario observes
the standard joined-federate MOM `HLAobjectInstancesReflected` and
`HLAreflectionsReceived` counters through direct attribute-value requests and
periodic `HLAsetTiming` reflections. It distinguishes repeated reflections of
one object from first reflections of another, includes a timestamped reflection,
and verifies both ordinary and timestamped callback paths using only the
adapter-supplied FOM, standard MIM, logical-time implementation, and official
IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-reflection-counts-contract` runner
exposes that direct/periodic joined-federate MOM reflection route as an
independently selectable pure standard C++ contract. It keeps the repeated-versus
first-reflection distinction, `HLAsetTiming` periodic delivery, timestamped
reflection ordering, standard time-advance callbacks, nested MOM data-element
decoding, and teardown while taking the provider, FOM, MIM, endpoint, callback,
logical-time, and application object/attribute configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-received-counts`
scenario sends one ordinary and one directed interaction of the same
adapter-supplied class, proves that ordinary receipt is excluded from
`HLAreportDirectedInteractionsReceived`, and verifies the reliable directed
bucket plus empty best-effort and idle buckets through nested standard
`HLAinteractionCounts`. It uses only the adapter-supplied FOM, standard MIM,
and official IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-received-counts-contract`
runner exposes that joined-federate MOM directed-interaction accounting route as
an independently selectable pure standard C++ contract. It keeps the ordinary
versus directed delivery distinction, reliable and best-effort directed buckets,
nested standard `HLAinteractionCounts` decoding, receiver metadata,
transportation-change confirmation, empty-response boundaries, and standard
teardown while taking the provider, FOM, MIM, endpoint, callback, and
application object/interaction configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-sent-counts`
scenario sends one ordinary and two directed interactions of the same
adapter-supplied class, changes the class to best effort for an ordinary send,
and proves that only the directed sends enter
`HLAreportDirectedInteractionsSent`. It verifies the reliable directed bucket
plus empty best-effort and idle buckets through nested standard
`HLAinteractionCounts`, using only the adapter-supplied FOM/MIM and official
IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-sent-counts-contract`
runner exposes that joined-federate MOM directed-interaction sent-accounting
route as an independently selectable pure standard C++ contract. It keeps the
ordinary-versus-directed filtering, reliable and best-effort directed buckets,
nested standard `HLAinteractionCounts` decoding, sender metadata,
transportation-change confirmation, empty-response boundaries, and standard
teardown while taking the provider, FOM, MIM, endpoint, callback, and
application object/interaction configuration from the adapter.

The promoted `cpp-tck.mom-transportation-type-change-request` scenario uses the
standard MIM request interactions for attribute and interaction transportation-
type changes. It verifies the corresponding confirmation callbacks, ordinary
object reflection and interaction delivery under best effort, reliable
per-federate transport isolation, and the RTI-originated service report. The
request payloads use official standard encodings while the adapter supplies the
FOM, MIM, endpoint, callback model, and logical-time implementation.

The promoted `cpp-tck.mom-transportation-type-change-request-contract` runner
exposes the same standard MIM transportation-change request, confirmation,
delivery-isolation, service-reporting, and cleanup checks as an independently
selectable pure standard C++ contract. Provider, FOM, MIM, endpoint, callback,
and application configuration remain adapter inputs.

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
The promoted `cpp-tck.service-report-receive-order-interaction` scenario and
its contract twin extend that route with an independent ordinary receiver. The
focused installed-package lane passed 4/4 cases across `HLA_EVOKED` and
`HLA_IMMEDIATE`, verifying callback timing, standard MOM report payloads, and
  application interaction metadata with only adapter-supplied FOM/MIM inputs.
  The promoted `cpp-tck.service-report-timestamped-directed-interaction` scenario
  and its contract twin extend that route through time-regulated timestamped
  `SendDirectedInteraction`. Their focused installed-package lane passed 4/4
  cases across `HLA_EVOKED` and `HLA_IMMEDIATE`, verifying typed MOM supplied and
  returned arguments, retraction identity, target routing, and callback-before-
   grant ordering with a five-epsilon lookahead. The source remains limited to
   the official IEEE C++ API, standard MIM data elements, and the standard library.
The promoted `cpp-tck.service-report-timestamped-attribute-update` scenario and
its contract twin extend that boundary through a time-regulated timestamped
`UpdateAttributeValues`. Their focused installed-package lane passed 4/4 cases
across `HLA_EVOKED` and `HLA_IMMEDIATE` in
`.build\\cpp-tck-timestamped-attribute-report\\timestamped-attribute-service-report.json`,
verifying typed MOM supplied and returned arguments, the optional timestamp,
retraction identity, timestamped reflection metadata, and reflection-before-
grant ordering. The source remains limited to the official IEEE C++ API,
standard MIM data elements, and the standard library.
The promoted `cpp-tck.service-report-timestamped-delete-object-instance`
scenario and its contract twin extend that boundary through a time-regulated
timestamped `DeleteObjectInstance`. Their focused installed-package lane passed
4/4 cases across `HLA_EVOKED` and `HLA_IMMEDIATE` in
`.build\\cpp-tck-timestamped-delete-service-report\\timestamped-delete-service-report.json`,
verifying typed MOM supplied and returned arguments, the optional timestamp,
retraction identity, timestamped removal metadata, and removal-before-grant
ordering. The source remains limited to the official IEEE C++ API, standard MIM
data elements, and the standard library.
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
The promoted `cpp-tck.service-report-object-attribute-declaration` scenario and
its contract twin translate the public ordinary declaration service-report
matrix: five invalid-handle calls followed by subscribe, attribute-set
unsubscribe, and whole-class unsubscribe success cases. Their focused
installed-package lane passed 4/4 callback-model cases; the ordinary FOM,
standard MIM, provider, endpoint, and callback configuration remain adapter
inputs. The catalog-wide recheck reproduced 20 failures in five pre-existing
scenario families, independently of this four-case slice, so it is not recorded
as a new green aggregate.
The promoted `cpp-tck.service-report-directed-interaction-declaration` scenario
and its contract twin translate the corresponding public directed declaration
matrix: invalid object and interaction handles for subscription and
unsubscription, followed by selective subscription, selective unsubscribe, and
whole-class unsubscribe. Their focused installed-package lane passed 4/4
callback-model cases; the ordinary adapter-supplied FOM already declares the
directed interaction, so no provider-specific fixture or portable-source header
is involved.
The promoted `cpp-tck.service-report-directed-interaction-publication` scenario
and its contract twin cover the matching publication matrix: invalid object and
interaction handles, successful selective publication, selective unpublication,
and whole-class unpublication. Their focused installed-package lane passed 8/8
callback-model cases together with the declaration slice, using only the
adapter-supplied ordinary FOM and standard MIM.
The promoted `cpp-tck.service-report-interaction-publication` and
`cpp-tck.service-report-interaction-subscription` scenarios, together with their
contract twins, cover the ordinary interaction-class publication and subscription
declaration reports. Their focused installed-package lane passed 8/8 callback-
model cases across `HLA_EVOKED` and `HLA_IMMEDIATE`, including invalid-handle
failures, successful services, typed Null returns, serial progression, and the
standard passive-subscription indicator. The source uses only official IEEE C++
API headers and the standard library; provider, FOM, MIM, endpoint, and callback
configuration remain adapter inputs.
The promoted `cpp-tck.federation-mom-save-conditionals-contract` and
`cpp-tck.joined-federate-mom-federate-state-save-restore-contract` runners
likewise expose the save/restore MOM routes as independently selectable pure
standard C++ contracts. Their focused four-scenario lane passed 8/8
callback-model cases; the catalog-wide aggregate passed 666/666 CTest cases
plus 666 direct passes with only the two expected connection-loss skips.
The ownership contract-twin lane passed 8/8 callback-model cases in
`.build\\cpp-tck-all\\verified-evidence-standard-ownership-cancellation-continuation.json`.
The promoted timed regular-candidate contract twin passed 4/4 callback-model
cases in its focused lane. The timed confirmation-cancellation contract twin
also passed 4/4 callback-model cases, and the timed pre-delivery cancellation
contract twin passed 4/4. Evoked mode retains each full cancellation route;
immediate mode verifies the standard unavailable cancellation-window boundary
and synchronous ordinary ownership handoff after queued delivery.
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

The promoted `cpp-tck.attribute-value-update-request-multi-requester` scenario
extends that route to two active requesters. It preserves their independent
request tags in separate provider callbacks, verifies that the provider
callback does not loop back to either requester, and checks that the ordinary
response reaches both subscribers with standard value, tag, producer,
transportation, and no-region metadata. Its corresponding
`cpp-tck.attribute-value-update-request-multi-requester-contract` runner
exposes the same boundary as an independently selectable pure standard C++
contract while taking provider, FOM, endpoint, and callback configuration from
the adapter.

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

The promoted `cpp-tck.regional-attribute-update-callback-ddm-recheck` scenario
and its pure contract twin cover direct ordinary regional Update/Reflect
eligibility at callback delivery after a committed subscription-region mutation.
They verify evoked queued-callback suppression, immediate-delivery boundaries,
restored-overlap delivery, source-region designators, and value/tag/producer/
transportation metadata. The focused lane passed 4/4 callback-model cases; the
candidate-inclusive catalog gate passed all 818 CTest cases and recorded 816
direct passes plus the two expected adapter-managed connection-loss skips. The
source uses only official IEEE C++ headers and the standard library, with
provider, dimensional FOM, endpoint, callback, and logical-time configuration
supplied by the adapter.

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

The promoted `cpp-tck.directed-interaction-multi-recipient-fifo` scenario sends
two ordinary directed interactions to one registered target and verifies that
two active universal subscribers receive both in producer order with stable
target, parameter, tag, producer, and transportation metadata; the sender
receives no loopback. The corresponding
`cpp-tck.directed-interaction-multi-recipient-fifo-contract` runner exposes the
same fan-out behavior as an independently selectable pure standard C++ contract
using only adapter-supplied provider, FOM, endpoint, and callback configuration.

The promoted `cpp-tck.directed-interaction-mixed-subscription-fanout` scenario
combines one by-ownership subscriber, two universal subscribers, and one
unsubscribed observer. It verifies ordered metadata-preserving delivery for a
shared target and confirms that a second target reaches only the universal
subscribers. Its corresponding contract runner remains limited to official API
headers, the standard library, and adapter-supplied provider/FOM/endpoint/
callback configuration.

The promoted `cpp-tck.directed-interaction-subscription-kind-contract`
runner exposes the by-ownership and universal directed-interaction
subscription boundary as an independently selectable pure standard C++
contract. It uses only official API headers and the standard library while
taking provider, FOM, endpoint, and callback configuration from the adapter.

The promoted `cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report`
scenario exercises the standard joined-federate MOM request/report route for
`HLArequestObjectInstancesThatCanBeDeleted` and
`HLAreportObjectInstancesThatCanBeDeleted`. It registers two adapter-supplied
objects, verifies the public standard `HLAobjectClassBasedCounts` decoding at
two, one, and zero deletable instances, and checks reliable report metadata and
ordinary deletion cleanup under both callback models. Its contract twin uses
only official `RTIambassador`/`FederateAmbassador` APIs, standard MIM names and
data elements, the C++ standard library, and adapter-supplied provider, FOM,
endpoint, callback, and logical-time configuration.

The promoted `cpp-tck.joined-federate-mom-object-instances-updated-report`
scenario requests the standard update-responsibility report for a subject that
updates two adapter-supplied object instances, repeating one instance's update.
It decodes the standard `HLAobjectClassBasedCounts` fixed records through public
standard APIs and verifies that the result is one class entry with a count of
two—not three update invocations. The adapter's selected attribute is declared
as standard `HLAbyte`, matching the portable P0 FOM. Combined candidate and
separate fresh verified installed-package builds of both MOM count pairs passed
8/8 CTest cases across evoked and immediate callback models. Direct results are
in
`.build/cpp-tck-joined-federate-mom-object-count-reports-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-joined-federate-mom-object-count-reports-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The promoted `cpp-tck.joined-federate-mom-object-instances-reflected-report`
scenario uses three joined federates: a subject registers two instances and
sends three receive-order updates, one target actively subscribes and receives
all three reflect callbacks, and a distinct requester addresses that target
through the standard `HLAfederate` request parameter. The requester decodes
`HLAobjectClassBasedCounts` with public APIs and verifies one reliable class
entry for two distinct instances despite the repeated reflection. The
adapter-selected attribute is standard `HLAbyte`; no region APIs are used.
The combined candidate and separate installed-package verification builds each
passed 8/8 CTest cases across evoked and immediate callback models. Direct
results are in
`.build/cpp-tck-joined-federate-mom-object-count-reports-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-joined-federate-mom-object-count-reports-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The promoted `cpp-tck.joined-federate-mom-object-instances-reflected-timestamped-report`
checks the pinned MIM rule that counts instances for which the target has had a
Reflect Attribute Values invocation, with no exclusion for timestamp order. A
subject sends three timestamped updates—two for one object and one for another—
to a time-constrained target; the target validates each callback and its
delivery-before-grant ordering. A distinct requester queries that target and
the test expects one reliable class-count record for two distinct instances.

The promoted `cpp-tck.joined-federate-mom-object-instances-updated-timestamped-report`
extends the updated-object MOM report through standard timestamped delivery. A
subject registers two objects, sends a timestamped update for each, and the
constrained observer validates both `reflectAttributeValues` records, including
logical time and retraction metadata. A third federate requests the standard
`HLAreportObjectInstancesUpdated`; the test decodes the MIM's class-based counts
with public standard APIs and expects two distinct updated instances. A fresh
candidate build passed 4/4 focused CTest cases across evoked and immediate
callbacks; an independent fresh installed-package build passed 28/28 cases for
the updated/reflected MOM regression family. Candidate and verification evidence
is in
`.build/cpp-tck-mom-updated-tso-candidate-20260927-a/candidate-results.json`
and
`.build/cpp-tck-mom-updated-tso-verified-20260927-b/verified-candidate-only-results.json`;
both scenario-isolated evidence checks passed.

The promoted `cpp-tck.joined-federate-mom-updated-object-count-periodic` pair
checks direct and `HLAsetTiming`-periodic values for
`HLAobjectInstancesRegistered`, `HLAobjectInstancesUpdated`, and
`HLAupdatesSent`. It samples the counts before and after two registrations,
repeated updates to one object, and an update to a second object; the final
snapshot is 2 registered objects, 2 distinct updated objects, and 3 update
invocations. Two fresh installed-package builds passed the focused 4/4 CTest
matrix under evoked and immediate callbacks, and direct evidence passed for both
callback models.

The promoted `cpp-tck.joined-federate-mom-discovered-object-count-periodic`
pair checks direct count deltas for two application object discoveries and a
third discovery after the receiver locally deletes and then rediscovers the
first object. It also verifies the standard `HLAsetTiming` periodic report and
MOM reflection metadata. The direct baseline is sampled after joined-federate
MOM discovery setup, avoiding an assumption about whether the provider includes
those setup discoveries. Two fresh installed-package consumer builds each
passed the focused 4/4 CTest matrix under evoked and immediate callbacks; direct
evidence also passed for both scenarios and callback models in each build.

The promoted `cpp-tck.joined-federate-mom-deleted-object-count-periodic` pair
checks direct `HLAobjectInstancesDeleted` count deltas after two successful
Delete Object Instance calls and its final `HLAsetTiming` periodic value. Each
deletion is correlated with an ordinary removal callback at a second federate;
the initial count is treated as a baseline rather than assuming provider-
specific setup accounting. Two fresh installed-package consumer builds each
passed the focused 4/4 CTest matrix under evoked and immediate callbacks, and
direct evidence passed for both scenarios and callback models in each build.

The promoted `cpp-tck.joined-federate-mom-deletable-object-count` scenario
exercises the standard joined-federate MOM
`HLAobjectInstancesThatCanBeDeleted` attribute through
`requestAttributeValueUpdate`. It verifies typed reliable reflections for
zero, one, and two live objects, then one and zero after ordinary deletion.
At the two-object state it sends the standard `HLAsetTiming` interaction for
the represented owner and verifies the periodic MOM reflection preserves the
count before disabling the period.
Its contract twin uses only official `RTIambassador`/`FederateAmbassador`
APIs, standard MIM names and data elements, the C++ standard library, and
adapter-supplied provider, FOM, endpoint, callback, and logical-time
configuration.

The promoted `cpp-tck.receive-order-attribute-update` scenario exercises the
ordinary public attribute-update path with two consecutive updates from an
adapter-supplied publisher to an active subscriber. It verifies FIFO reflection
ordering, object/attribute/value/tag/producer/transportation metadata, no
publisher loopback, and standard cleanup in both callback models. Its contract
twin uses only official `RTIambassador`/`FederateAmbassador` APIs and the
standard library; provider, FOM, endpoint, and callback configuration remain
adapter inputs.

The promoted `cpp-tck.receive-order-interaction` scenario exercises the
ordinary public interaction path with two consecutive sends from an
adapter-supplied publisher to an active subscriber. It verifies FIFO delivery,
class/parameter/value/tag/producer/transportation metadata, no sender
loopback, and standard cleanup in both callback models. Its contract twin uses
only official `RTIambassador`/`FederateAmbassador` APIs and the standard
library; provider, FOM, endpoint, and callback configuration remain adapter
inputs.

The promoted `cpp-tck.receive-order-object-removal` scenario exercises the
ordinary public object-removal path after adapter-supplied registration and
discovery. It verifies the removed object identity, removal tag, producer
metadata, no publisher loopback, and standard cleanup in both callback models.
Its contract twin uses only official `RTIambassador`/`FederateAmbassador` APIs
and the standard library; provider, FOM, endpoint, and callback configuration
remain adapter inputs.

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

The promoted `cpp-tck.support-switch-setter-lifecycle-boundaries` scenario and
contract twin exercise all eight writable standard support-switch setters,
using observed getter values while joined and checking the declared membership
and connection exceptions after resignation and disconnect. No provider
defaults are assumed; the adapter avoids service-report subscriptions for the
Set Service Reporting call. Candidate and verified CTest runs each passed 4/4
across evoked and immediate callback models, and the installed-package JSON and
JUnit evidence is in
`.build/cpp-tck-support-switch-setters-verified/`. At that checkpoint the catalog had 800
scenario IDs (798 promoted, 2 candidates). API-surface and Java-parity audits
remain valid; the two remaining candidate IDs are the federate-lost report
scenario and its contract twin.

The promoted `cpp-tck.automatic-resign-directive-enum-values` scenario and
contract twin round-trip all six `ResignAction` enumerators and verify
`InvalidResignAction` for the next value. They restore `NO_ACTION` before
resignation and do not test object or ownership cleanup. Candidate and verified
CTest runs each passed 4/4 across evoked and immediate callback models; direct
installed-package JSON/JUnit evidence is in
`.build/cpp-tck-automatic-resign-values-verified/`. At that checkpoint, the catalog had 802
scenario IDs (800 promoted, 2 candidates).

The promoted `cpp-tck.advisory-switch-value-round-trips` scenario and contract
twin toggle each of the four core relevance/scope advisory switches from its
observed value, verify the changed value, and restore it. They generate no
object, interaction, or region traffic. Candidate and verified CTest runs each
passed 4/4 across evoked and immediate callback models; installed-package
JSON/JUnit evidence is in
`.build/cpp-tck-advisory-switch-values-verified/`. At that checkpoint, the catalog had 804
scenario IDs (802 promoted, 2 candidates).

The promoted `cpp-tck.duplicate-federate-name-join-boundary` scenario and
contract twin verify the standard `FederateNameAlreadyInUse` response for an
active name, followed by a successful join under a distinct name. The scenario
reuses the `java-tck.federation-membership` parity target. Candidate and
verified CTest runs each passed 4/4 across evoked and immediate callback models;
installed-package JSON/JUnit evidence is in
`.build/cpp-tck-duplicate-federate-name-verified/`. At that checkpoint, the catalog had 806
scenario IDs (804 promoted, 2 candidates).

The promoted `cpp-tck.repeated-join-by-member-boundary` scenario and contract
twin require `FederateAlreadyExecutionMember` when one joined ambassador repeats
the named join, then confirm the original membership can resign normally. It
reuses `java-tck.federation-membership`. Candidate and verified CTest runs each
passed 4/4 across evoked and immediate callback models; installed-package
JSON/JUnit evidence is in `.build/cpp-tck-repeated-join-verified/`. At that
checkpoint, the catalog had 808 scenario IDs (806 promoted, 2 candidates).

The promoted `cpp-tck.foreign-federate-name-lookup-boundary` scenario and
contract twin join federates in separate executions, confirm each name resolves
through its own execution, and require `NameNotFound` when the other execution's
name is queried (§10.2). This is a C++ TCK extension with no direct Java parity
target. Candidate and verified installed-package CTest runs each passed 4/4
across evoked and immediate callback models; direct JSON/JUnit evidence is in
`.build/cpp-tck-foreign-federate-name-verified/`. The catalog now has 810
scenario IDs (808 promoted, 2 candidates).

The promoted `cpp-tck.foreign-object-instance-name-lookup-boundary` scenario and
contract twin reserve and register a named object in each of two executions,
confirm each name resolves locally, and require `ObjectInstanceNotKnown` for a
name belonging only to the other execution (§10.7). The standard name-reservation
callback is serviced in both callback models. This is a C++ TCK extension with
no direct Java parity target. Candidate and verified installed-package CTest
runs each passed 4/4; direct JSON/JUnit evidence is in
`.build/cpp-tck-foreign-object-name-verified/`. At that checkpoint, the catalog
had 812 scenario IDs (810 promoted, 2 candidates).

The promoted `cpp-tck.foreign-object-instance-handle-lookup-boundary` scenario
and contract twin register one local object and two objects in another
execution. Selecting a foreign handle unequal to the local handle avoids
assuming handles are globally unique; `getObjectInstanceName` resolves it in
the foreign execution and returns `ObjectInstanceNotKnown` locally (§10.8).
This C++ extension has no direct Java parity target. Candidate and verified
installed-package CTest runs each passed 4/4 across both callback models; direct
JSON/JUnit evidence is in
`.build/cpp-tck-foreign-object-handle-verified/`. The catalog now has 814
scenario IDs (812 promoted, 2 candidates).

The promoted `cpp-tck.local-delete-object-instance-name-lookup-boundary`
scenario and contract twin confirm that a subscriber's `getObjectInstanceName`
returns `ObjectInstanceNotKnown` after that federate locally forgets a discovered
object, while the publishing federate can still resolve it (§6.18 and §10.8).
This C++ extension has no direct Java parity target. Candidate and verified
installed-package CTest runs each passed 4/4 across evoked/immediate callback
models; direct JSON/JUnit evidence is in
`.build/cpp-tck-local-delete-name-candidate/` and
`.build/cpp-tck-local-delete-name-verified/`. The catalog now has 816 scenario
IDs (814 promoted, 2 candidates).

The promoted `cpp-tck.remote-delete-known-object-class-lookup-boundary` scenario
and contract twin wait for the standard `removeObjectInstance` callback after
ordinary remote deletion, then verify `getKnownObjectClassHandle` reports
`ObjectInstanceNotKnown` for the removed handle both to the deleting federate
and the subscriber (§6.16, §6.17, §10.6). This C++
extension has no direct Java parity target. Candidate and verified
installed-package CTest runs each passed 4/4 across evoked/immediate callback
models; direct JSON/JUnit evidence is in
`.build/cpp-tck-remote-delete-class-candidate/` and
`.build/cpp-tck-remote-delete-class-verified/`.

The promoted `cpp-tck.remote-delete-object-instance-name-lookup-boundary`
scenario and contract twin cover the inverse directions left out by the
existing deletion scenario: the deleting federate's `getObjectInstanceHandle`
after delete returns, and the subscriber's `getObjectInstanceName` after its
removal callback, both report `ObjectInstanceNotKnown` (§6.16–6.17, §10.7–10.8).
This C++ extension has no direct Java parity target. Candidate and verified
installed-package CTest runs each passed 4/4 across evoked/immediate callback
models; direct JSON/JUnit evidence is in
`.build/cpp-tck-remote-delete-name-candidate/` and
`.build/cpp-tck-remote-delete-name-verified/`. The catalog now has 820 scenario
IDs (818 promoted, 2 candidates).

The promoted `cpp-tck.resign-delete-object-lookup-boundary` scenario and
contract twin resign the object owner with `DELETE_OBJECTS`, wait until the
surviving federate receives `removeObjectInstance`, and verify that
`getObjectInstanceHandle`, `getObjectInstanceName`, and
`getKnownObjectClassHandle` all report `ObjectInstanceNotKnown` for the removed
object (§4.12, §6.17, §§10.6–10.8). This C++ extension has no direct Java parity
target. Candidate and verified installed-package CTest runs each passed 4/4
across evoked/immediate callback models; direct JSON/JUnit evidence is in
`.build/cpp-tck-resign-delete-lookup-candidate/` and
`.build/cpp-tck-resign-delete-lookup-verified/`. The catalog now has 822
scenario IDs (820 promoted, 2 candidates).

The promoted `cpp-tck.resign-no-action-owned-attributes-rejection-boundary`
scenario and contract twin confirm §4.12's `FederateOwnsAttributes` rejection
when an owner requests `NO_ACTION` while another federate remains joined. The
test also confirms the rejected resignation leaves the object queryable, then
performs a successful `DELETE_OBJECTS` resignation. Keeping a second member in
the federation avoids the separate final-federate rule. No direct Java TCK case
currently covers this exception boundary. Candidate and verified
installed-package CTest runs each passed 4/4 across evoked/immediate callback
models; direct JSON/JUnit evidence is in
`.build/cpp-tck-resign-no-action-two-member-candidate/` and
`.build/cpp-tck-resign-no-action-two-member-verified/`. The catalog has 824
scenario IDs at that checkpoint (822 promoted, 2 candidates).

The promoted `cpp-tck.resign-delete-objects-then-divest-cross-object-effects`
scenario and contract twin exercise `DELETE_OBJECTS_THEN_DIVEST` across distinct
object instances: both surviving subscribers observe removal of the departing
federate's registered object, while the other federate's object remains
queryable and its formerly owned attribute can be acquired. Candidate and
independent installed-package builds each passed 4/4 tests across evoked and
immediate callback models. Direct JSON/JUnit evidence is in
`.build/cpp-tck-resign-delete-then-divest-candidate/` and
`.build/cpp-tck-resign-delete-then-divest-verified/`. This C++ extension has no
direct Java counterpart. The catalog now has 826 scenario IDs (824 promoted,
2 candidates).

The promoted `cpp-tck.resign-unconditionally-divest-attributes-preserves-objects`
scenario and contract twin verify that a member can resign with
`UNCONDITIONALLY_DIVEST_ATTRIBUTES` while owning an attribute on another
member's registered object. The object remains known, no removal callback is
observed, and a surviving federate acquires the released attribute with the
expected callback tag. Candidate and independent installed-package builds each
passed 4/4 tests across evoked and immediate callback models. Direct JSON/JUnit
evidence is in `.build/cpp-tck-resign-unconditional-divest-candidate/` and
`.build/cpp-tck-resign-unconditional-divest-verified/`. The catalog now has 828
scenario IDs (826 promoted, 2 candidates).

The promoted `cpp-tck.resign-cancel-pending-ownership-acquisitions-action`
scenario and contract twin issue a regular ownership request, resign its
requester with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and then release the
attribute from the original owner. A different joined federate acquires it,
showing the resigned request left no stale acquisition state. Candidate and
independent installed-package builds each passed 4/4 tests across evoked and
immediate callback models. Direct JSON/JUnit evidence is in
`.build/cpp-tck-resign-cancel-pending-acq-candidate/` and
`.build/cpp-tck-resign-cancel-pending-acq-verified/`. The catalog now has 830
scenario IDs (828 promoted, 2 candidates). This C++ extension has no direct
Java counterpart.

The promoted `cpp-tck.resign-cancel-then-delete-then-divest-cross-object-effects`
scenario and contract twin exercise all three effects of the compound action.
The departing member has a pending regular acquisition on the owner's second
object, owns an attribute on the owner's retained object, and has registered a
separate object. Both surviving subscribers observe exactly one removal for
the departing object; both owner objects remain known; a successor acquires
the divested attribute and then acquires the other attribute after its owner
releases it, showing the resigned request left no stale reservation. Candidate
and independent fresh installed-package builds each passed 4/4 tests across
evoked and immediate callback models. Direct JSON/JUnit evidence is in
`.build/cpp-tck-resign-cancel-delete-divest-candidate/corrected-results.json`
and `.build/cpp-tck-resign-cancel-delete-divest-verified/results.json`. This
C++ extension has no direct Java counterpart. The catalog now has 844 scenario
IDs (842 promoted, 2 candidates).

The portable ownership suite now has the isolated positive negotiated-
divestiture slice `cpp-tck.negotiated-divestiture-confirmation-flow` and its
contract twin. They check the acquisition tag on the owner's confirmation
request, retained ownership before confirmation, the owner's confirmation tag
on the successor's acquisition notification, and ownership at both federates
after transfer. The pair maps to the existing Java parity ID
`java-tck.ownership`; provider, FOM, endpoint, and callback configuration remain
adapter inputs. Both the candidate and fresh promoted `verified` installed-
package lanes passed 4/4 cases across evoked and immediate callback models. The
promoted evidence is in
`.build/cpp-tck-negotiated-divestiture-verified/results.json` and
`.build/cpp-tck-negotiated-divestiture-verified/results.xml`.

The ordinary release-denial route is now isolated as
`cpp-tck.attribute-ownership-release-denied` and its contract twin. They check
the requester tag on Request Attribute Ownership Release, the denial tag on
Attribute Ownership Unavailable, and unchanged ownership after the denial. The
pair maps to Java parity ID `java-tck.ownership` and passed both the candidate
and fresh promoted `verified` installed-package lanes (4/4 callback-model
cases). Evidence is in `.build/cpp-tck-release-denied-verified/results.json`
and `.build/cpp-tck-release-denied-verified/results.xml`.

The ordinary one-acquirer Divestiture If Wanted route is now isolated as
`cpp-tck.attribute-ownership-divestiture-if-wanted` and its contract twin. They
check the pending acquisition tag, exact RTI-returned transferred set, the
owner-supplied divestiture tag on the acquisition notification, and ownership
transfer. This parity-linked pair passed both candidate and fresh promoted
`verified` installed-package lanes (4/4 callback-model cases). Evidence is in
`.build/cpp-tck-if-wanted-verified/results.json` and
`.build/cpp-tck-if-wanted-verified/results.xml`.

The ordinary Attribute Ownership Query owner-report path is isolated as
`cpp-tck.attribute-ownership-query-owner-report` and its contract twin. They
check the exact object, queried attribute set, and federate handle delivered by
Inform Attribute Ownership without changing ownership. The pair maps to
`java-tck.ownership` and passed the candidate and fresh promoted `verified`
installed-package lanes (4/4 callback-model cases). Evidence is in
`.build/cpp-tck-query-owner-verified/results.json` and
`.build/cpp-tck-query-owner-verified/results.xml`.

The mixed ownership-query result is isolated as
`cpp-tck.attribute-ownership-query-unowned-result` and its contract twin. With
all FOM names supplied by the adapter, the owner publishes one of two
attributes; the observer's query must return exact owned and not-owned subsets,
must not classify the ordinary unowned attribute as RTI-owned, and must leave
ownership unchanged. This pair also maps to `java-tck.ownership` and passed
candidate and fresh promoted `verified` installed-package lanes (4/4
callback-model cases). Evidence is in
`.build/cpp-tck-query-unowned-verified/results.json` and
`.build/cpp-tck-query-unowned-verified/results.xml`.

The distinct RTI-owned query result is isolated as
`cpp-tck.attribute-ownership-query-rti-owned-result` and its contract twin.
Using only the standard MIM's joined-federate object and adapter-supplied model,
MIM, and logical-time configuration, it verifies the exact
`attributeIsOwnedByRTI` callback and that the attribute is not owned by the
querying federate. This is a C++ extension with no Java counterpart. Candidate
and fresh promoted `verified` installed-package lanes both passed 4/4
callback-model cases; evidence is in
`.build/cpp-tck-query-rti-owned-verified/results.json` and
`.build/cpp-tck-query-rti-owned-verified/results.xml`.

The parity audit reports all 21 Java scenario IDs represented in the portable
C++ catalog. Java ownership's final negotiated-divestiture cancellation is
covered by the promoted `cpp-tck.negotiated-divestiture-cancellation` standard
API scenario, which verifies the transition back to the ordinary release
callback and ownership retention.

The federation-wide Auto Provide MOM setter is now covered by the promoted pair
`cpp-tck.federation-auto-provide-mom-switch` and
`cpp-tck.federation-auto-provide-mom-switch-contract`. The standard-MIM
`HLAsetSwitches` interaction is sent by joined federates to turn the switch off,
on, then off; discovery persists while disabled and exactly one correctly
scoped `provideAttributeValueUpdate` callback arrives while enabled. Candidate
and separate verified builds both passed 4/4 CTest cases across evoked and
immediate callback models. Direct results are in
`.build/cpp-tck-federation-auto-provide-mom-switch-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federation-auto-provide-mom-switch-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The joined-federate Service Reporting switch is covered by the promoted pair
`cpp-tck.federate-service-reporting-mom-switch` and its contract twin. The
standard `HLAsetSwitches` parameter is sent false-to-true-to-false and checked
through the public getter. Candidate and separate verified installed-package
builds both passed 4/4 CTest cases across evoked and immediate callback models;
direct results are in
`.build/cpp-tck-federate-service-reporting-mom-switch-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-service-reporting-mom-switch-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The joined-federate Exception Reporting switch is covered by the promoted pair
`cpp-tck.federate-exception-reporting-mom-switch` and its contract twin. The
standard MIM parameter is sent false-to-true-to-false; the public getter
confirms each transition and three neighboring switch values remain unchanged.
`HLAreportException` delivery is tested separately by
`cpp-tck.federate-exception-report-delivery`. Candidate and separate verified
installed-package builds both passed 4/4 CTest cases across evoked and
immediate callback models. Direct results are in
`.build/cpp-tck-federate-exception-reporting-mom-switch-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-exception-reporting-mom-switch-verified-20260927/verified-results.json`
(with matching JUnit XML files).

Standard `HLAreportException` delivery and Exception Reporting switch gating
are covered by the promoted pair `cpp-tck.federate-exception-report-delivery`
and its contract twin. With the switch at its standard false default, a typed
`NameNotFound` produces no report; enabling the switch through standard
`HLAsetSwitches` yields exactly one reliable RTI-originated report with service
and exception text and the exact joined-federate handle; disabling it suppresses
the next report. Candidate and separate verified installed-package builds both
passed 4/4 CTest cases across evoked and immediate callback models. Direct
results are in
`.build/cpp-tck-federate-exception-report-delivery-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-exception-report-delivery-verified-20260927/verified-results.json`
(with matching JUnit XML files).

Malformed standard MOM parameter reporting is covered by the promoted pair
`cpp-tck.federate-mom-exception-report-delivery` and its contract twin. The test
sends an out-of-range `HLAresignAction` value and an invalid `HLAswitch` value;
each produces one reliable `HLAreportMOMexception`, identifies the fully
qualified `HLAsetSwitches` interaction, and sets `HLAparameterError` true. Both
public switch values remain unchanged. The test does not constrain the
synchronous exception type or the provider-generated exception wording.
Candidate and combined fresh verified installed-package builds passed 4/4 CTest
cases for this pair across evoked and immediate callback models. Direct results are in
`.build/cpp-tck-federate-mom-exception-report-delivery-candidate-2-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-mom-exception-reporting-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The standard Service Reporting precondition is covered by the promoted pair
`cpp-tck.federate-mom-exception-report-service-precondition` and its contract
twin. While subscribed to `HLAreportServiceInvocation`, a well-formed request to
enable Service Reporting produces one reliable `HLAreportMOMexception` with
`HLAparameterError` false and leaves the switch disabled. After removing that
subscription, the same request succeeds and the switch can be restored. The
test does not constrain synchronous exception type or wording. Candidate and
combined fresh verified installed-package builds passed 4/4 CTest cases for
this pair across evoked and immediate callback models. Direct results are in
`.build/cpp-tck-federate-mom-exception-report-service-precondition-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-mom-exception-reporting-verified-20260927/verified-results.json`
(with matching JUnit XML files).

Missing-parameter standard MOM reporting is covered by the promoted pair
`cpp-tck.federate-mom-exception-missing-parameter` and its contract twin. The
test sends `HLArequestSynchronizationPointStatus` without its declared
`HLAsyncPointName`, receives one reliable `HLAreportMOMexception` identifying
the fully qualified request class, verifies `HLAparameterError` is true, and
confirms no synchronization-status response was generated. It does not constrain
synchronous exception type or provider-generated wording. Candidate and fresh
verified installed-package builds passed 4/4 CTest cases across evoked and
immediate callback models. Direct results are in
`.build/cpp-tck-federate-mom-exception-missing-parameter-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-mom-exception-missing-parameter-verified-20260927/verified-results.json`.

Federation-level missing-parameter MOM reporting is covered by the promoted pair
`cpp-tck.federation-mom-exception-missing-fom-module-indicator` and its contract
twin. The test sends `HLArequestFOMmoduleData` without its declared
`HLAFOMmoduleIndicator`, receives one reliable `HLAreportMOMexception` with the
fully qualified request class and `HLAparameterError` true, and confirms no
FOM-module content report was generated. It does not constrain synchronous
exception type or provider-generated wording. Candidate and fresh verified
installed-package builds passed 4/4 CTest cases across evoked and immediate
callback models. Direct results are in
`.build/cpp-tck-federation-mom-exception-missing-fom-module-indicator-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federation-mom-exception-missing-fom-module-indicator-verified-20260927/verified-results.json`.

Federate-scoped FOM-module retrieval is covered by the promoted pair
`cpp-tck.federate-mom-fom-module-content-report` and its contract twin. A subject
federate joins the adapter-supplied additional FOM module; a distinct requester
addresses it through the inherited standard `HLAfederate` parameter and requests
module index 0. The test verifies reliable `HLAreportFOMmoduleData` delivery,
the module indicator, standard FOM XML content, and the target handle if the
report includes that inherited parameter. Candidate and fresh verified
installed-package builds each passed 4/4 CTest cases across evoked and immediate
callback models. Direct results are in
`.build/cpp-tck-federate-mom-fom-module-content-report-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-mom-fom-module-content-report-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The standard object-instance-information query is covered by the promoted pair
`cpp-tck.federate-mom-object-instance-information` and its contract twin. It
checks the requesting federate's owned object, another object it knows but does
not own, and the NULL report after local deletion. The test decodes the MIM's
nested `HLAattributeHandleList` with public RTIambassador handle decoders and
verifies the MIM-required class omission for the NULL response. Candidate and
fresh verified installed-package builds both passed 4/4 CTest cases across
evoked and immediate callback models. Direct results are in
`.build/cpp-tck-federate-mom-object-instance-information-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-mom-object-instance-information-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The federate-level publication query is covered by the promoted pair
`cpp-tck.federate-mom-publication-query` and its contract twin. A distinct
requester asks about a subject that publishes one adapter-selected object
class/attribute and interaction class. The test decodes and checks both ordinary
publication reports, then verifies the standard zero-count/omitted-class/empty-
list response for directed interactions, which the subject did not publish.
It uses no directed-publication service or DDM regions. Candidate and fresh
verified installed-package builds both passed 4/4 CTest cases across evoked and
immediate callback models. Direct results are in
`.build/cpp-tck-federate-mom-publication-query-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-mom-publication-query-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The Send Service Reports to File switch is covered by the promoted pair
`cpp-tck.federate-send-service-reports-to-file-mom-switch` and its contract
twin. The test sets false and true, restores the observed starting value, reads
back the public switch, and confirms neighboring switches stay unchanged while
Service Reporting remains off. It does not claim report routing or file
creation. Candidate and separate verified installed-package builds both passed
4/4 CTest cases across evoked and immediate callback models. Direct results are
in
`.build/cpp-tck-federate-send-service-reports-to-file-mom-switch-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-send-service-reports-to-file-mom-switch-verified-20260927/verified-results.json`
(with matching JUnit XML files).

The Automatic Resign Action MOM switch is covered by the promoted pair
`cpp-tck.federate-automatic-resign-action-mom-switch` and its contract twin.
It sends standard `HLAresignAction` values `NO_ACTION` and `DELETE_OBJECTS`
encoded as `HLAinteger32BE`, verifies each through the public directive getter,
and restores the observed initial value before resignation. It does not test
resignation side effects. Candidate and separate verified installed-package
builds both passed 4/4 CTest cases across evoked and immediate callback models.
Direct results are in
`.build/cpp-tck-federate-automatic-resign-action-mom-switch-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-federate-automatic-resign-action-mom-switch-verified-20260927/verified-results.json`
(with matching JUnit XML files). At that prior checkpoint the catalog contained
874 IDs (872 promoted, 2 candidates).

The promoted `cpp-tck.joined-federate-mom-object-instances-reflected-multi-class-report`
scenario and contract twin extend the class-based report to two adapter-supplied
object classes. A distinct requester verifies the decoded standard MIM report
maps two reflected instances to each class, while repeated updates to one
instance per class do not inflate the counts. The application attributes are
standard `HLAbyte`; the report uses standard MIM data elements and the public
object-class handle decoder. No DDM region services are invoked. Separate fresh
candidate and verification builds each passed all 12 focused CTest cases across
evoked and immediate callback models; direct results are in
`.build/cpp-tck-joined-federate-mom-reflected-multi-class-candidate2-20260927/candidate-results.json`
and
`.build/cpp-tck-joined-federate-mom-reflected-multi-class-verified-20260927/verified-results.json`
(with matching JUnit XML files). Candidate-only direct evidence passed the TCK
evidence validator before promotion.

The promoted `cpp-tck.joined-federate-mom-object-instances-updated-multi-class-report`
scenario and contract twin apply the same two-class mapping to the standard
`HLAreportObjectInstancesUpdated` report. A subject registers two instances per
class, performs six ordinary receive-order updates with repeated updates to one
instance per class, and a subscribed observer confirms all callbacks. A distinct
requester verifies the decoded standard MIM count is exactly two for each
registered class. No DDM region services are called. Separate fresh candidate
and verification builds each passed all 16 focused CTest cases across evoked
and immediate callback models; direct results are in
`.build/cpp-tck-joined-federate-mom-updated-multi-class-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-joined-federate-mom-updated-multi-class-verified-20260927/verified-results.json`
(with matching JUnit XML files). Candidate-only direct evidence passed the TCK
evidence validator before promotion.

Before zero-update response promotion, the catalog contained 878 IDs (876
promoted, 2 candidates).

The promoted `cpp-tck.joined-federate-mom-object-instances-updated-null-report`
scenario and contract twin join a subject with no successful updates and a
distinct requester. The requester receives exactly one reliable standard
`HLAreportObjectInstancesUpdated` and verifies its `HLAobjectInstanceCounts`
parameter is undefined or decodes as an empty standard variable array, matching
the MIM NULL-response shape. Separate fresh candidate and verification builds
each passed all 20 focused CTest cases across evoked and immediate callback
models; direct results are in
`.build/cpp-tck-joined-federate-mom-updated-null-candidate-20260927/candidate-results.json`
and
`.build/cpp-tck-joined-federate-mom-updated-null-verified-20260927/verified-results.json`
(with matching JUnit XML files). Candidate-only evidence passed the TCK
evidence validator before promotion.

The no-reflections response is now promoted as the symmetric NULL-report case.
The subject performs no standard Reflect Attribute Values invocations; the
distinct requester receives exactly one reliable
`HLAreportObjectInstancesReflected` and checks that `HLAobjectInstanceCounts`
is undefined or decodes as an empty standard variable array. Two fresh
installed-package builds each passed all 24 focused CTest cases across evoked
and immediate callback models. Candidate-only evidence is in
`.build/cpp-tck-joined-federate-mom-reflected-null-candidate-20260927/candidate-only-results.json`;
the fresh verification evidence is in
`.build/cpp-tck-joined-federate-mom-reflected-null-verified-20260927/verified-results.json`.
A fresh installed-package consumer build passed 32/32 CTest cases across the
promoted MOM report regression and both timestamped report pairs. Direct
evoked/immediate evidence passed separately for all 14 promoted scenarios and
both candidates; both evidence validators passed.

The promoted `cpp-tck.joined-federate-mom-interactions-received-mim-attribute`
pair checks the initial zero, a direct value of three after three delivered
application interactions, a periodic value of five after two more callbacks,
and the unchanged direct value after periodic reporting is disabled. It maps
the standard MIM attribute at lines 350-359 directly to clause 11.4.1 and uses
only the adapter-backed official C++ API surface. Two independent installed-
package consumer builds each passed all four focused CTest cases, and direct
runs passed for both scenario IDs under both callback models.

The promoted `cpp-tck.joined-federate-mom-interactions-sent-mim-attribute`
pair captures a direct baseline after enabling periodic reporting, then checks
that three accepted ordinary application sends add exactly three and that the
periodic value matches. It maps to the standard MIM entry at lines 362-373 and
clause 11.4.1; DDM accounting is explicitly outside this ordinary-send slice.
Two independent installed-package consumer builds each passed all four
focused CTest cases, with direct runs of both IDs under both callback models.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-received-mim-attribute`
pair checks zero initially, confirms one ordinary callback raises only
`HLAinteractionsReceived`, then checks three and five delivered directed
callbacks in `HLAdirectedInteractionsReceived` alongside total counts four and
six. It maps the two standard MIM attributes to clause 11.4.1 (lines 376-386
and 350-359 respectively), and keeps the P0 directed target free of DDM regions.
Two independent installed-package consumer builds each passed all four focused
CTest cases and direct runs of both IDs under both callback models:
`.build/cpp-tck-directed-received-verify-a-20260927/` and
`.build/cpp-tck-directed-received-verify-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-sent-mim-attribute`
pair checks zero initially, confirms one ordinary send increments only
`HLAinteractionsSent`, then checks three and five directed sends in
`HLAdirectedInteractionsSent` alongside total counts four and six. The two MIM
entries map to clause 11.4.1 (lines 389-400 and 362-373); the no-region test
does not claim DDM-related `HLAinteractionsSent` accounting. Two independent
installed-package consumer builds each passed all four focused CTest cases and
direct runs of both IDs under both callback models:
`.build/cpp-tck-directed-sent-verify-a-20260927/` and
`.build/cpp-tck-directed-sent-verify-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-updates-sent-mim-attribute` pair
checks zero before application updates, three after three successful invocations
(including two updates to the same object), and five after two further updates
under `HLAsetTiming` periodic reporting. Each call is correlated with exactly one
ordinary reflection callback. The standard MIM entry maps to clause 11.4.1
(lines 335-347). Two independent installed-package builds each passed all four
focused CTest cases and direct runs of both IDs under both callback models:
`.build/cpp-tck-updates-sent-verify-a-20260927/` and
`.build/cpp-tck-updates-sent-verify-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-object-instances-updated-mim-attribute`
pair checks zero before updates, one after two updates to the first object, two
after updating a second object, and three after updating a third object twice.
Its periodic value is observed before any post-update direct query.

The promoted `cpp-tck.joined-federate-mom-object-instances-reflected-mim-attribute`
pair checks one reflection for the first application object, no increment for a
second reflection to that same object, then one increment for each new object.
A separate monitor federate observes the subject's MOM value so MOM
self-reflections cannot contaminate the application reflection count. Both
attributes map to IEEE 1516.1-2025 clause 11.4.1; the MIM entries are at lines
416-428 for `HLAobjectInstancesUpdated` and 430-442 for
`HLAobjectInstancesReflected`. Two independent installed-package verification
builds each passed 20/20 focused CTest cases across the ten scenario IDs and
both callback models, with direct evidence also passing for each ID in both
callback models:
`.build/cpp-tck-object-instances-reflected-candidate-a-20260927/` and
`.build/cpp-tck-object-instances-reflected-candidate-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-object-instances-deleted-mim-attribute`
pair checks zero before deletion, values one and two after the first two
successful receive-order calls, and an actual periodic value of three after the
third call before any post-traffic direct request. Every delete is paired with
exactly one observer-side removal callback carrying the submitted tag and
publisher handle. The MIM mapping is clause 11.4.1, lines 444-456;
`deleteObjectInstance` maps to clause 6.17, lines 832-847. Two independent
installed-package builds each passed all four focused CTest cases under evoked
and immediate callbacks, and direct evidence passed for both IDs and both
callback models:
`.build/cpp-tck-object-instances-deleted-candidate-a-20260927/` and
`.build/cpp-tck-object-instances-deleted-candidate-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-object-instances-discovered-mim-attribute`
pair checks the post-join baseline, two distinct object discoveries, and a
third discovery when an observer locally deletes and then becomes eligible to
discover the same object again. Direct values and an actual `HLAsetTiming`
periodic reflection agree at each checked total, including after reporting is
disabled. The scalar maps to IEEE 1516.1-2025 clause 11.4.1, lines 486-500;
`localDeleteObjectInstance` maps to clause 6.18. Both fresh installed-package
candidate builds passed all four focused CTest cases and direct evidence for
both scenarios under both callback models:
`.build/cpp-tck-object-instances-discovered-candidate-a-20260927/` and
`.build/cpp-tck-object-instances-discovered-candidate-b-20260927/`.
After promotion, the seven-pair verified regression passed 28/28 CTest cases
and direct evidence for all 14 IDs under both callback models in each build:
`.build/cpp-tck-object-instances-discovered-candidate-a-20260927/` and
`.build/cpp-tck-object-instances-discovered-candidate-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-object-instances-removed-mim-attribute`
pair checks a baseline-relative `HLAobjectInstancesRemoved` count after each of
three successful receive-order deletions, requiring exactly one matching
receiver removal callback with the sent tag and publisher handle for each
object. An actual periodic reflection after the third callback reports the
same total, and a direct request after disabling the period preserves it. The
MIM maps to IEEE 1516.1-2025 clause 11.4.1, lines 458-469; deletion maps to
clause 6.17, lines 832-847, and the callback to clause 6.9.3. Timestamped
removal/retraction and service-reporting variants remain separate. Two fresh
installed-package candidate builds each passed 4/4 focused CTest cases and
direct evidence for both scenarios under both callback models:
`.build/cpp-tck-object-instances-removed-candidate-a-20260927/` and
`.build/cpp-tck-object-instances-removed-candidate-b-20260927/`.
After promotion, the combined eight-pair verified regression passed 32/32
CTest cases and direct evidence for all 16 IDs under both callback models in
each build:
`.build/cpp-tck-object-instances-removed-candidate-a-20260927/` and
`.build/cpp-tck-object-instances-removed-candidate-b-20260927/`.

The promoted `cpp-tck.joined-federate-mom-reflections-received-mim-attribute`
pair checks a post-join baseline, then requires `HLAreflectionsReceived` to
advance once after each of three receive-order Reflect Attribute Values
callbacks. Two updates target the same object to distinguish callback
invocations from distinct object instances. Direct values and the final actual
`HLAsetTiming` periodic reflection agree, and the direct count is unchanged
after periodic reporting is disabled. The standard MIM (IEEE 1516.2-2025)
maps to clause 11.4.1, lines 320-331; the IEEE 1516.1-2025 update, direct
request, and reflection callback map to clauses 6.10, 6.21, and 6.9.3.
Timestamped updates and DDM remain out of scope.
Two fresh installed-package candidate builds each passed 4/4 focused CTest
cases and direct evidence for both scenarios under both callback models:
`.build/cpp-tck-reflections-received-candidate-a-20260927/` and
`.build/cpp-tck-reflections-received-candidate-b-20260927/`.
After promotion, the combined nine-pair verified regression passed 36/36
CTest cases and direct evidence for all 18 IDs under both callback models in
each independent build:
`.build/cpp-tck-reflections-received-candidate-a-20260927/` and
`.build/cpp-tck-reflections-received-candidate-b-20260927/`.

The catalog now contains 920 IDs (918 promoted, 2 candidates).

Keep `cpp-tck.federate-lost-mom-report` and its contract twin as candidates. A
fresh package install at
`.build/package-smoke-install-current-20260927/` confirmed both federates select
the adapter endpoint; the ownership anchor passed in both callback models. The
candidate reaches transport-loss cleanup and observes `ConnectionLost`, but
times out waiting for the subscribed standard `HLAreportFederateLost` report.
The current process-service fixture does not project that RTI-originated MOM
interaction, so do not synthesize it in the fixture or promote the case. Resume
this slice when server-side report delivery is available.

The promoted `cpp-tck.federate-automatic-resign-action-mim-attribute` pair
verifies the standard MIM's conditional `HLAautomaticResignAction` value. The
monitor subscribes to the standard federate MOM object, checks the initial
direct `HLAinteger32BE` value against `getAutomaticResignDirective`, then checks
one conditional reflection after `setAutomaticResignDirective` changes the
action and another after it restores the initial value. It also verifies
reliable receive-order reflection metadata and does not use `HLAsetSwitches`.
Two independent fresh builds against the installed package each passed all
4/4 focused CTest cases and both scenario IDs in evoked and immediate callback
modes. Build evidence is in
`.build/cpp-tck-federate-automatic-resign-action-mim-attribute-candidate-a-20260927/`
and
`.build/cpp-tck-federate-automatic-resign-action-mim-attribute-candidate-b-20260927/`.

The promoted `cpp-tck.federate-exception-reporting-mim-attribute` pair checks
the standard MIM's conditional `HLAexceptionReporting` value. The direct
`HLAinteger32BE` value agrees with the public getter before a standard setter
change and after restoring the initial state; exactly one reliable MOM
reflection accompanies each change. It does not generate service exceptions.
The MIM mapping is IEEE 1516.2-2025 clause 11.4.1, lines 619-627 and 3143-3155;
the public setter/getter and request/callback paths map to IEEE 1516.1-2025
clauses 10.48, 10.49, 6.21, 5.8, and 6.9.3. Two independent fresh candidate
builds each passed 4/4 focused CTest cases and both IDs in both callback models.
The post-promotion regression of both MOM-attribute pairs and their switch
counterparts passed 16/16 CTest cases and all eight IDs in evoked and immediate
modes. Evidence is in
`.build/cpp-tck-federate-exception-reporting-mim-attribute-candidate-a-20260928/`,
`.build/cpp-tck-federate-exception-reporting-mim-attribute-candidate-b-20260928/`,
and `.build/cpp-tck-conditional-federate-mom-attributes-verified-20260928/`.

The promoted `cpp-tck.federate-service-reporting-mim-attribute` pair covers the
conditional `HLAserviceReporting` MIM scalar. It compares direct `HLAinteger32BE`
requests with the public switch getter, checks exactly one reliable conditional
reflection after changing and restoring the switch, and deliberately does not
subscribe to service-invocation reports. Two independent fresh installed-package
builds passed 4/4 focused CTest cases, and the combined post-promotion regression
passed 24/24 cases for the three scalar MOM-attribute pairs and their switch
counterparts. Evidence is in
`.build/cpp-tck-federate-service-reporting-mim-attribute-candidate-a-20260928/`,
`.build/cpp-tck-federate-service-reporting-mim-attribute-candidate-b-20260928/`,
and `.build/cpp-tck-conditional-federate-mom-attributes-service-reporting-verified-20260928/`.

The promoted `cpp-tck.federate-object-class-relevance-advisory-mim-attribute`
pair applies the same conditional MOM value checks to
`HLAobjectClassRelevanceAdvisory`, mapped to MIM clause 11.4.1 lines 535-544
and API clauses 10.34/10.35, 6.21, 5.8, and 6.9.3. Two independent fresh
installed-package builds passed 4/4 focused CTest cases; its post-promotion
regression with the advisory-switch pair also passed.

The promoted `cpp-tck.federate-attribute-relevance-advisory-mim-attribute` pair
checks `HLAattributeRelevanceAdvisory` through direct standard-MIM requests, the
public getter/setter, and conditional reflections. Its exact mappings are MIM
clause 11.4.1 lines 547-556 and API clauses 10.36, 10.37, 6.21, 5.8, and 6.9.3.
Two independent fresh installed-package builds each passed 4/4 focused CTest
cases across both callback models; evidence is in
`.build/cpp-tck-federate-attribute-relevance-advisory-mim-attribute-candidate-a-clean-20260928/`
and
`.build/cpp-tck-federate-attribute-relevance-advisory-mim-attribute-candidate-b-clean-20260928/`.
The post-promotion regression of the advisory-switch pair and both conditional
MOM-attribute pairs passed 12/12 CTest cases across evoked and immediate models;
evidence is in `.build/cpp-tck-relevance-advisory-mom-verified-clean-20260928/`.
At that checkpoint the catalog contained 926 scenario IDs (924 promoted, 2
candidates). The promoted `cpp-tck.federate-attribute-scope-advisory-mim-attribute`
pair checks the conditional `HLAattributeScopeAdvisory` value, mapped to MIM
clause 11.4.1 lines 559-568 and API clauses 10.38, 10.39, 6.21, 5.8, and 6.9.3.
Two independent fresh installed-package builds each passed 4/4 focused CTest
cases across both callback models, and the post-promotion advisory regression
passed 16/16 cases across the core switch pair and all three conditional MOM
attribute pairs. Evidence is in
`.build/cpp-tck-federate-attribute-scope-advisory-mim-attribute-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-attribute-scope-advisory-mim-attribute-candidate-b-clean-20260928/`,
and `.build/cpp-tck-relevance-advisory-mom-regression-verified-clean-20260928/`.
The promoted `cpp-tck.federate-interaction-relevance-advisory-mim-attribute`
pair checks conditional `HLAinteractionRelevanceAdvisory`, mapped to MIM clause
11.4.1 lines 571-580 and API clauses 10.40, 10.41, 6.21, 5.8, and 6.9.3. Two
independent fresh installed-package builds passed 4/4 focused cases each across
both callback models. The post-promotion regression covering the core advisory
switch pair and all four conditional MOM-attribute pairs passed 20/20 cases;
evidence is in
`.build/cpp-tck-federate-interaction-relevance-advisory-mim-attribute-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-interaction-relevance-advisory-mim-attribute-candidate-b-clean-20260928/`,
and `.build/cpp-tck-advisory-mom-regression-verified-clean-20260928/`.
The promoted `cpp-tck.federate-asynchronous-delivery-mim-attribute` pair reads
the initial `HLAboolean` from the standard MIM rather than assuming a default,
uses Enable/Disable Asynchronous Delivery to change and restore that observed
state, and verifies the conditional reflection and direct-request value. It
maps MIM clause 11.4.1 lines 185-200 and the HLAboolean representation at
3046-3056 to API clauses 8.15, 8.16, 6.21, 5.8, and 6.9.3. Two independent
fresh installed-package builds passed 4/4 focused cases each across both
callback models; the post-promotion regression with the existing asynchronous-
delivery/callback-servicing pair passed 8/8 cases. Evidence is in
`.build/cpp-tck-federate-asynchronous-delivery-mim-attribute-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-asynchronous-delivery-mim-attribute-candidate-b-clean-20260928/`,
and `.build/cpp-tck-asynchronous-delivery-mom-regression-verified-clean-20260928/`.
The promoted `cpp-tck.federate-time-constrained-mim-attribute` pair reads the
initial `HLAtimeConstrained` value from the standard MIM, changes and restores
that observed state through Enable/Disable Time Constrained, and verifies one
conditional reflection and a direct request after each transition. It also
waits for exactly one `timeConstrainedEnabled` callback per enable. Two
independent fresh installed-package builds passed 4/4 focused cases each; the
post-promotion regression across the time-role callback, asynchronous-delivery,
and time-constrained MOM pairs passed 16/16. Evidence is in
`.build/cpp-tck-federate-time-constrained-mim-attribute-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-time-constrained-mim-attribute-candidate-b-clean-20260928/`,
and `.build/cpp-tck-time-constrained-mom-regression-verified-clean-20260928/`.
The promoted `cpp-tck.federate-time-regulating-mim-attribute` pair reads the
initial `HLAtimeRegulating` value from the standard MIM, changes and restores
that observed state through Enable/Disable Time Regulation using the
adapter-selected standard factory's epsilon lookahead, and verifies one
conditional reflection and a direct request after each transition. It also
waits for exactly one `timeRegulationEnabled` callback per enable. Two
independent fresh installed-package builds passed 4/4 focused cases each; the
post-promotion regression across time-role callbacks, asynchronous delivery,
and both conditional time-role MOM pairs passed 20/20. Evidence is in
`.build/cpp-tck-federate-time-regulating-mim-attribute-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-time-regulating-mim-attribute-candidate-b-clean-20260928/`,
and `.build/cpp-tck-time-regulating-mom-regression-verified-clean-20260928/`.
The promoted `cpp-tck.federate-lookahead-mim-periodic` pair checks the standard
MIM `HLAlookahead` report after `HLAsetTiming`, then again after a standard
Modify Lookahead increase verified by Query Lookahead. It decodes the
`HLAtimeInterval` bytes with the adapter-selected standard logical-time
factory, so no implementation-specific time encoding or array framing is
assumed. Two independent installed-package consumer builds passed 4/4 focused
cases each; the regression across both time-role MOM pairs, callback-controls
time role, asynchronous delivery, and the new lookahead pair passed 24/24.
Evidence is in
`.build/cpp-tck-federate-lookahead-mim-periodic-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-lookahead-mim-periodic-candidate-b-clean-20260928/`,
and `.build/cpp-tck-federate-lookahead-mim-periodic-regression-clean-20260928/`.
The portable Java parity catalog has no periodic `HLAlookahead` scenario, so
the pair is recorded as a C++ extension. At that promotion, the catalog
contained 938 scenario IDs (936 promoted, 2 candidates). The connection-loss
MOM report pair remains unpromoted until RTI-originated report delivery is
observable.

The promoted `cpp-tck.federate-logical-time-mim-periodic` pair checks the
standard-MIM `HLAlogicalTime` value before and after two successful grants. A
standard time-regulating monitor advances alongside the time-constrained
subject; each reported logical time is decoded with the adapter-selected
`LogicalTimeFactory` and compared with both the grant and Query Logical Time.
The portable Java parity catalog has no equivalent periodic logical-time
scenario, so this pair is recorded as a C++ extension. Two independent clean
installed-package consumer builds passed 4/4 focused CTest cases each, and the
related time-management regression passed 24/24. Evidence is in
`.build/cpp-tck-federate-logical-time-mim-periodic-candidate-a-clean-20260928/`,
`.build/cpp-tck-federate-logical-time-mim-periodic-candidate-b-clean-20260928/`,
and
`.build/cpp-tck-federate-logical-time-mim-periodic-regression-clean-20260928/`.
The promoted `cpp-tck.federate-time-manager-state-mim-attribute` pair checks
the conditional standard MIM `HLAtimeManagerState` values (`TimeAdvancing=1`,
`TimeGranted=0`) for Time Advance Request, Time Advance Request Available, Next
Message Request, Next Message Request Available, and Flush Queue Request. It
uses the adapter-selected standard logical-time factory and validates the
request's matching standard grant callback and reflection metadata. The flush
path specifically checks `flushQueueGrant`, not `timeAdvanceGrant`. A first
clean installed-package consumer build passed 4/4 focused CTest cases and the
direct run in both callback models; an independent clean regression build
passed 20/20 across the pair and adjacent time-role/periodic MOM scenarios, with
direct evidence for all ten selected IDs. Evidence is in
`.build/cpp-tck-federate-time-manager-state-a-clean-20260928/` and
`.build/cpp-tck-federate-time-manager-state-b-regression-20260928/`. There is no
Java counterpart for this conditional MIM attribute, so the pair is recorded
as a C++ extension.
The Java parity audit still represents all 21 Java scenario IDs. The catalog
now contains 942 IDs (940 promoted, 2 candidates). The standard periodic
`HLAGALT`/`HLALITS` and Query GALT/LITS route already has a promoted portable
pair, including its undefined-value boundary, so no duplicate was added. Next
bounded handoff: the adapter-managed process-loss fixture is now configured,
and a clean installed-package consumer build reaches the focused report test.
The base `cpp-tck.federate-lost-mom-report` scenario and contract twin were
tested separately; each times out waiting for the subscribed standard
`HLAreportFederateLost` interaction in both evoked and immediate callback
models. The fixture's subsequent observer-Resign error is cleanup fallout from
that TCK failure. The process fixture currently does not project the
RTI-originated MOM report, so keep this pair as candidates and do not synthesize
the callback in the fixture. The CMake fixture-only selection guard now
recognizes both candidate IDs. The native-gap survey found no directly portable
candidate among 297 Catch2 stems, while the API-surface audit confirms all 164
official `RTIambassador` methods and 56 callbacks are referenced and Java
parity remains 21/21. Next, identify an unasserted normative requirement before
translating another native test; do not treat internal-only native cases as
portable HLA evidence.

The promoted `cpp-tck.federation-mom-static-identity` pair verifies the
standard `HLAfederation` MOM object and all four required static identity
strings: `HLAfederationName`, `HLARTIversion`, `HLAMIMdesignator`, and
`HLAtimeImplementationName`. It checks the created federation name, a non-empty
RTI version without prescribing its format, `HLAstandardMIM`, the exact
adapter-supplied time-implementation name, reliable callback metadata, and
standard Unicode encoding without provider-specific assertions. Two
independent installed-package build trees each passed all 4 focused CTest
cases across the scenario and contract IDs under evoked and immediate callback
models:
`.build/cpp-tck-federate-time-manager-state-a-clean-20260928/` and
`.build/cpp-tck-federate-time-manager-state-b-regression-20260928/`. The pair
has no Java counterpart and is recorded as a C++ extension. The new promoted
`cpp-tck.federation-mom-static-switches` pair covers the four required static
federation switch attributes. It decodes each standard `HLAswitch` through
`HLAinteger32BE` and accepts only `Disabled=0` or `Enabled=1`, without asserting
provider-specific defaults. The promoted
`cpp-tck.federation-mom-federates-in-federation` pair covers the required
conditional federation roster: the requested initial value includes the
creator and observer, a join reflection includes the third member, and a resign
reflection returns to the two-member roster. Roster order is deliberately
ignored; each standard federate-handle reference is decoded through the official
`RTIambassador::decodeFederateHandle`. Two independent installed-package builds
each passed all 12 focused CTest cases across the three federation MOM scenario
pairs and both callback models:
`.build/cpp-tck-federate-time-manager-state-a-clean-20260928/` and
`.build/cpp-tck-federate-time-manager-state-b-regression-20260928/`. The roster
array is traversed using the standard `HLAinteger32BE` and `HLAvariableArray`
decode APIs because the nested-array convenience decoder stopped before the end
of this valid MIM value. The promoted
`cpp-tck.federation-mom-fom-module-designator-list` pair requests the federation's
initial FOM-module designators, observes their conditional refresh after a join
adds an adapter-supplied module, then confirms a direct request agrees. It uses
the standard `HLAvariableArray` and `HLAunicodeString` encoders and compares the
set without assuming ordering. The focused four-pair federation-MOM attribute lane
passed 16/16 focused CTest cases in each independent installed-package build
tree under evoked and immediate callback models.

The promoted `cpp-tck.federate-mom-subscription-query` pair uses the
adapter-selected application FOM and update-rate FOM with the standard MIM to
report one active object-attribute subscription and one active interaction
subscription for a named subject federate. It also verifies the zero-count
directed-interaction NULL response, including its omitted object-class
parameter and empty interaction-class list. The scenario uses no DDM regions
or provider-specific interfaces. Two independent installed-package consumer
builds each passed all 4 focused CTest cases across both callback models; the
regression with the existing publication-query pair passed 8/8. Evidence is in
`.build/cpp-tck-federate-mom-subscription-query-candidate-a-clean-20260928/`
and `.build/cpp-tck-federate-mom-subscription-query-candidate-b-clean-20260928/`.

The promoted `cpp-tck.joined-federate-mom-object-lifecycle` pair also checks the
required static `HLAfederateHost` Unicode value for non-emptiness without
asserting platform-specific hostname spelling. Its focused 4/4 callback-model
matrix passed in both independent installed-package trees. The portable Java
parity audit still represents all 21 Java scenario IDs.

The passive-subscription pair extends the standard `HLArequestSubscriptions`
coverage without adding a provider interface: the queried object-class report
must identify the adapter-selected attribute as passive (`HLAactive=false`)
while preserving its selected update-rate name. It also confirms the active
ordinary interaction report and the required empty directed-interaction NULL
response. Two independent installed-package consumer trees each passed 8/8
focused CTest cases across both callback models, and direct evidence passed all
four active/passive scenario IDs in both callback models. Evidence is in
`.build/cpp-tck-federate-mom-passive-subscription-query-candidate-a-clean-20260928/`
and
`.build/cpp-tck-federate-mom-passive-subscription-query-candidate-b-clean-20260928/`.
The passive-interaction pair validates the adapter-selected interaction class
as a passive subscription while retaining an active object-attribute
subscription. Its standard `HLAinteractionClassList` record must contain the
selected interaction handle paired with `HLAactive=false`. Two independent
installed-package consumer trees each passed 12/12 focused CTest cases across
both callback models, and direct evidence passed all six active/passive-object/
passive-interaction scenario IDs in both callback models. Evidence is in
`.build/cpp-tck-federate-mom-passive-interaction-subscription-query-candidate-a-clean-20260928/`
and
`.build/cpp-tck-federate-mom-passive-interaction-subscription-query-candidate-b-clean-20260928/`.
The pair is promoted. The catalog now contains 956 IDs (954 promoted, 2
candidates); Java parity remains 21/21. Next bounded handoff: translate the
standard MIM NULL responses for empty object-attribute and interaction
subscriptions; the existing native `requestSubscriptions` test is green for
those report shapes, while the portable TCK currently checks only the empty
directed-interaction response.
