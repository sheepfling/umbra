# Python IEEE 1516-2010 / 1516e coverage plan

This is the edition-specific plan for the long-running 2010 Python route. It
deliberately mirrors the completed 2025 layering without treating 2010 as an
alias of the 2025 contract.

## Surface-first success boundary

The active goal is surface completeness and bindability, not an overnight
reimplementation of every 2010 RTI service. A surface-complete result means
that the authoritative `rti1516e` / `hla.rti1516e` symbols, overloads,
callbacks, exceptions, encoder/time types, factory entry points, and provider
load/import/link boundaries are present and mechanically exercised across the
pure-Python, JPype, direct-pybind, and JNI routes. Stateful scenarios are
bounded proof slices only. DDM, MOM, save/restore, and the remaining time and
RTI services stay visible as explicit capability-profile `unsupported` gates
until a later implementation tranche; their presence in the catalogs does
not imply that Umbra implements them today.

The completion criterion for this goal is the callable surface itself: a
provider may expose a method and deliberately report `unsupported` when that
service is invoked. A surface audit must therefore check names, overload
shapes, callback carriers, factories, namespaces, and load/link boundaries
without requiring every method to perform RTI work. Behavioral proof slices
are additive evidence, not prerequisites for declaring a symbol bound.

## Current inventory

As of 2026-08-24, the repository contains:

- Requirements Lab exports for the 2010 editions of Parts 1, 1.1, and 1.2;
- SISO FOM metadata whose XML namespace is
  `http://standards.ieee.org/IEEE1516-2010`; and
- a checked-in provenance manifest and immutable official 2010 C++ headers,
  plus a generated `hla.rti1516e` Python contract; and
- a bounded C++/pybind/JNI provider whose Java API JAR remains caller-supplied.
  The direct C++/pybind route now owns the scalar and opaque encoder families,
  both mandated integer64/float64 logical-time representations, connection
  lifecycle, the fixed-array, variable-array, fixed-record, and variant-record
  composite encoder families, and a deterministic federation/declaration /
  receive-order object and interaction callback slices, object-attribute ownership
  query/inform, and synchronization-point registration/achievement callbacks;
  the remaining RTI/MOM services remain
  intentionally unsupported.

The first implementation gate was provenance and API-surface inventory. The
contract now records the official Java method names, overload counts, and
parameter/return-type tuples (172 RTIambassador declarations and 60
FederateAmbassador declarations). We still must not infer a 2010 signature
from the 2025 declarations or from a vendor fixture.

With locally staged IEEE downloads, run:

```powershell
python tools\verify_1516e_artifacts.py `
  --part1-archive C:\path\to\1516.1-2010_downloads.zip `
  --part2-archive C:\path\to\1516.2-2010_downloads.zip
```

The verifier checks the committed archive digests, extracts only the API
archives in memory, and confirms the `rti1516e`/`hla.rti1516e` namespaces plus
the initial C++ abstract-surface counts.

## Namespace and package policy

The canonical Python package is `hla.rti1516e`, matching the official Java
package. There is intentionally no `hla.rti1516_2010` re-export: the edition
boundary is explicit in the standard namespace and avoids making 2010 values
assignable to the independently versioned 2025 package.
The existing
`hla.rti1516_2025` package remains independent, and neither package will
re-export the other's handles, exceptions, callbacks, encoders, or time
values. A neutral umbrella may select an edition, but the selected provider
must expose one concrete edition contract at a time.

