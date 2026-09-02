# IEEE 1516.1-2010 Java RTI Contract TCK

This is the transplantable Java test boundary for an IEEE 1516.1-2010 / 1516e
RTI. The source imports only `hla.rti1516e.*`; it does not import Umbra JNI,
C++, a vendor package, or the 2025 API. Compile once against the authoritative
2010 Java API archive and run the same classes with any provider JAR that
registers the standard `RtiFactory` through `RtiFactoryFactory`.

The executable Java scenarios cover factory discovery, primitive encoding
(including `ByteWrapper` cursor-preserving decode), the
172/60 API inventory, overload and exception identity, connect/disconnect, an
explicit FOM-gated federation membership lifecycle, FOM-gated declaration
handle/name and publish/subscribe transitions, FOM-gated object
register/discover/update/reflect callbacks, FOM-gated time-role /
time-advance callbacks, FOM-gated interaction send/receive and synchronization
point callbacks, ownership query/inform callbacks, and save/restore
initiation/completion callbacks, including the scalar and timestamped
`requestFederationSave` overloads and the typed `LogicalTime` callback carrier
(the fixture delivers the callback immediately and does not claim scheduled
persistence semantics). It also covers a FOM-gated DDM region
dimension/bounds and regional-subscription lifecycle using only standard
`hla.rti1516e` carriers, plus a standard-MIM-gated MOM service-reporting
interaction with all six `HLAreportServiceInvocation` parameters, serial
numbers starting at zero, and a single sent-region entry in supplemental
receive info. It also
exercises the standard `HLArequestPublications` →
`HLAreportObjectClassPublication` path, including the per-class payload and
the zero-count NULL response after unpublication. It also covers the matching
`HLArequestSubscriptions` → `HLAreportObjectClassSubscription` interaction,
including active/update-rate/attribute-list fields and its NULL response. The
MOM coverage also enables the standard exception-reporting switch and
asserts the targeted `HLAreportException` callback after an unknown-object
service; exception reporting remains explicitly disabled by default until that
switch is set. A missing FOM or MIM is reported as `unsupported`, never as a
2025-derived pass.
The MOM slice also verifies RTI-owned `HLAmanager.HLAfederate` discovery and
static `HLAfederateName` reflection, plus the one-per-federation
`HLAmanager.HLAfederation` object and static `HLAfederationName` reflection.
Malformed MOM interactions are also checked through the fully qualified
service name, `HLAparameterError`, and `HLAreportMOMexception` callback.
The same `HLAsetTiming` path is then exercised with a non-negative
`HLAreportPeriod` encoded as standard `HLAinteger32BE`; the valid control
interaction must not generate a second MOM exception.
The bounded timing companion arms the same standard interaction and checks
`HLAlogicalTime`/`HLAlookahead` reflection after one wall-clock period,
including caller-gated delivery for `HLA_EVOKED`, automatic delivery for
`HLA_IMMEDIATE`, and disablement when the period is zero. It intentionally does
not claim the remaining periodic MOM attribute matrix.
The object-instance-information request/report is checked against a registered
Restaurant object, including inherited federate targeting, the owned-attribute
handle list, and registered/known class handles.
The companion deletable-object count request/report verifies a positive
class/count projection and the empty response after the owned object is deleted.
The updated/reflected object-instance count request/report verifies distinct
instance class/count payloads, inherited federate targeting, callback metadata,
and empty NULL responses for federates with no matching activity.
The transport-count request/report verifies one update and reflection report per
standard reliable/best-effort transportation, provider-owned transportation
handles, nested class/count payloads, and explicit empty buckets.
The interaction-count request/report verifies the corresponding sent and
received interaction ledgers for reliable/best-effort transportation, nested
interaction-class/count payloads, and explicit empty buckets.
The FOM/MIM data-report scenario verifies federate-scoped FOM-module content
and federation-scoped FOM-module and MIM Unicode payloads, including module
indicators and reliable callback metadata.
The interaction publication/subscription report scenario verifies both
interaction-class lists and their empty NULL projections after unpublication
and unsubscription.
The synchronization-report scenario consumes the federation-scoped
`HLArequestSynchronizationPoints` and
`HLArequestSynchronizationPointStatus` interactions, decodes the standard
`HLAsyncPointList` and `HLAsyncPointFederateList` payloads through the Java
encoder, checks partial achievement status, unknown-label empty lists, and the
empty active-point projection after all federates achieve the point.
The service-reporting interlock scenario verifies the standard
`FederateServiceInvocationsAreBeingReportedViaMOM` exception and the
`HLAreportMOMexception` callback when enabling reporting would violate the
subscription precondition.

