# umbra-rti-jni

The staged C++ → JNI → Java façade for Umbra's own Java RTI provider. Umbra
has one semantic RTI implementation: C++. This artifact exposes that
implementation to standard-shaped Java callers and, through JPype, to Python.
It remains separate from the direct pybind provider and from arbitrary
third-party Java-vendor JARs.

The tracked Java fixture surface is routed to C++ rather than being a
second, Java-side RTI implementation:

- constructs one real `UmbraRtiAmbassador` in C++;
- dispatches every fixture `RTIambassador` Java name/arity to a registered JNI
  endpoint, including all connection, federation, declaration, object, time,
  DDM, ownership, support, and reporting families;
- maps Java overloads such as vector FOM modules and timestamped operations to
  their matching C++ service without creating a Python or Java shadow model;
- copies a joined C++ `FederateHandle` into an immutable Java encoded-handle
  value and converts C++ federation-execution/member reporting callbacks into
  standard-shaped Java values; and
- translates C++ RTI exception names into Java RTI exception classes.

The bridge now has a reproducible staging path, but it is not a Maven/Gradle
publication and Umbra does not claim redistribution rights for the IEEE API.
`package.ps1` stages the bridge JAR and native library, runs the release
verifier, and records the API coordinate, source, and SHA-256 in a dependency
manifest. `-IncludeJavaApiJar` can copy an API JAR supplied by the release
owner beside the bridge for a directly consumable Python artifact directory;
without that switch, the manifest records the external API dependency without
copying it. It can compile against the repository's small API declaration JAR
for fixture tests or an independently obtained IEEE 1516.1-2025 Java API JAR
for consumer validation. The latter path has passed the Java smoke test and the
C++ → JNI → Java → JPype → Python integration suite. The façade never inherits mock ambassador services: every standard
operation either crosses JNI to C++ or raises `RTIinternalError` as explicitly
unbound. The compact fixture is retained for Java compilation and smoke
coverage; JPype/JNI logical-time conformance uses the independently obtained
IEEE JAR because the fixture's legacy top-level time aliases are not the exact
2025 Java return types. Its primitive and composite encoder factory is
likewise C++-owned: it binds signed and
unsigned 16/32/64-bit values, binary32/64 floating values in both byte orders,
distinct `HLAbyte`/`HLAoctet` values, and big-/little-endian `HLAoctetPair`
values, plus `HLAASCIIchar`, `HLAASCIIstring`, `HLAboolean`, UTF-16BE
`HLAunicodeChar`/`HLAunicodeString`, and `HLAopaqueData`, to C++ data
elements. Those are all twenty-six primitive C++ elements provided by Umbra.
Java signed primitives intentionally carry the unsigned values' raw bits,
preserving the standard Java method signatures and C++ byte layout.
All four standard composite factory forms are bound: `HLAvariableArray`,
`HLAfixedArray`, `HLAfixedRecord`, and `HLAvariantRecord`. C++ owns their
prototypes, copied values, mappings, alignment, encoding, and decoding;
Java/Python retain only typed shells reconstructed from C++ octets. Native
composites clone recursively at the JNI boundary, and the raw standard Java
`HLAextendableVariantRecord` carrier now participates in that same composition
path, so standard shapes can nest without reimplementing an encoder in either
adapter. Fixed arrays and records
also expose an internal native setter used by the Python adapter, while the
public Java surface stays within the standard interfaces.
Prototype and mapped-variant type mismatches are delegated to those standard
Java methods as well; C++ raises the standard `EncoderException`, which JPype
maps to the shared Python encoding exception.
The external 2025 Java `EncoderFactory` handle and logical-time creators are
also available through provider-scoped Python façade methods. Their Java
data-element shells delegate encoding, decoding, and value conversion to the
RTIambassador's C++-validated factories. The methods require the Java-backed
ambassador and are intentionally extensions of the shared Python factory, not
new native-provider contract methods. The standard Java extendable interface
also crosses this façade; because it has no `addVariant`, the adapter registers
a mapping on the first `setVariant` and keeps the carrier outside the
provider-neutral shared Python factory.
Encoding failures retain the standard Java encoding surface: C++ encode errors
map to `EncoderException`, and C++ decode errors map to `DecoderException`
before the JPype adapter exposes them as the corresponding Python exceptions.
Connection credentials follow the same ownership rule: the JNI bridge copies
the standard Java `Credentials` type and byte payload into C++'s generic
`Credentials` value, including credential implementations other than
`HLAnoCredentials`. Authorization and any resulting `Unauthorized` decision
therefore remain C++ policy, rather than a JNI-only special case.
The bridge also publishes `NativeAuthorizerFactory` through the standard
`AuthorizerFactoryFactory` ServiceLoader. Its `Authorizer` and
`AuthorizationResult` methods call the C++ reference authorizer; the optional
`umbra.rti.jni.authorizer.password` JVM property supplies its test configuration
without moving policy into Java. Authorization objects remain outside the
provider-neutral Python contract.
The direct pybind provider remains the primary C++→Python route, while the
ordinary JPype path continues to support vendor JARs independently.

The build script clears its generated Java classes directory before invoking
`javac`, so a reused output directory cannot retain stale façade or API
classes. A clean build against the independently obtained IEEE JAR has been
smoke-tested and then run through the complete external suite; the bridge JAR
contains only `org.umbra.jni` classes plus its standard RTI and authorization
`ServiceLoader` descriptors.
The build now fails fast if an API class is accidentally bundled or either
descriptor is missing or names a provider other than the native façade.