The standard Java 2010 surface uses `hla.rti1516e` in the official API
materials. Umbra's implementation classes will live below
`org.umbra.jni.rti1516e`; they must not shadow `hla.rti1516e`. The standard C++
surface uses the
`rti1516e` namespace (the conventional HLA Evolved spelling), with Umbra
internals kept outside that public namespace. The C++ and Java surfaces are
not textually identical: C++ exposes handle decoders and legacy transportation
names, while Java exposes handle factories, passive-subscription overloads,
and supplemental callback accessors. The Python contract intentionally mirrors
the Java method groups; native adapters must map the C++-only names explicitly.
`RtiFactoryFactory` discovers providers through the edition-specific
`hla.rti1516e.factories` group, while `LogicalTimeFactoryFactory` honors any
standalone time-provider entries and otherwise discovers the standard time
families through each selected 2010 `RtiFactory` ambassador. This preserves the
Java factory-factory shape without requiring a second unconfigured provider
registry.
Run `tools/generate_1516e_surface_inventory.py` against the locally staged
official sources to produce the machine-readable parity evidence.

## Adapter boundary

The 2010 route will have the same four observable layers as 2025:

1. provider-neutral Python contracts and value types;
2. direct C++/pybind provider, when a stateful 2010 C++ implementation is
   available;
3. standard Java 2010 provider consumed through JPype; and
4. the separate Umbra JNI 2010 null boundary, followed by stateful slices
   without introducing a second Java or Python RTI state model.

The Python layer adapts Java overloads and carrier objects, while the Java
provider remains authoritative for encoder bytes, logical-time arithmetic,
handle identity, callbacks, and federation state. Typed encoder and
logical-time façades now expose those provider-owned operations without
reimplementing their semantics. A vendor Java JAR remains usable through the
generic JPype adapter even when Umbra has no stateful 2010 implementation.

## Work sequence

1. Add provenance manifests and immutable checksums for the 2010 C++ headers,
   Java API JAR, and 2010 FOM/DIF/OMT resources.
2. Generate a 2010 C++/Java abstract-surface inventory and compare it with the
   2025 inventory, recording removed, renamed, and signature-changed services.
3. Extend the existing pure `hla-rti-api` distribution with an independent
   `hla.rti1516e` contract namespace. A second wheel would collide with the
   same `hla/` paths, so the edition boundary is the subpackage and its
   provider entry-point group, not a duplicate distribution. Include
   versioned exceptions, encoders, handles, callbacks, time, and factory
   surfaces.
4. Build a provider-neutral Java TCK that imports only `hla.rti1516e` and uses
   the same lifecycle, declaration, object, time, DDM, ownership,
   synchronization, save/restore, MOM, overload, and malformed-input scenario
   families as the 2025 TCK, with 2010-specific IDs.
5. Add generic `JavaRtiFactory` discovery for a 2010 JAR and keep the separate
   Umbra JNI 2010 null bridge available as the native ABI checkpoint.
6. Add direct Python, Java/JPype, and JNI evidence artifacts and map each
   scenario to the 2010 Requirements Lab IDs. Unsupported behavior must be an
   explicit profile result, never an inherited 2025 pass.