```powershell
.\build.ps1 -ApiJar C:\path\to\IEEE1516-2010-Java-API.jar
.\run.ps1 `
  -ApiJar C:\path\to\IEEE1516-2010-Java-API.jar `
  -ProviderJar C:\path\to\vendor-rti.jar `
  -DependencyJar C:\path\to\vendor-dependency.jar `
  -FactoryName 'Vendor RTI' `
  -FomPath C:\path\to\RestaurantFOMmodule.xml `
  -MimPath C:\path\to\HLAstandardMIM.xml
```

Both the Java and Python runners accept the same optional capability profile:

```powershell
.\run.ps1 `
  -ApiJar C:\path\to\IEEE1516-2010-Java-API.jar `
  -ProviderJar C:\path\to\vendor-rti.jar `
  -CapabilityProfile .\profiles\minimal-provider-2010.properties
```

The profile is a Java-properties file with `scenario.<stable-id>=run`,
`unsupported`, or `not applicable` entries. `run` and `pass` mean “execute the
assertion”; the other modes gate the provider call and remain visible in the
result. The file is a capability declaration, not a conformance claim, and it
can be passed unchanged to `run-python.ps1` because the shared file includes
both the `java-2010-tck.*` and `python-2010-tck.*` IDs. See
`profiles/mock-java-rti-2010.properties` and
`profiles/minimal-provider-2010.properties` for portable examples. The
`profiles/jni-null-provider-2010.properties` profile is reserved for the
repository's bounded JNI scaffold: it keeps the API inventory/exception
probes visible, runs the connection lifecycle slice, and marks the remaining
provider services `unsupported`, so a JNI boundary smoke cannot be mistaken
for a conformant RTI.

The resulting JSON is evidence for the 2010 scenario catalog and can be
transplanted unchanged to another pure-Java RTI. It is not a claim that a
provider implements every 2010 service; unsupported capability profiles stay
visible in the result.

The opt-in JPype fixture lane additionally drives the callback matrix with
both receive-order and timestamped object/interaction callbacks, both standard
integer64 and float64 logical-time factories, and scalar and timestamped
federation-save request overloads. Fractional float64 values are asserted after
the Java → JPype → Python conversion. Those checks prove Java carrier shape and
Python conversion; the deliberately small fixture does not claim real TSO
scheduling or persistence timing semantics. Failed and aborted saves also
exercise the standard restore-request failure callback.

When a capability profile is supplied, `run-python.ps1` first validates every
profile key against the checked-in Java/Python 2010 catalogs with
`tools/verify_1516e_capability_profile.py`. Misspelled or duplicate scenario
keys fail before the provider is started; the profile still only gates
execution and cannot turn a failed assertion into a pass. Direct invocation of
`jpype_smoke.py` applies the same catalog-linked validation.

The same provider-neutral runner can exercise the direct C++/pybind route;
this keeps the Python contract and scenario IDs identical across JPype, JNI,
and native providers. Build and stage `umbra-rti-native` so the extension is
under `umbra/_native/rti1516e`, then run:

```powershell
.\packages\umbra-rti-native\run-2010-tck.ps1 `
  -NativePackageDirectory .\out\native-2010-install `
  -CapabilityProfile .\packages\umbra-rti-java-tck-2010\profiles\native-2010-provider.properties