The current native service slice covers connection/callback control, federation
listing and membership reports, FOM/MIM creation and destruction, named and
unnamed joins with additional FOM module sequences, resign, and synchronization-point
registration/achievement, federation save-status queries, and scalar and
timestamped federation-save initiation plus the success,
federate-reported-failure, and abort lifecycle.
Synchronization uses a native `FederateHandle` decoder and set factory, so
explicit Python handle sets round-trip into the C++ RTI rather than being
represented by fixture objects. Save-status callbacks likewise construct
standard Java handle/status records from C++ results, while save failures
preserve `SaveFailureReason`; restore-status queries preserve both C++ handle
values and `RestoreStatus`. The complete scalar restore lifecycle also crosses
the bridge: request acceptance/rejection, federation-begun, per-federate
initiation with a native encoded handle, complete, not-complete, and abort;
typed failure callbacks preserve `RestoreFailureReason`. The opt-in Python
integration test drives all three terminal paths through C++ → JNI → Java →
JPype rather than substituting Java fixture behavior.
Federate support services now also bind C++ `getFederateHandle`,
`getFederateName`, and `normalizeFederateHandle`; their Java signature uses
the standard `long` normalization coordinate, verified by the same Python
integration path. The JNI registrations preserve that 64-bit Java return type
for every standard normalization service instead of truncating the C++
`unsigned long` coordinate to `int`. The typed-handle substrate now extends to object classes and
attributes: each has a distinct Java handle and decoder factory, and the
integration test proves C++ object-class lookup/name/normalization plus an
object-class → attribute lookup/name round trip. Attribute-handle normalization
is deliberately absent because IEEE 1516.1 does not define that operation.
Interaction classes follow the same domain-safe implementation: C++ lookup,
name lookup, `long` normalization, and factory decode return a distinct Java
`InteractionClassHandle`, with all conversions verified from Python.
Its dependent interaction-parameter support services are likewise native:
C++ `getParameterHandle`/`getParameterName` cross a distinct Java
`ParameterHandle` and factory, with a real FOM parameter round-trip in the
Python integration test.
The initial interaction-management slice is now live as well: the Java
`ParameterHandleValueMap` factory creates a standard map carrier, and JNI
copies every typed key/value into C++ for `publishInteractionClass`,
`subscribeInteractionClass`, and receive-order `sendInteraction`. A two-member
Python integration vector proves the C++-queued `receiveInteraction` callback
returns the distinct interaction, parameter, transportation, and federate
handle domains with copied parameter octets and tag.
The same slice binds unpublish/unsubscribe teardown and the standard
transportation support services: `HLAreliable` round-trips through C++
lookup/name and its distinct Java decoder factory, and exactly matches the
transportation handle delivered by the C++ interaction callback.
The matching receive-order object-management substrate is native as well:
`AttributeHandleSet` and `AttributeHandleValueMap` factories copy typed
attribute keys into C++, object instances use a distinct encoded-handle class
and decoder factory, and the façade binds object attribute
publish/unpublish/subscribe/unsubscribe, provider- or reservation-named
registration, instance name/handle lookup, normalization, and receive-order
updates. Single-name reservation/release and both standard reservation
callbacks cross JNI as well. A two-member Python integration vector proves
C++ discovery/reflection and reservation callbacks return typed
instance/class/attribute/transportation/federate values and copied
octets/tags. Receive-order deletion/removal crosses the same route and
preserves the standard `ObjectInstanceNotKnown` exception after removal.
Both standard receive-order attribute-value request overloads and the
`provideAttributeValueUpdate` callback also cross the route with copied typed
attribute sets and tags. Attribute and interaction transportation queries
return their typed standard report callbacks, while explicit transportation
change requests return typed confirmation callbacks. Time- and region-based
object variants now use the same C++-owned carriers: timestamped
updates/deletions, region associations, regional requests, and scope callbacks
cross the JNI route with typed time, retraction, and region metadata. Attribute
scope advisory switching and its in-scope/out-of-scope typed callbacks are
bound as well.
Order-type lookup/name and receive-order object/interaction change services,
update-rate support, default attribute transportation, and local object
deletion also use the real C++ ambassador. The first shared-time slice now
uses the C++ time factory to decode all five standard advance/queue requests
and reconstitutes the selected reference-time Java carrier for
`timeAdvanceGrant`, `flushQueueGrant`, and `timeConstrainedEnabled`;
constrained-mode and asynchronous-delivery state changes also remain in C++.
Provider-created integer and floating logical-time intervals now cross the same
boundary for time regulation and lookahead queries/modification, and GALT/LITS
return each C++ validity bit and encoded time as one atomic bridge result.
The Java carriers remain standard `LogicalTime`/`LogicalTimeInterval` views:
their construction, factory decode, add, subtract, and distance operations
call dedicated JNI endpoints, so the selected C++ time factory remains the
sole owner of representation, validation, and arithmetic. Its
`InvalidLogicalTime`, `InvalidLogicalTimeInterval`,
`IllegalTimeArithmetic`, and decode failures survive the Java and JPype
boundaries. The external vector also calls the standard Java factory
construction/decode methods and interval mutators directly, checks the
smallest-positive-subnormal boundary, and checks integer64 values above 2^53
without loss of precision. It also verifies exact and one-step-overflow
addition at the largest finite float64 value and the symmetric signed
`Long.MAX_VALUE` boundary, preserving `IllegalTimeArithmetic` from C++ through
the raw Java carrier. It also proves raw Java carrier add/subtract round trips,
positive distances, and the exact C++ `IllegalTimeArithmetic` cause inside the
standard Java `UndeclaredThrowableException` wrapper for negative distance. It
preserves the standard Java offset-overload
distinction between short-payload `IllegalArgumentException` and C++-owned
negative/non-finite `CouldNotDecode`; trailing bytes remain legal to that raw
Java overload while the normalized Python factory rejects them.
Timestamped interaction send now
returns an opaque C++ message-retraction handle; timestamped receipt preserves
time, both order types, and the handle, and `retract` returns through the
standard typed retraction callback. The same route now covers timestamped
attribute updates and object deletion, with typed timed reflection/removal
callbacks and matching retraction notifications.
Dimension/region factory values, range bounds, and region lifecycle are also
native. The JNI DDM interaction slice preserves regional subscription
filtering and sent-region callback metadata for both receive-order and
timestamped interaction delivery. The external IEEE-JAR vector also keeps
two equal-range interaction subscriptions active through selective
unsubscription, proving the C++ reprojection state remains live until the
final overlap is removed.
Regional object declarations, registration, update associations, and regional
attribute-value requests consume the standard Java attribute-set/region-set
pair-list carrier and are backed by the C++ RTI; regional reflections retain
their sent-region metadata at the Python callback boundary. The JNI vector
also proves that changing a live object's association changes the C++ delivery
passel: unassociation conveys the empty default-region designator, while
reassociation restores the publisher's explicit region.
It also preserves the C++ region-lifetime guard: deleting a source region
still referenced by that registration maps to the standard
`RegionInUseForUpdateOrSubscription` exception through Java and JPype, and the
independent observer decodes the same failure as the standard `Delete Region`
`HLAreportException` interaction.
The same external object vector sends empty-region pair-list subscriptions and
source associations through the official Java factories and verifies that the
C++ no-op semantics survive matching teardown.
The regional-rate vector additionally invokes the standard named-rate
subscription overload and preserves the C++ scope-advisory callbacks,
including the `High` update-rate designator, through JPype.
The update-rate matrix then drives two independent standard Java subscribers:
the `Low` best-effort stream is gated while an `HLAdefault` subscriber receives
the same immediate passels, and an unsubscribe/resubscribe creates a fresh C++
projection-generation admission key. The queried FDD and per-attribute rates
are retained through the same Java and JPype surface.
The ownership-transfer vector also verifies that C++ clears the former owner's
association: the new owner's first update carries the empty/default region
designator, and a later explicit association carries the new owner's region.
The mixed-declaration vector verifies that a retained ordinary subscription is
suppressed by a disjoint explicit regional declaration, then reprojected through
the derived default region when that declaration is removed, with empty conveyed
region metadata preserved for both recipients.
The passive-subscription vector independently verifies that ordinary and regional
declarations remain non-delivering until an active replacement, then reproject
existing objects and deliver later updates through the standard Java callbacks.
The queued-association vector verifies that a timestamped update accepted under
source region A is not retargeted when the association is replaced with region B
before the callback boundary; only the later update carries B and its exact
standard retraction metadata.
The regional interaction re-enable vector mirrors the same callback-boundary
rule through the standard Java API: one queued explicit-source timestamped
interaction survives `disableTimeConstrained` followed by
`enableTimeConstrained` exactly once, retaining its source region, order fields,
and consumed retraction handle. Its companion default-region vector performs
the same transition with an empty conveyed `RegionHandleSet`, proving that the
Java callback keeps the standard default-region realization as well.
The regional object-update companion carries the same proof through the
attribute-value passel: one queued explicit-source reflection survives the
constrained-role transition with its source region, `TIMESTAMP` metadata, and
retraction handle intact.
The default-region object companion repeats the transition with an empty
conveyed `RegionHandleSet`, keeping the standard default realization intact.
The suppressed-timestamp vector verifies that a queued regional recipient
becoming disjoint before delivery consumes the recipient ledger, so no reflection
or `requestRetraction` callback is emitted after the publisher retracts it.
The constrained-grant vector carries the same disjoint update through a receiver
time-advance grant, proves the grant is delivered without reflection, and maps
the terminal C++ `MessageCanNoLongerBeRetracted` boundary through Java and JPype.
The mixed-default-region vector fans one ordinary registration to three regional
subscribers using `flushQueueRequest`, `timeAdvanceRequestAvailable`, and
`nextMessageRequestAvailable`; all receive the C++ timestamped update before
their grant, with empty conveyed regions and one shared retraction handle.
The matching interaction vector fans one ordinary timestamped interaction to
three regional subscribers through those same FQR/TARA/NMRA request families;
each Java callback arrives before its grant with empty sent-region metadata,
`TIMESTAMP` send/receive order, and the same consumed retraction handle.
The timestamped-deletion vector similarly routes one C++ `Delete Object Instance`
to all three Java time-request modes before their grants, preserving removal
time/order metadata and mapping the consumed handle to
`MessageCanNoLongerBeRetracted`.
The directed-interaction vector exercises the standard
`sendDirectedInteractionWithTime` overload against the same three grant modes,
preserving target, reliable transport, producing-federate, timestamp/order, and
terminal retraction metadata through the directed callback.
The regional-retraction vector queues an overlapping source/receiver update,
retracts it while FQR/TARA/NMRA requests are pending, and proves all three
grants complete without reflection or `requestRetraction` callbacks.
The regional-interaction sequence adds the corresponding Java
`sendInteractionWithRegionsWithTime` path: a pre-grant retraction is suppressed,
the next timestamped interaction arrives after its grant, and a later callback
conveys the explicit source `RegionHandleSet`.
The matching regional-object vector routes a timestamped associated update
through FQR, TARA, and NMRA available-advance requests, preserving the
conveyed source region, `TIMESTAMP` order metadata, shared retraction handle,
and callback-before-grant ordering.
The multi-source regional-object matrix registers one attribute against two
source regions, isolates recipient delivery and conveyed-region metadata as
associations are removed, verifies default-region fallback after the final
explicit association is removed, and restores one source association alone.
It also repeats the complete source association and verifies the additive C++
association ledger is idempotent: each Java recipient receives one reflection
with the unchanged source-region set.
The multi-source regional-interaction matrix sends one region set through two
independently moving source ranges and proves lower-only, upper-only, both, and
fully disjoint recipient transitions through the standard Java region carrier.
The timestamped-save/NMRA vector exercises the exact Java
`nextMessageRequestAvailable` path at both sides of its strict boundary: the
equal first grant delivers the queued TSO without opening the save, and a
later grant invokes `initiateFederateSave` before its time-advance grant.
The companion timestamped-save/TARA vector proves the strict Available
boundary without a queued message: equality leaves the save pending, while a
later grant invokes `initiateFederateSave` before its Java grant callback.
The all-advance-mode timed-save vector joins five constrained Java members and
one regulator, requests TAR/NMR/TARA/NMRA/FQR at their qualifying boundaries,
and proves each constrained callback precedes its own grant while the regulator
receives save initiation after its grant.
The restore matrix then runs each exact Java advance service in an isolated
saved execution, reconstructs its pending grant through C++, and proves the
standard `federationRestored` callback precedes the reconstructed grant for TAR,
NMR, TARA, NMRA, and FQR; each service is submitted again after restore to
prove the Java surface is released for continued use.
The two-member interlock matrix holds both standard Java ambassadors in active
save and restore barriers and verifies declaration, object-registration, region,
time-query, synchronization, interaction, order-change, and every attribute-
ownership service fail with the corresponding C++-originated `SaveInProgress` or
`RestoreInProgress` before the peer completes the barrier. Ownership failures
also arrive as separately decoded standard `HLAreportException` interactions,
so the temporal guard and MOM reporting cross the same Java and JPype boundary.
The immediate/evoked scope vector preserves synchronous versus queued
`attributesInScope`/`attributesOutOfScope` callbacks, stale-transition
suppression, and advisory-switch gating through the standard Java callback
surface.
When using the IEEE API JAR, the bridge creates that interface through
`AttributeSetRegionSetPairListFactory` and supplies standard
`AttributeRegionAssociation` values. JNI reads their `ahset`/`rhset` fields;
the compact fixture's older accessor-pair carrier remains a compatibility
fallback only.
The Python adapter exposes the exact Java passive and universal declaration
method names alongside its keyword-based convenience methods. Interaction
class sets and pair lists are created through provider-neutral mutable builders,
while the external-JAR lane still invokes the official Java factory methods
through JNI before crossing into C++.
All standard advisory/reporting support switches now cross the same bridge:
mutable relevance, service-reporting, and exception-reporting values round-trip
against C++; the automatic-resign directive preserves the standard Java
`ResignAction` enum; provider-defined read-only support switches return their
C++ value; and `normalizeServiceGroup` takes the standard Java `ServiceGroup`
enum. The standard `setSendServiceReportsToFileSwitch` setter/getter also
round-trips per-federate state, and `getHLAversion()` returns the C++-owned
`IEEE 1516.1-2025` metadata string. The opt-in Python vector exercises those
public Python results rather than accessing JNI carriers directly.
Directed interaction declarations accept the standard Java
`Set<InteractionClassHandle>` carrier. Their C++ receive-order and timestamped
sends return through the distinct standard Java `receiveDirectedInteraction`
callbacks; target handles, time/order/retraction metadata, and declaration
teardown all reach the public Python adapter in the JNI integration vector.
All eleven standard ownership services now also route to the C++ ambassador.
The JNI federate-ambassador target declares all nine ownership callback
methods and the opt-in two-member Python vector proves the ownership
assumption, query/report, transfer, denial, and cancellation flows. It also
proves typed tag and handle conversion plus the C++ `AttributeHandleSet` out
parameter of `attributeOwnershipDivestitureIfWanted`. Discovered RTI-owned
joined-federate MOM objects now use the explicit standard
`attributeIsOwnedByRTI` callback without inventing a federate owner handle;
the same vector checks `isAttributeOwnedByFederate` remains false for that
object. A companion
real-JVM negative-path vector proves membership, unknown-instance, and
undefined-attribute ownership exceptions retain their standard Python types.
That vector also proves an if-available request against an already-owned
attribute produces the standard typed unavailable callback and leaves the
owner unchanged, while both regular and if-available requests from the owner
itself and requests that overlap a pending regular acquisition retain the standard
`FederateOwnsAttributes` and `AttributeAlreadyBeingAcquired` preconditions.
Repeated negotiated-divestiture and acquisition-cancellation requests retain
the standard no-pending exceptions as well, and confirming a divestiture
before any request maps to `AttributeDivestitureWasNotRequested`; the
duplicate negotiated request maps to `AttributeAlreadyBeingDivested`.
The C++ header, Java fixture, and JNI callback target now agree on all 56
standard `FederateAmbassador` method names and 62 callback overloads. The
separate IEEE C++ binding inventory reports 63 pure virtual members because it
also includes the class's pure virtual destructor; that is not an additional
callback. The
Python JNI vector verifies
declaration registration and interaction advisories plus both update-relevance
callback overloads, including an optional named update-rate designator; it
also proves a C++ duplicate synchronization-label failure reaches Python as
the standard `SynchronizationPointFailureReason` enum.
The Java fixture also has runtime declarations for all 109 concrete exception
names in the IEEE C++ header, matching the public Python exception registry.
This lets the JNI edge preserve native exception types through the Java proxy;
the integration vector verifies both an unknown FOM class (`NameNotFound`) and
a structurally valid unknown object-class handle (`InvalidObjectClassHandle`).
It also verifies that a Python exception raised by a federate callback is
contained at the callback boundary and returns from `evokeCallback` as the
standard `FederateInternalError`.