Steps 1–5 have an initial implementation and the cross-language inventory now
records the intentional C++/Java name differences. Step 6 is the active long-running
gate: the Java TCK is green against the proxy fixture, and the JPype adapter
now covers provider-owned encoders, logical-time arithmetic, handle factories,
pair-list arguments, supplemental callback records, and a generated forwarding
matrix for all 172 RTI overloads and all 60 callback overloads in fake-runtime
tests. The focused adapter tests also exercise standard return-carrier conversion
(handles, logical-time/query records, and message-retraction results), all
edition-specific enum members, and every standard exception constructor. Regional
registration also preserves provider-owned object-handle identity through the
adapter. The vendor checkout runner now performs a metadata-only
2010 `ServiceLoader`/namespace preflight before starting JPype, and validates
capability-profile keys against both transport catalogs so a typo cannot
silently omit a scenario.
Unknown vendor logical-time and interval carriers are deliberately kept as
raw Java objects at the Python boundary, including values returned through a
custom `LogicalTimeFactory`; factory construction, decode, callback, and
ambassador query results all preserve that identity. Dedicated 12-octet
fixtures also prove that unknown 2010 and 2025 factories own variable-width
decode and malformed-wire decisions; the Python edge does not truncate them
to the eight-octet reference shape. Source-safe and opt-in JNI tests cover
that preservation without claiming vendor arithmetic semantics. The Python
value facades additionally encode those wide carriers into non-zero-offset
buffers with surrounding sentinels, proving that the selected provider width
and caller-owned bytes survive the final Python write step. The provider
arithmetic vector also records the edition-specific ownership boundary: a
2025 vendor wrapper maps a provider `IllegalTimeArithmetic` to the Python
exception, while an unknown 2010 carrier remains raw and exposes that Java
exception unchanged. The same opt-in JNI
lane now carries a provider-owned `DataElement` with exact, empty, truncated,
and overlong payload vectors, preserving its generic Python wrapper, cursor,
and provider-selected `DecoderException` behavior. The same carrier is
encoded into exact-origin, offset-with-trailing, truncated, and empty
`ByteWrapper` windows, preserving bytes/cursor state and mapping undersized
destinations to `EncoderException`. Standard float64
carriers with negative, non-finite, or otherwise malformed numeric values are
rejected before wrapping, while the live mock-provider matrix proves that
fractional valid values survive Java → JPype → Python.
The Python façade also checks the destination window against the provider
encoded length before entering JNI, so undersized Python `ByteWrapper`
destinations fail deterministically without relying on a vendor's raw Java
exception path. The provider-neutral adapter test uses a tracking foreign
carrier to assert that this preflight does not invoke `encode` after the
capacity check fails.
The standalone Java `NativeTypeRoundTripTest` and the Python-driven direct Java
carrier loop both encode every reflected 2010 data-element kind through a
non-zero-offset `ByteWrapper`, preserving the trailing sentinel and exact
cursor advancement; this complements the Python façade's shared 79-case
reverse-encode matrix.
The 2010 encoder fallback also treats Java interface identity as authoritative:
a vendor class whose name merely contains `HLAinteger32BE` remains a generic
`DataElement` instead of being guessed into the standard scalar wrapper. A
provider-defined logical-time factory that exposes only the required sentinel
and decode methods reports the missing numeric convenience creators explicitly
as `NotImplementedError`. Raw provider-owned time carriers returned by a
callback or query can also be supplied back to a time-taking Java service: the
adapter obtains their exact encoded bytes through the carrier's Java encoder
instead of assuming a Python `toByteArray()` helper.
The transplantable Python runner now emits thirty-four catalog-linked scenarios and
has passed an actual JPype JVM run against the standard `hla.rti1516e` API
classes plus the fixture JAR, including proxy encoder dispatch, standard
`ByteWrapper` cursor-preserving decode, standard handle/
collection/DDM pair-list/transportation/logical-time factory discovery, and
integer time arithmetic. The same runner now covers overload/exception identity, an
optional FOM-gated create/join/resign/destroy lifecycle, declaration/object
management, ownership query/inform callbacks, interaction send/receive,
synchronization-point registration / announcement callbacks,
time-role/time-advance callbacks, DDM dimension/region bounds and regional
subscription forwarding, MOM service-reporting callbacks, RTI-owned
`HLAmanager.HLAfederate` discovery/reflection, publication and subscription
request/report plus NULL-response callbacks, exception-reporting enablement and
`HLAreportException` callbacks, RTI-owned `HLAfederation` discovery,
malformed MOM interaction reporting through `HLAreportMOMexception`, and
object-instance-information request/report callbacks through the official 2010
MIM, service-reporting subscription/enabling interlocks, deletable-object and
updated/reflected object-instance count request/report callbacks, transportation-
bucketed update/reflection invocation-count reports (including explicit reliable
and best-effort NULL buckets), interaction sent/received invocation-count reports
with the same transportation buckets, federate/federation FOM-module and MIM
text reports, interaction publication/subscription reports with NULL lists,
federation synchronization-point list/status reports across partial and completed
achievement, bounded `HLAsetTiming` delivery of `HLAlogicalTime` and
`HLAlookahead` in both `HLA_EVOKED` and `HLA_IMMEDIATE` callback models, plus
save/restore
initiation/completion callbacks. A real
vendor-JAR smoke, complete
complex-encoder/callback scenario matrix, and the remaining stateful 2010
C++/pybind/JNI service families remain external-provider gates. The official Java encoding
interface names are mechanically checked with
`tools/verify_1516e_encoding_contract.py`.

