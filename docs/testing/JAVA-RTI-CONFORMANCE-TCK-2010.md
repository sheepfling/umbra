# IEEE 1516.1-2010 Java RTI conformance TCK

`packages/umbra-rti-java-tck-2010` is the provider-neutral 1516e test
boundary. Its Java source imports only the official `hla.rti1516e` package,
so the compiled classes can be run unchanged against a pure-Java vendor RTI,
an Umbra JNI provider once one exists, or the included proxy fixture.

The thirty-one executable scenarios are deliberately layered:

- factory/encoder discovery;
- exact 172 `RTIambassador` and 60 `FederateAmbassador` method counts;
- overload and exception namespace checks;
- connect/disconnect;
- FOM-gated create/join/resign/destroy membership; and
- FOM-gated declaration handle/name and publish/subscribe transitions; and
- FOM-gated object register/discover/update/reflect callbacks; and
- FOM-gated interaction publish/subscribe/send/receive callbacks; and
- FOM-gated synchronization-point registration and announcement callbacks; and
- FOM-gated dimension/region bounds and regional subscription lifecycle; and
- FOM/MIM-gated MOM service-reporting enablement and six-parameter report
  callback; and
- FOM/MIM-gated service-reporting subscription/enabling interlocks, including
  the standard exception and MOM failure-report paths; and
- FOM/MIM-gated RTI-owned `HLAmanager.HLAfederate` discovery and static
  attribute reflection; and
- FOM/MIM-gated publication request/report interactions, including the
  object-class publication report and its NULL response; and
- FOM/MIM-gated subscription request/report interactions, including the
  object-class subscription report and its NULL response; and
- FOM/MIM-gated exception-reporting switch and `HLAreportException` callback; and
- FOM/MIM-gated RTI-owned `HLAmanager.HLAfederation` discovery and static
  federation-name reflection; and
- FOM/MIM-gated malformed MOM interaction reporting through
  `HLAreportMOMexception`; and
- FOM/MIM-gated bounded `HLAsetTiming` delivery of `HLAlogicalTime` and
  `HLAlookahead` in both `HLA_EVOKED` and `HLA_IMMEDIATE` callback models; and
- FOM/MIM-gated object-instance information request/report callbacks,
  including owned-attribute, registered-class, and known-class handles; and
- FOM/MIM-gated deletable-object count request/report callbacks, including the
  zero-count response after the object is deleted; and
- FOM/MIM-gated updated/reflected object-instance count request/report
  callbacks, including positive class/count payloads and NULL responses; and
- FOM/MIM-gated update/reflection invocation-count request/report callbacks,
  one report per reliable/best-effort transportation and explicit empty
  buckets; and
- FOM/MIM-gated interaction sent/received invocation-count request/report
  callbacks, one report per reliable/best-effort transportation and explicit
  empty buckets; and
- FOM/MIM-gated federate/federation FOM-module and federation MIM text
  request/report callbacks; and
- FOM/MIM-gated interaction publication/subscription request/report callbacks,
  including NULL interaction-class lists; and
- FOM/MIM-gated federation synchronization-point list and per-federate status
  reports, including pre-achievement, partial-achievement, unknown-label, and
  completed-point projections; and
- FOM-gated time-role enablement and time-advance/grant callbacks; and
- FOM-gated ownership query/inform callbacks; and
- FOM-gated save/restore initiation and completion callbacks.

Every result has a stable ID in
`compliance/catalogs/java-2010-tck-scenario-catalog.json`. The catalog uses
the pinned `hla-1516.1-2010` Requirements Lab document boundary. Verify its
requirement, mapping, and transition references with:

```powershell
python tools\verify_1516e_tck_catalog.py
python tools\verify_1516e_tck_results.py out\java-tck-2010\script-results.json
python tools\java_tck_2010.py out\java-tck-2010\script-results.json \
  --provider "Umbra mock Java 1516e"
```

### Capability profiles

`run.ps1` accepts `-CapabilityProfile` using the same Java-properties format as
the Python/JPype runner. Each `scenario.<id>` entry is `run`/`pass`,
`unsupported`, or `not applicable`. Gated scenarios are recorded explicitly
with the message “no provider assertion was made”; a profile cannot create a
pass or change a Requirements Lab mapping. The selected profile path is
included in the result JSON as `capability_profile`.

The checked-in examples are
`packages/umbra-rti-java-tck-2010/profiles/mock-java-rti-2010.properties` and
`packages/umbra-rti-java-tck-2010/profiles/minimal-provider-2010.properties`.
The latter demonstrates a core-only provider: the five non-FOM scenarios run,
while the twenty-six FOM/MIM-backed service families are explicit `unsupported`.

This is intentional: a 2025 requirement ID is not silently treated as a 2010
requirement. API inventory scenarios carry no transition ID when they do not
exercise a state-machine service.

## 2010 factory compatibility note

The downloaded 2010 Java `RtiFactoryFactory` calls
`javax.imageio.spi.ServiceRegistry`, which modern JDKs reject for the HLA
`RtiFactory` category (`not an ImageIO SPI class`). The TCK and JPype adapter
attempt that exact standard helper first, then use Java `ServiceLoader` only
for this identified runtime defect. The fallback is printed/recorded as a
compatibility event; it is not presented as a standards change.

The first executable gate was run with the local 2010 API classes and the
proxy fixture: thirty-one scenarios passed when the Restaurant FOM and official
`HLAstandardMIM.xml` were supplied, and the same run reports the membership,
declaration, object, interaction, synchronization, DDM, MOM, time, ownership,
and save/restore scenarios as explicit `unsupported` when no FOM/MIM is
configured.

## Python/JPype evidence

The same fixture can exercise the Python route through the exact Java
interfaces, rather than a bespoke Python mock. Build the fixture and invoke
the transplantable runner with JPype installed:

```powershell
python packages/umbra-rti-java-tck-2010/jpype_smoke.py `
  --api-jar C:\path\to\2010\api\classes `
  --provider-jar C:\path\to\vendor-rti.jar `
  --results out\java-tck-2010\python-jpype-smoke.json
python tools/verify_1516e_tck_catalog.py `
  --catalog compliance/catalogs/python-2010-tck-scenario-catalog.json
python tools/verify_1516e_python_tck_results.py `
  out\java-tck-2010\python-jpype-smoke.json
python tools/python_tck_2010.py out\java-tck-2010\python-jpype-smoke.json `
  --provider "Vendor Java 1516e"
```

For a checkout-level vendor run, `packages/umbra-rti-java-tck-2010/run-python.ps1`
first invokes `tools/verify_1516e_provider_jar.py`. That metadata-only guard
requires the 2010 `META-INF/services/hla.rti1516e.RtiFactory` descriptor, checks
that each listed provider class exists in the provider/dependency class path,
checks the canonical API entries, and rejects a mixed `hla.rti1516_2025`
namespace. It does not load classes or make a conformance claim; the JPype smoke
and this TCK remain the behavioral gates.

The Python catalog covers factory discovery (including standard handle,
collection, DDM pair-list, transportation, and logical-time factory families),
provider-owned scalar and complex
encoding, the 150-method/172-overload RTI inventory and 51-method/60-overload
callback inventory, connect/disconnect, provider-owned logical-time
arithmetic/comparison, and (when a FOM is supplied) declaration handle/name
round-trips, a two-federate register/discover/update/reflect path, a
two-federate interaction publish/subscribe/send/receive path,
synchronization-point registration/announcement callbacks,
time-regulation/time-constrained enablement with a time-advance grant, and
save/restore initiation/completion callbacks. The Python route additionally
exercises ownership query/inform callbacks and a DDM region lifecycle using
the standard dimension, range-bounds, region-set, and attribute-region-pair
types. With the official 2010 MIM it also drives the standard
`HLAsetServiceReporting` interaction and decodes all six parameters of the
RTI-originated `HLAreportServiceInvocation` callback, including serial numbers
starting at zero and a single sent-region entry in the supplemental receive
record. With the official MIM it
also drives `HLArequestPublications`, asserting the per-class
`HLAreportObjectClassPublication` payload and the zero-count NULL response. A
companion `HLArequestSubscriptions` scenario asserts the active
`HLAreportObjectClassSubscription` payload and its zero-count NULL response. A
companion exception-reporting scenario verifies the disabled-by-default switch
and the targeted `HLAreportException` payload after an unknown-object service.
A federation-object scenario verifies the single RTI-owned
`HLAmanager.HLAfederation` instance and its static `HLAfederationName` value.
Malformed MOM interactions are checked through the fully qualified service name,
parameter-error flag, and `HLAreportMOMexception` callback; the same
`HLAsetTiming` path also accepts a non-negative `HLAreportPeriod` encoded as
standard `HLAinteger32BE` without producing a second MOM exception.
The companion timing scenario arms that same control with a one-second
`HLAseconds` period, verifies `HLAlogicalTime`/`HLAlookahead` through both
`HLA_EVOKED` and `HLA_IMMEDIATE`, and verifies that period zero disables later
reflections. The 2010 Requirements Lab export has no dedicated timing IDs, so
this slice is mapped to the authoritative 2010 MIM boundary only.
An object-instance-information request/report also verifies inherited federate
targeting, owned-attribute handle-list encoding, registered/known class handles,
and callback metadata. A provider that cannot expose a selected convenience
operation is recorded as `unsupported`; it is never promoted to a pass from the
2025 route.
The deletable-object report slice verifies class/count payloads and the empty
projection after deleting the last owned object. The updated/reflected count
slice then verifies the standard `HLAreportObjectInstancesUpdated` and
`HLAreportObjectInstancesReflected` reports, including distinct-instance
counting, inherited `HLAfederate` targeting, callback metadata, and NULL
responses for federates with no matching activity.

The Python route also verifies RTI-owned `HLAmanager.HLAfederate` discovery and
static `HLAfederateName` reflection. The publication-report slice is linked to
`req-68e20e7c8fc1`, `req-4fbdaebebd19`, `req-b45cedc503e6`,
`req-cdffc08fc1c2`, and `req-8f3fcf5c4732`. The object-instance-information
slice is linked to `req-f27af81e0869` and `req-774e30fa5c44`; the
federation-level MOM object is covered by `req-3214ebb081d7` and
`req-2ef657fd277e`, and the deletable-object count slice is linked to
`req-794d62353902`. The updated/reflected count slice is linked to
`req-7c7b15e12a6a` and `req-38dc81937b52`. The service-reporting interlock
slice is linked to `req-2cf048d37300` and `req-304790c162d4`. The
federate/federation FOM-module and MIM data-report slice is linked to
`req-f5b71efaf0d3`. The synchronization-report scenario is additionally
tracked against the 2010 federation-management MOM requirements
`req-6143e62f8408`, `req-0a914b04a1ec`, `req-6903e8739211`,
`req-6d9621de8866`, `req-d7f25ac7f3d1`, `req-d5be17a200d6`,
`req-4443a0453b11`, `req-7c17d1300084`, and `req-d3585f388c10`.
The interaction publication/subscription slice is linked to
`req-fb571fad20bc`, `req-8e751ef9923d`, `req-d5ce409d795e`,
`req-0c3fccfc1f38`, and `req-3e46df94e2d8`. Broader MOM behavior
(directed-interaction families, exception-reporting interlocks, and
vendor/native-provider evidence) remains a separate explicit gap; these slices
do not claim total MOM conformance.