The external IEEE-JAR integration lane additionally enables the FOM's service
reporting switches, routes time-regulation services through Java/JPype, and
verifies that the C++ service-report JSON file records the accepted invocations.
It also routes direct, timestamped, and region-context `Send Interaction` calls
through the standard seven-parameter `HLAreportServiceInvocation` interaction
and decodes those parameters through the Java `EncoderFactory` at the JPype
callback boundary, including region-set, timestamped logical-time, and
message-retraction return arguments.
The standard MOM report-service declaration guard is exercised in both
directions: enabling service reporting rejects a report-stream subscription
with `FederateServiceInvocationsAreBeingReportedViaMOM`, while an existing
report subscription rejects enabling the switch with
`ReportServiceInvocationsAreSubscribed`.

The opt-in integration suite maintains a public-surface proof gate: all 184
abstract `hla.rti1516_2025.RTIambassador` services must be represented by a
real C++ → JNI → Java → JPype vector. With
`UMBRA_JNI_REQUIRE_RUNTIME_SERVICE_COVERAGE=1`, the suite also records calls
on the concrete JPype façade and requires every one of those services to run
at runtime. Separate structural tests prevent a fixture method from falling
through the Java dispatcher, an endpoint from missing native registration, or
a standard callback from losing its JNI/Python marshaller. A façade-state guard
also requires `NativeRTIambassador` to retain only its native C++ handle, so
Java cannot grow a parallel federation/ownership/time state model. This is
coverage accounting, not a substitute for the deeper state-machine conformance
cases retained in the C++ test suite.