The provider-neutral Python adapter suite now also consumes the shared
`save_restore_matrix.py`: all 720 combinations of callback model, integer or
float time carrier, scalar or timestamped save, the five standard advance
services, one/two-member
membership, and complete/not-complete/abort save and restore outcomes are
forwarded through the 1516e Java-shaped surface. This remains a surface and
argument-shape check; it does not promote a bounded provider into a complete
2010 save/restore implementation.

The opt-in `test_jpype_2010_mock_integration.py` lifecycle lane builds the
same fixture through `ServiceLoader` and drives complete, not-complete, and
abort save/restore callbacks for both callback-model labels, both standard
integer64/float64 time factories, and one- or two-member federations. It
exercises both scalar and timestamped `requestFederationSave` overloads and
asserts the timestamped `initiateFederateSave(String, LogicalTime)` carrier
after the Java → JPype → Python conversion, including fractional float64
values. The compact fixture delivers that callback immediately; it does not
model a scheduled save boundary or persistence timing semantics, so broader
time-based scheduling remains represented by the provider-neutral forwarding
matrix rather than being over-claimed as RTI behavior. Failed and aborted
saves also exercise the standard restore-request failure callback, proving the
save/restore interlock label path without claiming persistence semantics.

The same live callback matrix now sends both receive-order and timestamped
object/interaction updates for each two- or three-member, immediate/evoked
case. It asserts the eight-argument Java callback overloads and converts the
embedded integer64 and fractional float64 `LogicalTime` values back to
provider-neutral Python carriers. The fixture remains intentionally immediate
and does not claim TSO queueing or vendor-specific ordering beyond the
documented partial-order assertions.

The JNI carrier lane additionally walks every 2010 `FederateAmbassador`
callback name and all 60 generated overloads for both the integer64 and
float64 logical-time carriers. JNI-produced handles, sets, maps, callback
supplement records, save/restore arrays, enums, strings, and byte arrays must
arrive at the Python callback as their provider-neutral types. This is a
carrier conversion matrix, not a claim that the null JNI ambassador delivers
stateful callbacks; callback delivery and state-ordering remain in the
provider-neutral and mock-provider lanes.

The same runner has also been exercised against the repository's explicit
`umbra-rti-jni-2010` ABI/null-provider scaffold. Its provider JAR passed the
metadata preflight, Python discovered the standard `RtiFactory`, and the
profiled result retained the 150/172/51/60 inventory and exception probes plus
the bounded connection lifecycle as passes while recording the remaining
provider-service scenarios as `unsupported`. This is boundary evidence only:
the scaffold still intentionally throws `RTIinternalError` for every service
outside connect/disconnect; stateful service evidence remains pending.

The direct `umbra-rti-native` extension is now covered by the same runner with
`--native-provider`. Its `UmbraNative2010` entry point passes the generated
surface, overload/exception, scalar/opaque/composite-encoding,
malformed-input, integer/float-time, connection-lifecycle,
federation-membership, declaration, receive-order object, receive-order
interaction, ownership, and synchronization-point scenarios and records the
remaining 20 provider scenarios as `unsupported`. This is the
direct C++/Python counterpart to the JNI result, not a claim of full 2010 RTI
behavior. The native pytest surface gate additionally instantiates the
provider and checks all 150 standard method names, asserting that each
generated gap is an explicit named `NotImplementedError` rather than a missing
or accidentally cross-edition method.

