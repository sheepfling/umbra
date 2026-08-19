# Python provider conformance testing

## Purpose

The Python binding has a small public surface today, so it should test that
surface as a provider contract rather than repeat provider-specific examples.
`hla.rti1516_2025.testing.ConnectionFoundationConformanceMixin` and
`ConnectionOverloadConformanceMixin` are reusable `unittest` mixins. A
provider supplies `make_factory()`; together they verify the observable RTI
connection state machine and all four currently supported `connect` forms
through the public Python API.

This is intentionally a binding conformance suite, not an assertion that any
provider implements the full IEEE standard.

## C++ test mapping

The initial contract mirrors the available portions of
`cpp/tests/ieee1516_2025_connection_catch2.cpp`.

| C++ assertion family | Python conformance assertion | Status |
| --- | --- | --- |
| Base `connect` with immediate and evoked models | Both `CallbackModel` values connect and yield a well-formed `ConfigurationResult` | Implemented |
| Duplicate `connect` | Raises `AlreadyConnected` at the Python provider edge | Implemented |
| Unsupported callback enum | A non-`CallbackModel` value raises `UnsupportedCallbackModel`, then the ambassador remains connectable | Implemented |
| `disconnect` before connection | Raises `NotConnected` | Implemented |
| Callback enable/disable controls | Native and real-JVM providers suppress queued callback delivery while disabled and deliver retained callbacks after re-enabling; the boolean evoke result remains provider-defined | Implemented |
| Disconnect then reconnect | A fresh connection succeeds after disconnect | Implemented |
| C++ configuration/credential overloads | Base, `RtiConfiguration`, `HLAnoCredentials`, and both together each connect and yield a well-formed `ConfigurationResult` | Implemented |
| C++ `RtiConfiguration` value semantics | Java-shaped Python builder retains name, address, and additional-settings values | Implemented |
| `listFederationExecutions` / `reportFederationExecutions` | Both callback models deliver a typed `FederationExecutionInformationSet`; unconnected use raises `NotConnected` | Implemented |
| Scalar federation create/destroy with a FOM module | Native provider creates a real federation, lists its typed record, then destroys it; Java adapters perform the same method path against their configured RTI | Implemented |
| Federation FOM/MIM and membership overloads | Native C++ vectors and MIM creation, Java `URL[]`/`URL` creation, additional join FOM modules, and explicit synchronization `FederateHandleSet` registration all cross the shared Python contract | Implemented |
| `listFederationExecutionMembers` / member-or-missing callbacks | Both callback models report a missing federation through `reportFederationExecutionDoesNotExist`; native and JVM integration tests also convert the typed empty-member report for a real federation | Implemented |
| Scalar join/resign | C++ and Java bindings return a portable encoded `FederateHandle`; native integration proves that a joined federate appears in the typed member report, while native and real-JVM fixtures convert the RTI-originated `federateResigned` reason callback | Implemented |
| `queryFederationSaveStatus` / response callback | Native provider queries a two-member real federation before, during, and after a save; the fake and real-JVM Java fixture also converts the ordered `(FederateHandle, SaveStatus)` response | Implemented |
| Scalar federation save success | Two real federates receive save initiation, begin/complete their saves, and each receives `federationSaved`; the real-JVM fixture exercises the same lifecycle and enforces the federation-wide completion barrier | Implemented |
| Timestamped federation save request | Native Python drives a provider `LogicalTime` through the C++ overload; native coverage includes immediate, constrained two-federate grant boundaries, an in-transit timestamped interaction delivered before save admission, and a save requested re-entrantly from a TSO callback (initiation waits for callback return); fake Java and the real JVM fixture receive the decoded Java `LogicalTime`, hold initiation until time advance, and assert the same receive -> initiate-save -> grant order | Implemented; vendor-specific TSO scheduling edge cases remain |
| Scalar federation save failure/abort | A real federate-reported failure and an explicit abort both report matching typed `SaveFailureReason` values to each member; the real-JVM fixture covers both reasons and broadcasts each outcome across two members | Implemented |
| `queryFederationRestoreStatus` / response callback | Native provider queries a two-member real federation before, during, and after restore; the real-JVM fixture also converts typed pre/post handles and `RestoreStatus` records across those transitions | Implemented |
| Scalar federation restore success | A saved two-member federation reports missing-snapshot rejection or acceptance, restore-begin, typed per-federate initiation, and `federationRestored` on completion; native and real-JVM fixtures rewind saved logical time/lookahead, and native also restores a queued timestamped interaction with its original retraction handle and proves post-restore retraction/request-retraction; the real-JVM fixture enforces the multi-member restore completion barrier and rewinds distinct member times | Implemented in native and real-JVM fixture runs |
| Scalar federation restore failure/abort | A real federate-reported failure and an explicit abort both report matching typed `RestoreFailureReason` values to each member; the real-JVM fixture broadcasts both outcomes across the shared restore transaction | Implemented in native and real-JVM fixture runs |
| Basic data-element encoding | Native C++ and real-JVM fixture factories produce the same signed/unsigned 32-bit, four-octet boolean, and UTF-16BE Unicode vectors; malformed native boolean data maps to `DecoderException` | Implemented |
| Support name/handle lookups | A joined native Restaurant FOM federation and the fake/real JVM fixtures round-trip federate, object class, attribute, interaction class, parameter, transportation type, and dimension handles, including known object-class lookup; Java input handles are recreated through their standard `get*HandleFactory().decode` methods | Implemented |
| Handle, set, and map factories | Native decoders and fake/real-JVM Java handle factories round-trip each declared opaque handle domain; set/map factories create mutable Python builders, copy byte values, and feed those builders back through provider services | Implemented; message-retraction decoding remains provider-internal because Java exposes no public factory |
| Support value lookups | Native Restaurant FOM and fake/real JVM fixtures resolve update rates, order types/names, available object/interaction dimensions, and dimension upper bounds through typed Python values | Implemented |
| Support normalization | Native C++ and fake/real JVM fixtures resolve service-group and typed handle normalization coordinates through strict public domains | Implemented |
| Timestamped object/interaction services | Native C++ and fake/real JVM fixtures invoke timestamped update, interaction send, regional interaction send, object deletion, and retraction through typed logical-time and message-retraction boundaries; native and real-JVM fixtures also assert typed timed callback metadata, non-null region designators, and `requestRetraction` conversion; the real-JVM fixture fans timestamped object and interaction traffic to unconstrained and constrained subscribers, preserves retraction identity for delivered recipients, and exercises mixed regional designator convey for interactions and associated object updates | Implemented; native regional timestamp callback is now exercised end-to-end |
| Basic declaration management | A native two-federate scenario publishes/subscribes object attributes and interaction classes, then receives typed start/stop and interaction on/off advisories; Java creates its `AttributeHandleSet` via the selected RTI's standard factory and exercises the same calls/callback conversions through the JVM fixture | Implemented |
| Receive-order object management | A native two-federate Restaurant FOM scenario reserves single and multiple names, registers a named object, proves typed discovery and name/handle lookup from the subscribing federate, reflects an opaque attribute-value map with tag, transport, and producer, then deletes it and verifies the typed removal callback and state transition; native local-delete ownership preconditions map to `FederateOwnsAttributes`, while Java fake/JVM fixtures exercise successful local removal and matching set/map factory, service, and callback conversions; the real JVM fixture also fans one registration/update out to two independent subscribers | Implemented; timestamped update/reflection and regional multi-recipient routing remain in the dedicated DDM rows |
| Receive-order interaction management | A native two-federate Restaurant FOM scenario publishes/subscribes a parameterized interaction, sends an opaque parameter-value map, and verifies typed interaction, transport, producer, tag, and value conversion at the subscribed federate; Java exercises its standard map factory, send service, and callback conversion through the JVM fixture, including two-subscriber fanout | Implemented; timestamped interaction is covered in the dedicated timestamped-service row |
| Logical time management | Native C++ and Java adapter tests obtain the provider time factory, round-trip integer and floating time/interval encodings, verify smallest-positive-subnormal floating epsilon, exercise an epsilon-sized floating time advance/grant, enable/disable regulation and constrained mode, toggle asynchronous delivery, query/modify lookahead, exercise time-advance/next-message/flush request variants, query GALT/LITS with valid and undefined results, request grants, query logical time, and convert regulation/constrained/grant/`flushQueueGrant` plus timed object/interaction callbacks; restore tests also prove saved logical time, actual lookahead, and deferred decreases are reinstated | Provider-neutral integer/floating boundaries implemented; fake adapter and real-JVM fixture tests pass |
| Region substrate | Native C++ and Java adapter tests create a typed region, round-trip its dimension set, set/query range bounds, commit, and delete it through the provider boundary | Implemented |
| Regional interaction management | Native C++ verifies overlapping regional subscriptions and receive/timestamp-order sends, including existing-subscription reprojection as regions are added/removed, the convey-region-designator switch, and exact-boundary Allow Relaxed DDM; Java fake and real-JVM fixtures forward regional subscription/send calls and convert optional sent-region and timed callbacks, including the same bounded relaxed-DDM boundary policy; both providers now re-evaluate an explicit regional subscription at the callback boundary when delayed subscription evaluation is enabled, while the real-JVM fixture also applies range-overlap filtering before fanning a timestamped regional interaction with mixed convey settings | Implemented |
| Directed interaction management | Native C++ verifies object-class directed declaration and receive-order delivery to a registered target object, plus three-federate timestamped fanout/retraction behavior; Java fake and real-JVM fixtures forward directed declaration overloads, receive-order/timestamped sends, typed target/source/transport callbacks, and retraction metadata; the real-JVM fixture now exercises multi-recipient directed fanout with a constrained pending recipient | Implemented |
| Regional object management | Native C++ verifies Java-shaped attribute-set/region-set pair vectors through regional registration, discovery, update, association/unassociation, and regional value request, including mixed-fanout timestamped reflection with the convey-region-designator switch, multi-region selective-unassociation/default-region behavior, existing-object discovery reprojection when a subscription gains an overlapping region, scope-advisory out/in transitions across regional unsubscription and re-subscription, default-region callback realization, and exact-boundary relaxed DDM; native and real-JVM tests now re-evaluate a regional object update at the callback boundary after a committed range change when delayed subscription evaluation is enabled; Java fake callback conversion covers present/absent region metadata and the real-JVM fixture carries per-attribute registration/association/unassociation region sets into region-relevant discovery and timestamped multi-recipient reflection, conveys an empty `RegionHandleSet` for receive-order and timestamped default-region callbacks while preserving `None` for ordinary non-regional callbacks, filters disjoint ranges before delivery, suppresses passive regional subscriptions until activation, and exercises exact-boundary relaxed DDM updates | Implemented; broader regional discovery/reprojection matrices remain follow-on |
| Attribute value update requests | Native C++ and fake/real-JVM fixtures exercise both class- and instance-targeted requests and verify copied request tags plus typed `provideAttributeValueUpdate` callback attributes | Implemented |
| Object-management advisory callbacks | Native C++ regional scope transitions and fake/real-JVM fixtures convert scope entry/exit plus plain and named-rate per-object update-relevance callbacks | Implemented |
| Order and transportation management | Native C++ and fake/real-JVM fixtures invoke object/interaction order changes, attribute/interaction transportation changes and queries, and convert typed confirmation/report callbacks | Implemented |
| Attribute ownership management | Native C++ and real-JVM fixtures query a registered object's attribute ownership, execute unconditional/negotiated divestiture and acquisition transitions, exercise confirmation/cancellation/release-denied/divestiture-if-wanted services, and convert typed owner callbacks; Java fake runtime verifies encoded handle forwarding and service tags; the callback proxy test covers all nine Java ownership callback conversions with copied tags and typed domains; native and real-JVM two-member scenarios prove confirmation ordering, transfer tags, unavailable acquisition, release/reacquisition, and pending cancellation | Implemented; broader provider edge cases remain |
| Advisory/reporting support switches | Native C++ and fake/real-JVM Java fixtures toggle and read relevance, automatic-resign, service/exception-reporting, and provider support switches through the shared Python contract | Implemented; broader regional discovery/reprojection and vendor-specific floating-time arithmetic remain |
| C++ `VariableLengthData` value semantics | Provider-private conversion to the future portable Python `bytes` boundary; do not expose C++ pointer ownership | Deferred |