Build and run the Java-side smoke test from this directory:

```powershell
.\build.ps1 -RunSmokeTest
```

The smoke run covers the Java `ServiceLoader`, native encoder, connection and
callback path, missing-federation reporting, and—when the checked-in 2025
Restaurant FOM is available—create/join/member-report/handle-lookup/resign/
destroy lifecycle calls. These operations all enter the C++ RTI through the
standard Java `RTIambassador`; the Java façade owns no federation state.

For a consumer or release build, pass the independently obtained standard API
JAR explicitly. It remains a dependency on the application class path; Umbra
does not repackage its declarations into the provider JAR.

```powershell
.\build.ps1 -JavaApiJar C:\path\to\ieee-1516-java-api.jar -RunSmokeTest
```

It produces:

- `umbra-rti-jni.jar` — the `ServiceLoader` Java façade;
- `umbra_rti_jni.dll` — the native bridge; and
- `umbra-mock-java-rti.jar` — only when `-JavaApiJar` is omitted, for the
  repository integration fixture.

Before publishing or handing the directory to the Python adapter, run the
release verifier against the same independently obtained API JAR. It checks
the API digest, required standard classes, absence of shadow API classes in
the bridge, exact `ServiceLoader` descriptors, native-library presence, and
the Java smoke path. `-ManifestPath` writes a reproducible file/digest record
without embedding or redistributing the API artifact:

```powershell
$bridgeDirectory = 'C:\path\to\umbra-rti-jni-build'
.\verify.ps1 `
  -ArtifactDirectory $bridgeDirectory `
  -JavaApiJar C:\path\to\ieee-1516.1-2025-java-api.jar `
  -ExpectedApiSha256 '<publisher-supplied-api-sha256>' `
  -ExpectedBridgeSha256 '<release-bridge-sha256>' `
  -ExpectedNativeSha256 '<release-native-sha256>' `
  -ManifestPath (Join-Path $bridgeDirectory 'umbra-jni-manifest.json')
```

The Python adapter accepts the same directory directly. If it contains one
top-level API JAR beside `umbra-rti-jni.jar`, that JAR is selected; multiple
siblings remain explicit-only so dependencies cannot be mistaken for the
standards API.