The transplantable Python smoke entry point is
`packages/umbra-rti-java-tck-2010/jpype_smoke.py`; it exercises factory
discovery, connect/disconnect, the generated API inventory, typed scalar and
complex encoder round-trips, overload/exception identity, an optional FOM
membership lifecycle, declaration handle/name and publish/subscribe state,
two-federate object registration/discovery and attribute update/reflection,
two-federate interaction publish/subscribe and send/receive with parameter
payloads, synchronization-point registration-success and announcement
callbacks, time-regulation/time-constrained enablement and time-advance grants,
MIM-gated service-reporting and six-parameter MOM report callbacks plus
RTI-owned `HLAfederate` object discovery/reflection, publication request/report
and subscription request/report callbacks, both publication/subscription NULL
responses, exception-reporting enablement through `HLAreportException`, and
RTI-owned `HLAfederation` discovery/reflection, object-instance-information
request/report callbacks with owned-attribute, registered-class, and known-class
handles, and
provider-owned logical-time arithmetic when an optional JPype runtime and
vendor JAR are present; `--native-provider` selects the direct C++/pybind
entry point without a JVM. Results are checked and exported through
`tools/verify_1516e_python_tck_results.py` and `tools/python_tck_2010.py`.
The paired catalog invariant is checked by
`tools/verify_1516e_tck_parity.py`; it permits only the explicitly listed
Python-only encoder/time probes and the `ddm`/`ddm-management` transport ID
alias.
The declaration and object scenarios accept `--object-class` and
`--attribute-name`; the interaction scenario accepts `--interaction-class` and
`--parameter-name`, so a vendor run can select the corresponding names from
its 2010 FOM rather than changing the test source.

The provider-neutral catalog intentionally names the full family matrix so a
different provider can declare capabilities without changing test source. The
implemented Umbra proof slice is factory/encoding, lifecycle, declaration,
object, interaction, synchronization, and ownership. The generated Python
contract and JPype adapter expose the full 2010 DDM and MOM method/type
families, but those families are not implied to be native service
implementations. DDM has a stateful, FOM-backed
fixture scenario, and the official MIM-backed MOM service-reporting,
service-reporting subscription/enabling interlock,
RTI-owned-object, publication request/report, subscription request/report,
exception-reporting, federation-object, malformed-MOM-exception, and
object-instance-information, deletable-object-count, and updated/reflected
object-instance-count slices are stateful and callback-verified. The
bounded `HLAsetTiming` slice is stateful and callback-verified as well: it
arms `HLAlogicalTime`/`HLAlookahead` for one wall-clock period, keeps
`HLA_EVOKED` delivery behind `evokeMultipleCallbacks`, delivers the same
projection automatically for `HLA_IMMEDIATE`, and disables later reports at
period zero. It does not claim the remaining periodic MOM matrix. The
transport-count slice is stateful and callback-verified as well: it asserts one
`HLAreportUpdatesSent` and one `HLAreportReflectionsReceived` response per
standard transportation bucket, provider-owned transportation handles, nested
class/count payloads, and empty reliable/best-effort arrays when a federate has
no activity. The interaction-count slice is stateful and callback-verified too:
it asserts sent and received ledgers for reliable and best-effort interactions,
provider-owned interaction-class handles in the nested count payload, and empty
transport buckets for federates with no activity. The FOM/MIM data-report slice
also verifies federate-scoped FOM-module and federation-scoped FOM/MIM Unicode
payloads and module indicators. The
publication slice asserts one
`HLAreportObjectClassPublication` per published class and the zero-count NULL
response after unpublication. The exception slice asserts the disabled-by-
default switch and the targeted exception payload after an unknown-object
service. The malformed-MOM slice asserts the fully qualified service name and
parameter-error flag, then accepts a valid non-negative `HLAreportPeriod`
(`HLAinteger32BE`) without a second `HLAreportMOMexception`. The
timing companion then verifies one-second `HLAseconds` delivery of
`HLAlogicalTime`/`HLAlookahead` for both `HLA_EVOKED` and `HLA_IMMEDIATE`, plus
period-zero disablement. The 2010 Requirements Lab export has no dedicated
timing IDs, so that slice is mapped to the authoritative 2010 MIM boundary
only. The
object-instance-information slice asserts inherited
federate targeting, object-handle round-trips, owned-attribute handle-list
encoding, and registered/known class handles. The deletable-object-count slice
asserts inherited federate targeting, a positive class/count payload, and an
empty response after deletion. The updated/reflected count slice asserts
distinct-instance class/count payloads, inherited federate targeting, callback
metadata, and NULL responses for federates with no matching activity. The
service-reporting interlock slice asserts the standard
`FederateServiceInvocationsAreBeingReportedViaMOM` exception and
`HLAreportMOMexception` failure path. The service-reporting slice also asserts
serial numbers starting at zero and incrementing per invocation, plus one
sent-region entry in the supplemental receive record. Broader MOM request/report families,
exception-reporting interlocks, and vendor/native provider evidence remain
explicit gaps rather than being inferred from the 2025 route.