All 100 concrete exception names in the 2025 Java exception package are
available as Python subclasses of `RTIexception`. Provider boundaries map a
known C++ or Java simple name to its specific Python type; individual services
are still added and tested only when bound.

Federation management, logical time, handles, encoding, and the remaining
callback families receive their own corresponding conformance mixins only once
their public Python value types and provider bindings exist.

## Current provider runs

The native provider combines the mixin with its pybind11 wheel test:

~~~text
NativeConnectionFoundationConformanceTest
    -> UmbraRtiFactory
    -> pybind11
    -> Umbra C++ RTI
~~~

The mock Java vendor adapter combines the same mixin with a real JVM fixture:

~~~text
MockVendorJPypeIntegrationTest
    -> MockJavaRtiFactory
    -> JPype
    -> Java ServiceLoader / MockRtiFactory
~~~

The Java fixture queues both a `connectionLost` event and an entire restore
lifecycle, including failure and abort. Its test therefore proves actual
Java-to-Python callback conversion, not merely method invocation. Both
provider runs include the overload mixin, so they exercise the C++ and Java
connection dispatches rather than only their Python method signatures.

## Adding a provider

An adapter package adds a short test class:

~~~python
import unittest

from hla.rti1516_2025.testing import (
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
)


class VendorConnectionConformance(
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
    unittest.TestCase,
):
    def make_factory(self):
        return VendorRtiFactory(configured_jar)
~~~

The provider's normal integration test configures any native dependencies,
JARs, or broker endpoint before the mixin runs. This keeps the behavioral
contract the same while preserving provider-specific setup.

## Running the current suites

The pure API and Java adapter unit suites require no Java vendor JAR:

~~~powershell
$env:PYTHONPATH = 'packages/umbra-rti-api/src;packages/umbra-rti-jpype/src'
python -m unittest discover -s packages/umbra-rti-api/tests
python -m unittest discover -s packages/umbra-rti-jpype/tests
~~~

The native conformance suite runs against an installed `umbra-rti-native`
wheel plus the API source. The mock vendor's real-JVM suite additionally needs
`JPype1`, a JDK, and its generated fixture JAR; its test builds that fixture in
a temporary directory automatically. These setup requirements are deliberate:
they ensure the test exercises the same packaging and bridge boundaries that
an application uses.