For a release-style staging directory, use the packaging helper. The API JAR
is copied only when `-IncludeJavaApiJar` is explicit, which keeps the standard
dependency and its redistribution decision visible:

```powershell
.\package.ps1 `
  -BuildDirectory $bridgeDirectory `
  -JavaApiJar C:\path\to\ieee-1516.1-2025-java-api.jar `
  -OutputDirectory (Join-Path $bridgeDirectory 'release') `
  -JavaApiCoordinate 'se.pitch.oss.fedpro:hla-4-api' `
  -JavaApiVersion '2.1.0' `
  -JavaApiSource 'https://repo1.maven.org/maven2/se/pitch/oss/fedpro/hla-4-api/2.1.0/hla-4-api-2.1.0.jar' `
  -IncludeJavaApiJar
```

The resulting directory contains `umbra-rti-jni.jar`, the platform native
library, `umbra-rti-jni-manifest.json`, and
`umbra-rti-jni-dependencies.json` (plus the copied API JAR when requested).
It can be supplied as `artifact_directory` to `JniRtiFactory`; the adapter then
uses the adjacent API JAR and the standard Java `ServiceLoader` route.

The Python integration test starts a fresh JPype JVM with those first two JARs
and `-Dumbra.rti.jni.library=<absolute DLL path>`. It is opt-in because it
builds the embedded C++ development profile:

The JNI adapter calls the standard `RtiFactoryFactory` API and selects the
bridge through its standard named-factory overload (`Umbra JNI C++ RTI`). This
still resolves through Java `ServiceLoader`; the name prevents an unrelated
provider descriptor in an application class path from winning discovery. The
generic JPype provider can leave `rti_factory_name` unset when the application
intentionally wants default `ServiceLoader` selection. The Python JNI adapter
does not contain a private factory implementation or a second RTI state model.
Applications that want this route through ordinary Python entry-point
discovery can install the small companion `umbra-rti-jni-python` package. It
only supplies artifact paths and delegates to `umbra-rti-jpype`; it does not
add a Python or Java RTI state model.

```powershell
$env:UMBRA_ENABLE_JNI_INTEGRATION_TESTS = '1'
$env:UMBRA_JNI_REQUIRE_RUNTIME_SERVICE_COVERAGE = '1'
python -m unittest packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py
```

To validate the external Java API route, build against the independently
obtained API JAR and point the opt-in Python suite at the same artifact:

```powershell
$bridgeDirectory = Join-Path $env:TEMP 'umbra-rti-jni-ieee'
.\build.ps1 -OutputDirectory $bridgeDirectory -JavaApiJar C:\path\to\ieee-1516.1-2025-java-api.jar -RunSmokeTest
$env:UMBRA_ENABLE_JNI_INTEGRATION_TESTS = '1'
$env:UMBRA_JNI_REQUIRE_RUNTIME_SERVICE_COVERAGE = '1'
$env:UMBRA_JNI_BRIDGE_ARTIFACT_DIRECTORY = $bridgeDirectory
$env:UMBRA_JNI_JAVA_API_JAR = 'C:\path\to\ieee-1516.1-2025-java-api.jar'
python -m unittest packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py
```

The external route runs 159 integration cases (all 159 passed) with 74
subtests, including 141 functional vectors, both named and no-argument
standard `RtiFactoryFactory` discovery paths, plus the artifact-level
non-shadowing/ServiceLoader gate. It includes the focused
federation-lifecycle exception vector (including same-member duplicate-join
mapping), integer/floating malformed logical-time
decode mapping, empty/short/trailing and structurally invalid standard
handle-factory decode mapping, C++-owned
float64 add/subtract/distance boundary arithmetic,
integer64 precision above 2^53 through the standard Java `long` carriers
and `getValue`/`distance` methods, and direct standard Java interval
add/subtract, standard carrier predicate/comparison/equality/hash-code methods,
canonical signed-zero construction and raw-wire decoding for
both Java and Python time carriers, all six standard `ResignAction` values with
post-resignation member reports, disconnect-while-member `FederateIsExecutionMember`
mapping, and direct standard Java two-argument scalar-`String` and `String[]`
federation-create, unnamed-join, and three-argument standard-MIM-create
overloads. It also lists two simultaneous federation executions and resolves
an extension-defined object class from a join-time additional FOM. The
resignation vector additionally routes `DELETE_OBJECTS` through C++ object
cleanup and verifies the surviving member receives the standard
`removeObjectInstance` callback with the departing owner handle and empty tag.
The membership callback vector also attempts standard Connect, Disconnect,
Join, and Resign calls from inside a Java callback and preserves C++
`CallNotAllowedFromWithinCallback` for all four attempts before a normal
resignation succeeds.
It also proves three-federate regional overlap/disjoint fanout, the official
message-retraction factory decode, the send-report switch, and
`getHLAversion()`. A two-dimensional regional-interaction vector additionally
requires overlap in both X and Y, then mutates one committed source region to
prove independent X-only and Y-only recipient transitions through the standard
Java region carriers. It also proves a two-member save/restore completion
barrier. The mixed callback-model vector additionally proves
HLA_IMMEDIATE/HLA_EVOKED notification ordering through the standard Java
callbacks. The companion two-dimensional regional-object vector first establishes known
object discovery, then routes reflections through independent X-only and Y-only
source-range mutations using the standard Java attribute/region pair-list
carrier. It also proves a two-member save/restore completion barrier, typed
member listing, requester-only restore acceptance callbacks, two-member
save and restore failure/abort reason propagation, and both
current-member and explicit two-member synchronization-point completion
barriers, the standard failed-achievement path with its typed failed-member
handle set, plus typed non-member synchronization-set failure mapping, and
synchronization edge cases through the same standard Java API: disconnected
and unjoined preconditions, unknown-label `SynchronizationPointLabelNotAnnounced`,
asynchronous duplicate-label registration failure, and idempotent repeated
achievement with one-shot completion callbacks. It also reuses a departed
peer's still-valid encoded `FederateHandle` in a later explicit set and retains
the asynchronous `SYNCHRONIZATION_SET_MEMBER_NOT_JOINED` failure. The three-federate
regional-object overlap/disjoint filter using the official pair-list factory,
and multi-region discovery reprojection with duplicate-discovery suppression and
selective unassociation. The external regional-object vector also keeps two
independently associated attributes separate across disjoint filtering,
association, selective unassociation, and default-region fallback. It also
proves C++ automatic provision queues the standard empty-tag
`provideAttributeValueUpdate` callback after discovery through the Java
`String[]` FOM-create overload. The same route drives the standard
`HLAsetSwitches` interaction to disable and re-enable later discovery-triggered
callbacks for both federation members. It also
proves timed-save admission waits for the
constrained member's qualifying boundary and orders initiation before that
member's grant callback, and delivers an in-transit timestamped interaction
before save initiation while preserving initiation-before-grant ordering, and
defers a save requested re-entrantly from that callback until the callback
returns. It also verifies restore rewinds the saved logical time and lookahead
after both are mutated post-save, including a float64-time round trip, and preserves a terminal TSO retraction
across restore while rejecting a post-save handle as invalid. Its
relaxed-DDM vector also proves boundary-touching regional interaction and
object-update delivery through the external Java region carriers. Its
regional interaction lifetime vector preserves
`RegionInUseForUpdateOrSubscription` until standard regional unsubscription
releases the region for deletion, while an independent observer decodes the
subscriber's `Delete Region` `HLAreportException` through the standard Java
encoder and JPype. Its
delayed-regional vectors also re-evaluate the committed subscription range at
callback time before delivering queued interaction and object updates. Its
timestamped regional-association vector suppresses a queued update after
unassociation and preserves typed TSO metadata after reassociation. Its
timestamped default-region object vector keeps the empty standard
`RegionHandleSet`, sent `TIMESTAMP` order, received `RECEIVE` order, and valid
retraction handle when an explicit source association is absent. Its
timestamped default-region interaction vector likewise keeps the empty
standard `RegionHandleSet`, `TIMESTAMP` sent/received order metadata, and the
consumed retraction-handle lifecycle through the external IEEE Java API. Its
directed-TSO vector limits retraction callbacks to recipients that actually
received the message. Its
restore vector also preserves a deferred lookahead decrease across restore
until the post-restore time advance makes it legal. Its
pending-time restore vector saves a constrained member while a time advance
is still pending, completes the save from the standard Java callback, rejects
time services during restore with the standard typed state exceptions, and
proves that the C++-reconstructed request emits the saved time-5 grant only
after `federationRestored`. Its HLA_IMMEDIATE companion saves a pending
`flushQueueRequest`, preserves `save-initiate`/`save-complete`/`flush-grant`
ordering, reconstructs the saved time-6 flush grant synchronously after
restore, and accepts a fresh time-7 flush request. Its
advance-variant matrix also restores pending `timeAdvanceRequestAvailable`,
`nextMessageRequest`, and `nextMessageRequestAvailable` requests under both
HLA_EVOKED and HLA_IMMEDIATE, preserving the saved time-5 grant and
post-restore usability for every standard Java overload. A dedicated NMR
vector then advances the C++ regulator to timestamps 5 and 7 and proves that
the standard Java request selects the two queued interactions in order,
preserving tags, timestamp/order metadata, and distinct retraction handles.
Its lifecycle
precondition vector invokes the exact scalar, `String[]`, and create-with-MIM
federation-management overloads before connection, plus destroy, join, and
resign, preserving C++ `NotConnected` through raw Java and JPype. It also
rejects an inconsistent logical-time FOM before the C++ registry reserves its
name. Its logical-time
implementation-mismatch vector also obtains carriers from separate C++-backed
providers and preserves typed `InvalidLogicalTime`/`InvalidLookahead` rejection
before mismatched bytes can reach a time service. Its regional-object
recipient-isolation vector also keeps two known subscribers separate as C++
source associations are added and selectively removed, preserving typed
`attributesInScope`/`attributesOutOfScope` callbacks and default-region
fallback delivery. Its
two-member live-retraction restore vector flushes a queued timestamped
interactions after restore, verifies their ordered typed time/order/retraction
metadata, and routes each restored handle's `requestRetraction` callback to the
receiving federate.
Its transport-loss vector uses an internal embedded-transport fault source only
to drive the exact standard Java `connectionLost` callback; it then verifies
C++ membership cleanup in the surviving member report and typed `NotConnected`
behavior for the lost JPype ambassador. The fault source is test-only and is
not an additional Java RTI service. The same vector verifies the C++
`ConnectionLost` service-report record and fault-description argument are
durable before callback eviction. An HLA_IMMEDIATE companion verifies
synchronous delivery and the same typed post-loss state.
Its companion MOM vector subscribes through the exact Java
`HLAreportFederateLost` interaction and decodes the C++ federate handle,
federate name, last-known logical time, and fault description with the Java
`EncoderFactory` before JPype delivers the callback to Python.
The exception-report vectors enable the C++ Exception Reporting Switch,
retain the typed `InteractionClassNotPublished` from standard Java
`sendInteraction`, `sendInteractionWithTime`, `sendDirectedInteraction`, and
`sendInteractionWithRegions`, and `sendInteractionWithRegionsWithTime`,
and decode the separate
`HLAreportException` service and exception strings through the Java encoder
factory.
The declaration-management companion enables both C++-owned reporting
switches, invokes the standard Java `subscribeInteractionClass` conflict for
`HLAreportServiceInvocation`, preserves the typed
`FederateServiceInvocationsAreBeingReportedViaMOM`, and decodes the separate
`HLAreportException` callback through the Java encoder factory.
The complementary switch vector subscribes the report stream first, then
preserves `ReportServiceInvocationsAreSubscribed` from the standard Java
`setServiceReportingSwitch(true)` call and decodes its separate exception
report through the same Java encoder path.
The time-management companion enables regulation through the standard Java
`enableTimeRegulation` call, then preserves the typed
`TimeRegulationAlreadyEnabled` from a duplicate request and decodes its
separate `HLAreportException` callback through the same Java encoder path.
The pending-regulation companion invokes standard Java
`enableTimeRegulation` twice before the first callback, preserves
`RequestForTimeRegulationPending`, and decodes its separate exception report.
Its constrained-mode companion performs the same duplicate-call check for
`enableTimeConstrained`, preserving `TimeConstrainedAlreadyEnabled` and
decoding a separate exception report.
The pending-constrained companion invokes standard Java
`enableTimeConstrained` twice before the first callback, preserves
`RequestForTimeConstrainedPending`, and decodes its separate exception report.
Its asynchronous-delivery companion performs the same duplicate-call check for
`enableAsynchronousDelivery`, preserving `AsynchronousDeliveryAlreadyEnabled`
and decoding a separate exception report.
Its inverse-state companion invokes `disableAsynchronousDelivery` while the
switch is off, preserving `AsynchronousDeliveryAlreadyDisabled` and decoding a
separate exception report.
The inverse time-role companions invoke `disableTimeRegulation` and
`disableTimeConstrained` while their modes are off, preserving
`TimeRegulationIsNotEnabled`/`TimeConstrainedIsNotEnabled` and decoding
separate exception reports.
The unregulated lookahead companions invoke `queryLookahead` and
`modifyLookahead` through the standard Java API, preserve
`TimeRegulationIsNotEnabled`, and decode separate exception reports.
The pending-lookahead companion invokes standard Java `modifyLookahead` while
a time advance is pending, preserves `InTimeAdvancingState`, and decodes its
separate exception report.
The pending-request companion invokes `timeAdvanceRequest` twice before the
first grant callback, preserves `InTimeAdvancingState`, and decodes a separate
exception report.
The regulation-pending companion invokes standard Java `timeAdvanceRequest`
and `timeAdvanceRequestAvailable` while `enableTimeRegulation` awaits its
callback, preserves `RequestForTimeRegulationPending`, and decodes separate
reports retaining each overload's service label.
The constrained-pending companion invokes standard Java `timeAdvanceRequest`
and `timeAdvanceRequestAvailable` while `enableTimeConstrained` awaits its
callback, preserves `RequestForTimeConstrainedPending`, and decodes separate
reports retaining each overload's service label.
The overload companion invokes `timeAdvanceRequestAvailable` twice before the
first grant callback, preserves `InTimeAdvancingState`, and decodes its
separately named exception report.
The next-message companion invokes `nextMessageRequest` twice before the first
grant callback, preserves `InTimeAdvancingState`, and decodes its separate
exception report.
The overload companion invokes `nextMessageRequestAvailable` twice before the
first grant callback, preserves `InTimeAdvancingState`, and decodes its
separately named exception report.
The flush-queue companion invokes `flushQueueRequest` twice before the first
grant callback, preserves `InTimeAdvancingState`, and decodes its distinct
`Flush Queue Request` exception report.
Its automatic-resign companion sets the standard Java `DELETE_OBJECTS`
directive before the test-only fault source, then verifies C++ cleanup through
the survivor's exact `removeObjectInstance` callback and stale-name rejection.
Its unconditional-divest companion retains the object and carries the exact
typed ownership-assumption callback, including the delete-privilege attribute,
to the surviving Java member.
Its delete-then-divest companion applies the standard
`DELETE_OBJECTS_THEN_DIVEST` directive through C++ transport loss, proving the
survivor receives one typed object-removal callback for the lost member's object
and one ownership-assumption callback for a retained transferred object; the
retained name remains resolvable while the deleted name is rejected.
The normal Java resignation path exercises the same combined directive without
the test-only transport fault and verifies the identical C++ deletion/divestiture
callbacks and survivor state.
Its forced-resignation vector uses a separate internal RTI membership-control
source to drive the exact standard Java `federateResigned` callback, verifies
survivor cleanup and duplicate-control rejection, and proves the connected
proxy can rejoin. An HLA_IMMEDIATE companion verifies synchronous delivery,
the same survivor cleanup, and rejoin through the still-live proxy. Neither
control input is part of the Java RTI service API.
The federate-lookup companion covers disconnected/unjoined preconditions,
cross-federation handle rejection, missing joined names, and preservation of a
departed member's immutable name from its returned `FederateHandle`.
The exception-surface check loads every C++ exception whose exact standard
Java class is present in the external API; the independently supplied API
remains authoritative for its vocabulary and intentionally omits a few legacy
advisory exception names.
An external-interface arity audit also covers every standard
`EncoderFactory.createHLA*` overload so a new creator cannot fall through the
JNI handler silently.
The malformed encoder matrix also covers all twenty-six primitive encoder
classes and all four standard composite forms, including malformed nested
composite children, proving that native decoder failures retain the standard
Java `DecoderException` type through JPype.
The same external lane drives the exact Java `ByteWrapper` cursor overloads
through both the raw Java surface and the provider-scoped Python façade for
primitive, fixed-record, fixed-array varargs, variable-length
ASCII/unicode-string, opaque-data, and variable-array carriers (including the
standard `resize` operation) with non-zero offsets and consumed-byte
advancement;
malformed string length prefixes, fixed-record/fixed-array child payloads,
variable-array counts, and signed integer primitive decoders preserve
`DecoderException` at the raw Java boundary as well as through the Python
adapter.
The complete JPype test-package discovery against the same external artifact
executes 202 discovered tests (180 passed, 22 deliberate skips, and 78
subtests).

The object-name reservation matrix also preserves standard validation and
state semantics through the external Java API: disconnected and unjoined
preconditions, illegal names and empty sets, asynchronous contention failure,
release-without-reservation, and partial batch success/failure callbacks.

The ownership edge vector also transfers an attribute to a second member and
then verifies that the former owner receives the standard `AttributeNotOwned`
exception through ordinary update/order/transport services and release-denied,
unconditional, if-wanted, and negotiated divestiture services.

The mixed `If Available` ownership vector submits one standard Java request
containing a remote-owned and an unowned attribute, then verifies that the
C++ planner emits recipient-local secured and unavailable callbacks with the
same copied tag and correctly encoded handles.

The mixed regular-acquisition vector submits one standard Java request with
two remote-owned attributes and one unowned attribute, then verifies that C++
emits a secured callback for the unowned attribute and one grouped release
request for the remote-owned subset. The owner denies only that subset,
yielding a typed unavailable callback while the local acquisition remains
owned by the requester.

That same vector queries the two ownership states together and verifies the
standard Java owner-report and not-owned callbacks remain separate through
JNI and JPype. A late-publication companion proves an unconditional
divestiture retains its C++ assumption search until a known candidate publishes,
then delivers the standard assumption set including the implicit delete
privilege through Java and JPype.

The resignation ownership vector also proves that `NO_ACTION` preserves the
standard `FederateOwnsAttributes` precondition, while
`UNCONDITIONALLY_DIVEST_ATTRIBUTES` routes the C++ ownership assumption to the
surviving Java member and permits its standard acquisition/query path.

The final-federate resignation vector also proves that a last member's
`NO_ACTION` resignation performs the C++ forced-delete pass: after rejoining,
the same object name is reserved and registered successfully through the
standard Java API.

The pending-resignation ownership vector preserves `OwnershipAcquisitionPending`
for `UNCONDITIONALLY_DIVEST_ATTRIBUTES` and verifies that the combined
`CANCEL_THEN_DELETE_THEN_DIVEST` action completes the C++ resignation path.

The save/restore resignation vectors preserve
`FEDERATE_RESIGNED_DURING_SAVE` and `FEDERATE_RESIGNED_DURING_RESTORE` through
the standard Java callbacks, reject stale save completion, and prove that the
remaining member can complete a subsequent save after the failed save operation.

The ownership/time boundary vector accepts a timestamped attribute update from
one C++-owned source, transfers that attribute through the standard Java
ownership services before the constrained recipient's grant, and then verifies
through JPype that the original producer handle, copied value/tag, timestamp,
`TIMESTAMP` order metadata, and live message-retraction handle are preserved.

The companion If Available order-reset vector sets the old owner's per-instance
attribute order to `TIMESTAMP`, transfers ownership through the standard
`attributeOwnershipAcquisitionIfAvailable` path, and verifies the acquiring
Java member's timestamped update returns an invalid retraction while the
callback carries `RECEIVE` sent/received order metadata. This keeps the C++
ownership/order state authoritative through JNI, Java, and JPype.

The companion restore vector saves the same C++ object while federate A owns its
attribute, transfers ownership to federate B after save completion, and restores
the image through both standard Java ambassadors. The post-restore ownership
queries prove the saved C++ owner is reinstated and the later transfer is gone.

The object-management restore companion saves a completed baseline before the
standard Java members publish/subscribe and register an object. After both
members complete restore, `getKnownObjectClassHandle` maps the C++
`ObjectInstanceNotKnown` result through JNI and JPype, proving the post-save
object and declarations were rolled back while the Java members remain joined.

The regional-object restore companion saves an overlapping source/subscriber
association, moves the source region to a disjoint boundary after save, and
restores through both standard Java ambassadors. The restored C++ range and
association then deliver a later update again, with the source `RegionHandle`
preserved at the JPype callback boundary.

The regional-interaction restore companion saves an overlapping regional
subscription, moves the source range to a disjoint boundary after save, and
restores the C++ range before sending through the standard Java interaction
overload. The callback is admitted again with the restored source
`RegionHandle`.

The joined-federate MOM state companion subscribes to the RTI-owned
`HLAfederateState` object, decodes its standard Java `HLAinteger32BE` values,
and proves save/complete and restore/complete state reflections (3/1 and 5/1)
through the C++ → JNI → Java → JPype callback route.

The regional joined-federate MOM companion subscribes to `HLAfederateName`
through the exact Java `AttributeSetRegionSetPairList` overload. It starts with
a disjoint range, then mutates the committed range onto the C++-owned immutable
`HLAfederate` point and proves discovery, reflection, requested reflection,
and removal for the matching observer while the disjoint observer remains
unaware. The RTI-originated callbacks retain the invalid producer handle and
empty tag expected for an RTI-owned MOM object.

The conditional-MOM companion subscribes to the complete joined-federate
attribute set and verifies that advisory switches, timing state, lookahead,
logical time, and time-manager transitions are reflected only through their
current C++ state. It drives all five standard time-advance/queue services and
the standard `HLAsetSwitches` interaction through both evoked and immediate
Java callback models, decoding the typed integer/boolean values with the
external Java `EncoderFactory`.

The FOM-snapshot companion uses the exact Java `String[]` Join overload with an
additional FOM module, then observes the RTI-owned `HLAFOMmoduleDesignatorList`
through standard MOM reflection. It decodes the `HLAmoduleDesignatorList` with
the external Java `EncoderFactory` and checks the corresponding C++ service
report, proving that the federate-scoped list contains only modules supplied at
Join.

The regional request/response vector invokes the standard Java
`provideAttributeValueUpdate` callback, answers through the same routed
ambassador, and verifies copied values, tags, transport, producer, and
conveyed region metadata on the returned reflection.
Its timestamped companion answers that callback with the standard Java timed
update overload and proves the C++ queue holds delivery until the constrained
grant, preserving `TIMESTAMP` order metadata, source-region conveyance, and
the consumed retraction handle.

The region-validation vector also preserves `InvalidRegion`,
`RegionDoesNotContainSpecifiedDimension`, and `InvalidRangeBound` for
incomplete regions, unrelated dimensions, invalid range bounds, and repeated
deletion through C++ → JNI → Java → JPype. The foreign-region vector
additionally preserves `InvalidRegion` for read-only lookups and
`RegionNotCreatedByThisFederate` for mutating set/commit/delete calls when a
peer supplies a structurally valid region handle.

The dimension-validation vector also preserves `InvalidDimensionHandle` for
unknown-but-structurally-valid dimensions through standard Java name,
upper-bound, and region-creation services, while retaining
`RegionDoesNotContainSpecifiedDimension` when that same handle is used against
an otherwise valid region.

The regional interaction exception vector preserves
`RegionNotCreatedByThisFederate`, `InvalidRegion`, and `InvalidRegionContext`
for foreign, uncommitted, and wrong-dimension subscription/send regions. With
exception reporting enabled on both originating federates, it also decodes
seven separate standard `HLAreportException` interactions through the Java
`EncoderFactory` and JPype, retaining the regional service name for each
subscription/send failure.

The regional-object exception vector applies the same typed vocabulary to the
official Java `AttributeSetRegionSetPairList` through registration,
subscription, and association services.

The regional attribute-value request vector also proves the empty-set no-op,
disjoint filtering, overlap admission, and the same typed foreign,
uncommitted, and wrong-dimension errors through
`requestAttributeValueUpdateWithRegions`.

The external lane also verifies save/restore precondition and overlap
exceptions (`SaveNotInitiated`, `FederateHasNotBegunSave`,
`SaveNotInProgress`, `RestoreNotRequested`, `RestoreNotInProgress`,
`SaveInProgress`, and `RestoreInProgress`) through C++ → JNI → Java → JPype,
including the opposite status query, synchronization registration/achievement,
declaration publication, named object registration, object update, object
deletion, direct/regional/directed/timestamped interaction sends, scalar and
timestamped federation-save requests, attribute-value requests,
ownership queries, order changes, transportation changes, region lifecycle,
regional declaration, and region association/unassociation while each
operation is active.

The integration suite audits every fixture `RTIambassador` name/arity route so
new Java overloads cannot silently fall through as unbound. It also verifies
that every `NativeBridge` declaration is registered by the C++ JNI table and
that Java dispatch calls only declared endpoints. The same audit compares every
Java `FederateAmbassador` overload against the C++ callback slots and JPype
marshaller; when an external IEEE JAR is selected, it first reflects the actual
loaded callback interface instead of trusting the compact fixture, verifies all
56 exact callback names, and constructs the actual standard JPype proxy. The audit
also ensures all callback-carrier global class references are released when the
native ambassador closes. It exercises timestamped federation
save end-to-end: Java encodes the selected logical time, JNI decodes it with
the C++ factory, and C++ supplies timed-save admission and the distinct
`initiateFederateSave(label, LogicalTime)` callback all the way to Python.

Both standard reference logical-time implementations are selected from the
joined C++ federation. Integer and float factory values, queries, lookahead,
and callback times therefore retain their declared Java and Python types.