The completed publication-report slice is linked to
`req-68e20e7c8fc1`, `req-4fbdaebebd19`, `req-b45cedc503e6`,
`req-cdffc08fc1c2`, and `req-8f3fcf5c4732`.
The completed subscription-report slice is linked to
`req-cdad00911fe1`, `req-0e405d18e4af`, `req-82571a22ce64`, and
`req-f79a72cae198`. The object-instance-information slice is linked to
`req-f27af81e0869` and `req-774e30fa5c44`. The completed federation-level
`HLAmanager.HLAfederation` object is linked to `req-3214ebb081d7` and
`req-2ef657fd277e`. The completed updated/reflected count slice is linked to
`req-7c7b15e12a6a` and `req-38dc81937b52`. The transport-count slice is linked
to `req-eb90a8d28da5`, `req-385d5b57ade9`, `req-23b416361e0e`,
`req-b52022c1d6b6`, `req-65eda9e65f03`, `req-05501777546a`, and
`req-a3463ac669ae`. The FOM/MIM data-report slice is linked to
`req-f5b71efaf0d3`. The interaction publication/subscription slice is linked to
`req-fb571fad20bc`, `req-8e751ef9923d`, `req-d5ce409d795e`,
`req-0c3fccfc1f38`, and `req-3e46df94e2d8`. The synchronization-report slice is
linked to `req-6143e62f8408`, `req-0a914b04a1ec`, `req-6903e8739211`,
`req-6d9621de8866`, `req-d7f25ac7f3d1`, `req-d5be17a200d6`,
`req-4443a0453b11`, `req-7c17d1300084`, and `req-d3585f388c10`.
The directed-interaction names present in Umbra's newer 2025 inventory are not
present in the authoritative 2010 Java or C++ API archives, so they are an
intentional edition boundary rather than missing 2010 Python methods. Do not
back-port those names into `hla.rti1516e`; any future 2010 scenario must first
be sourced from a 2010 artifact. The completed
service-reporting serial/region semantics are covered by the MOM service-report
scenario. The completed
service-reporting subscription/enabling interlock slice is linked to
`req-2cf048d37300` and `req-304790c162d4`.

### Shared capability profiles

The Python runner accepts `--capability-profile` (also exposed by
`run-python.ps1`) and consumes the same Java-properties format as the pure-Java
2010 TCK. Entries use stable transport-specific IDs:

```properties
scenario.python-2010-tck.factory-discovery=run
scenario.python-2010-tck.save-restore=unsupported
```