```

The native profile currently records fourteen passes (the exact 2010 Python
surface, overload/exception identity, scalar/opaque/composite encoders,
malformed-input rejection, integer/float logical-time arithmetic,
connect/disconnect, federation membership, declaration management, two-federate
receive-order object callbacks, object-attribute ownership query/inform
callbacks, two-federate receive-order interaction callbacks, and
synchronization-point registration/announcement callbacks) and explicit
unsupported results for the remaining MOM/RTI service families. Its result uses
`transport: python-native` and is verified/exported by the same catalog
tooling as the JPype result.

For the Python route, `jpype_smoke.py` performs the same factory discovery
(including handle decoders, mutable set/map/pair-list factories,
transportation defaults, and logical-time factory identity) and
connect/disconnect check through `umbra._java.rti1516e`. Its encoder gate also
checks the standard `ByteWrapper` decode cursor semantics, plus provider-owned
standard scalar and complex encoder round-trips, malformed scalar rejection,
the generated 2010 method/overload inventory, overload/exception identity, and provider-owned
logical-time arithmetic/comparison. With `-FomPath`, it also exercises the
standard URL overload and create/join/resign/destroy membership lifecycle,
class/attribute handle and declaration round-trips, a two-federate
register/discover/update/reflect object path, and a two-federate interaction
publish/subscribe/send/receive path, synchronization-point registration and
announcement callbacks, an ownership query/inform path, and
time-regulation/time-constrained enablement with a
time-advance grant, a DDM region/bounds and regional-subscription path, and
save/restore initiation/completion. With `-MimPath` it also exercises the
standard `createFederationExecution(URL[], URL)` overload, two-federate
service-reporting enablement, and RTI-originated MOM callback metadata. Its
stable scenario IDs are defined in
`compliance/catalogs/python-2010-tck-scenario-catalog.json`; verify and export
the result with `tools/verify_1516e_python_tck_results.py` and
`tools/python_tck_2010.py`. It requires the optional JPype runtime and
therefore remains an external-provider gate in this repository. Callback
delivery is drained for a bounded interval; a provider that connects but does
not produce a callback for that run is recorded as `unsupported`, not as a
false pass or an implementation failure.

To run that same Python route against a real vendor JAR, use the checkout
runner. It accepts provider dependency JARs and JVM options, then verifies and
exports the result automatically:

Before JPype starts, the runner performs the metadata-only 2010 provider-JAR
preflight: it requires the standard `hla.rti1516e.RtiFactory` service descriptor,
checks that each listed provider class is present in the provider/dependency
classpath, checks the supplied API JAR's canonical 2010 entries, and rejects a
mixed 2025 namespace. This is an onboarding guard, not a conformance result.

```powershell
.\run-python.ps1 `
  -ApiJar C:\path\to\IEEE1516-2010-Java-API.jar `
  -ProviderJar C:\path\to\vendor-rti.jar `
  -DependencyJar C:\path\to\vendor-dependency.jar `
  -FactoryName 'Vendor RTI' `
  -FomPath C:\path\to\RestaurantFOMmodule.xml `
  -MimPath C:\path\to\HLAstandardMIM.xml `
  -JvmOption '-Dvendor.setting=true' `
  -CapabilityProfile .\profiles\minimal-provider-2010.properties
```

The repository's ABI-only JNI scaffold can be exercised through exactly the
same Python route after building `packages/umbra-rti-jni-2010`:

```powershell
.\packages\umbra-rti-jni-2010\build.ps1 `
  -JavaApiJar .\out\java-tck-2010\ieee1516e-api-staged.jar `
  -OutputDirectory .\out\java-tck-2010\jni-2010-null
.\run-python.ps1 `
  -ApiJar .\out\java-tck-2010\ieee1516e-api-staged.jar `
  -ProviderJar .\out\java-tck-2010\jni-2010-null\umbra-rti-jni-2010.jar `
  -JvmOption '-Dumbra.rti.jni.2010.library=C:\path\to\umbra_rti_jni_2010.dll' `
  -CapabilityProfile .\profiles\jni-null-provider-2010.properties
```

That run is expected to report the API/exception and connection-lifecycle
probes as passes and the remaining provider-service scenarios as `unsupported`;
it verifies the JNI boundary, not IEEE 1516e service conformance.