`run` and `pass` execute the provider assertion. `unsupported` and `not
applicable` gate it before JPype/JVM provider code is called and remain in the
result with an explicit message. The profile cannot turn a failure into a
pass, and its resolved path is emitted as `capability_profile` for auditability.
The shared examples under
`packages/umbra-rti-java-tck-2010/profiles/` include both Java and Python IDs,
so one file can be used for the Java and Python routes. This makes a vendor's
capability declaration transplantable without changing the test source or the
Requirements Lab mapping.
The standalone `jpype_smoke.py` entry point performs the same catalog-linked
ID and duplicate-key checks, so bypassing `run-python.ps1` does not weaken the
profile boundary.

The catalog gate also parses the checked-in `hla.rti1516e` source contract and
rejects an `api_methods` reference that names a missing class or method. This
keeps a transplanted Java/Python scenario tied to the 2010 surface itself (and
not merely to a requirement ID); the timing probe consequently uses the
standard `LogicalTimeFactory.makeEpsilon()` operation rather than a concrete
factory convenience method.

## Compatibility rules

- `hla.rti1516e` and `hla.rti1516_2025` objects are never interchangeable.
- A 2010 FOM namespace does not make a 2025 API provider 2010-compliant.
- A Java vendor JAR may be tested through JPype without claiming Umbra's C++
  implementation supports 2010.
- Every 2010 binding claim needs an exact API artifact, a runtime provider,
  and a transplantable test/evidence record.

## Evidence matrix

The Java and Python catalogs use parallel, transport-specific scenario IDs but
share the pinned 2010 Requirements Lab requirement, mapping, and transition
references. The Java TCK now mirrors the Python route's declaration, object,
interaction, synchronization, DDM, and all seventeen MOM slices, so those
scenarios can be
transplanted between the two routes without changing their requirement
mapping. A scenario can therefore be transplanted to a different provider
without changing its requirement mapping.
The current evidence boundary is:

| Layer | Provider under test | Evidence | Status |
| --- | --- | --- | --- |
| Pure Python contract | No provider (abstract/value surface) | `packages/umbra-rti-api/tests/test_2010_contracts.py`; `tools/verify_1516e_python_surface.py` | Green |
| Python type inventory | Official Java API source tree | `tools/verify_1516e_python_types.py` | Green; all Java type names exported |
| Cross-language inventory | Official staged 2010 C++ and Java sources plus generated Python contract | `tools/generate_1516e_surface_inventory.py` | Green; C++/Java differences explicit |
| Edition boundary | Python 2010 entry point versus native/JNI transport registration | `tools/verify_1516e_provider_boundaries.py` | Green; bounded 2010 C++/JNI transports are separate from 2025 |
| Python adapter | Fake runtime and dynamic Java-shaped proxies | `packages/umbra-rti-jpype/tests/test_java_2010_provider.py`; `tools/verify_1516e_python_surface.py`; `tools/verify_1516e_provider_jar.py` | Green; all generated RTI/callback overloads and representative return carriers are forwarded, adapter metadata retains the generated parameter/return overload shapes, and provider-JAR service/provider-class/namespace preflight is executable |
| Python → JVM | Official `hla.rti1516e` API classes plus stateful Umbra mock fixture JAR | `packages/umbra-rti-java-tck-2010/jpype_smoke.py`; `out/java-tck-2010/python-serial-full.json` | Green (34/34 with Restaurant FOM + official MIM; the twenty-six FOM/MIM-gated scenarios are explicit unsupported without their artifacts) |
| Java TCK | Same mock fixture through the standard Java package | `packages/umbra-rti-java-tck-2010/src/main/java/umbra/rti/tck1516e/RtiTck2010Main.java`; `out/java-tck-2010/java-serial-full.json` | Green (31/31 with Restaurant FOM + official MIM; membership, declaration, object, interaction, synchronization, DDM, MOM, time, ownership, and save/restore are explicit unsupported without their artifacts) |
| Python → vendor Java RTI | A supplied vendor JAR discovered by its standard factory | `packages/umbra-rti-java-tck-2010/run-python.ps1` plus the same smoke/catalog | Pending external provider |
| Python → native 2010 | Bounded `rti1516e` C++ implementation via direct pybind11 or JNI | `packages/umbra-rti-native` (`UmbraNative2010`) and `packages/umbra-rti-jni-2010`; shared Python TCK/profile gates plus `compliance/catalogs/ieee1516e-2010-jni-type-roundtrip-catalog.json` | Direct route green for surface/factory discovery, scalar/opaque/composite encoding, the shared 79-case basic-value matrix, integer/float time, connect/disconnect, federation membership, declaration management, receive-order object and interaction callbacks, ownership query/inform, and synchronization-point callbacks (15 TCK pass/20 unsupported). The JNI carrier matrix is green for all 24 standard encoding types, both Java `DataElement.decode` overloads with offset/cursor assertions, the same provider-owned 79-case basic-value matrix, nine C++ handle families plus Java-only transportation handle, logical-time/handle/collection factories, sets/maps/pair lists, records, callbacks, and a 28-case malformed-wire matrix (24 truncations plus four declared-length overruns) preserving Java `DecoderException` through Java and JPype. Its opt-in Python route also consumes all 16 shared integer/float logical-time wire vectors through the exact standard Java factory proxy, including offset windows, C++-owned signed-zero canonicalization, and typed malformed decode failures, and round-trips all ten standard handle families (including provider-owned opaque `RegionHandle` and message-retraction values), all four typed handle sets, the `FederationExecutionInformationSet`, attribute/parameter maps, and the attribute-region pair-list through the Python façade. Logical-time arithmetic remains an explicit null-provider gap and is covered by the direct native and provider-neutral Java profiles. The JNI carrier lane additionally invokes every standard time/interval arithmetic method and verifies the Java return-category casts without assigning numeric semantics to the null provider. `tools/verify_1516e_python_surface.py --check-native` independently audits all 150 façade methods. Unsupported service families are intentional gates, not missing surface symbols |

“Green” here means the scenario and its traceability machinery run end to end;
the JNI Python carrier evidence additionally converts all ten standard handle
families (including the two provider-owned opaque families), all four typed
handle sets, the `FederationExecutionInformationSet`, six standard return
records, and three supplemental callback records into provider-neutral Python
values. A provider-issued `MessageRetractionHandle` is also retained and
passed back through the standard `retract` argument path for each of the four
standard return-producing services (`updateAttributeValues`, `sendInteraction`,
`deleteObjectInstance`, and `sendInteractionWithRegions`), proving the complete
return-to-service carrier loop. The provider-return matrix also feeds all 50
generated non-void RTI service overloads with JNI-produced Java return carriers
under both integer64 and float64 logical-time implementations, and normalizes
JPype primitive wrappers to the provider-neutral Python scalar types. Factory,
`getHLAversion`, and connection methods remain separately covered by their
explicit façade paths. The factory-specific matrix now resolves the named
provider through the standard `hla.rti1516e.RtiFactoryFactory`/`ServiceLoader`
route, consumes its standard `EncoderFactory`, and checks all 150 ambassador
method names plus the 172 generated overload declarations on both the raw Java
proxy and the Python façade before completing connect/disconnect. This keeps
factory discovery and symbol bindability transplantable independently of
stateful RTI behavior. A companion argument-dispatch matrix feeds the other
169 non-lifecycle overload declarations for both integer64 and float64 time
configurations through provider-neutral Python values and standard factory
carriers into the real JNI ambassador; the null provider returns the expected
`RTIinternalError` for each service. This closes the input-side binding check
without converting an unsupported service into a behavior claim.
it does not turn the mock fixture into a production RTI. Vendor and native
rows become green only when their provider-specific artifacts are supplied.
