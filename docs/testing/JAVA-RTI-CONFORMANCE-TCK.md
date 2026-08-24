# Java RTI conformance TCK

`packages/umbra-rti-java-tck` is the provider-neutral Java test boundary for
Umbra and other IEEE 1516.1-2025 Java RTIs.  Its sources import only the
standard `hla.rti1516_2025` API.  Provider selection is runtime configuration:

```text
API JAR + provider JAR(s) + factory name + FOM/MIM paths
```

The TCK deliberately does not import `org.umbra.jni`, `NativeBridge`, or any
C++-specific report format.  Consequently, the same compiled test classes can
exercise a pure Java vendor RTI or Umbra's C++ → JNI → Java provider.

## Test boundaries

| Layer | Purpose | Provider-specific? |
| --- | --- | --- |
| Java TCK | Exact Java factory, ambassador, encoder, callback, handle, time, and standard exception behavior | No |
| JPype contract suite | Java provider consumed through the public Python API | No, except Python/JPype behavior |
| JNI integration suite | C++ semantics crossing JNI and becoming standard Java values/callbacks | Umbra JNI |
| Native C++ suite | C++ state machines and embedded RTI semantics | Umbra C++ |

The runner has executable scenarios for factory/lifecycle, encoders and
malformed octets, membership/callback delivery, logical-time factories,
declarations, object registration/reflection, API-surface inventory, overloads
and standard exceptions, malformed FOM input, DDM region lifecycle,
save/restore, ownership, synchronization, MOM, and a two-federate
time-regulation/time-constrained advance with grant callbacks. The checked-in
Umbra JNI profile passes all fifteen scenarios. Every family remains an
explicit catalog scenario with `unsupported` or `not applicable` available
when a provider lacks the needed capability; scenarios never silently
disappear from the evidence.

Every result carries a stable scenario ID, stable requirement IDs, standard
Java API method linkage, provider identity, capability profile, and an evidence
artifact path. `-ResultsPath` writes the machine-readable evidence and
`-JUnitPath` writes JUnit XML suitable for existing compliance tooling.

## Porting to another RTI

Compile once against the authoritative IEEE API JAR, then run with the other
RTI's provider class path:

```powershell
.\packages\umbra-rti-java-tck\build.ps1 `
  -ApiJar C:\deps\hla-4-api-2.1.0.jar `
  -OutputDirectory out\java-tck

.\packages\umbra-rti-java-tck\run.ps1 `
  -ApiJar C:\deps\hla-4-api-2.1.0.jar `
  -ProviderJar C:\vendor\work-rti.jar `
  -FactoryName 'Work RTI' `
  -FomPath C:\fom\minimal.xml `
  -ClassesDirectory out\java-tck
```

For a two-provider run, put two provider configurations in a JSON array and
use the same compiled classes:

```powershell
.\packages\umbra-rti-java-tck\run-matrix.ps1 `
  -ConfigurationPath .\packages\umbra-rti-java-tck\matrix.example.json
```

Each configuration may change only the API/provider JARs, factory name,
FOM/MIM inputs, native library transport, and capability profile. The included
`profiles\pure-java-mock.properties` is a deliberately limited unrelated
pure-Java ServiceLoader fixture; a complete vendor profile can enable the
behavioral families it implements.

Umbra's JNI run adds only `umbra-rti-jni.jar` and
`-NativeLibrary`; those are transport details, not part of the TCK contract.

## Requirements Lab export

`compliance/catalogs/java-tck-scenario-catalog.json` is the portable traceability source
of truth. Validate it against the pinned Lab bundle and export one or more
provider runs with:

```powershell
python tools/java_tck.py validate
python tools/java_tck.py export `
  --results out\java-tck\matrix\umbra-jni.results.json `
  --results out\java-tck\matrix\pure-java-provider.results.json
```

The export joins all Java API surfaces in the Lab bundle, all referenced
compliance-contract requirements, provider statuses, JUnit/evidence artifacts,
and the documented portable exclusions. It is an Umbra-owned compliance
artifact and does not claim protected-review acceptance by the Requirements
Lab.

## JPype handoff

After the direct Java TCK passes for a provider, the same API/provider JARs can
be checked through JPype:

```powershell
.\packages\umbra-rti-java-tck\run-jpype.ps1 `
  -DirectResults out\java-tck\matrix\umbra-jni.results.json `
  -ApiJar C:\deps\hla-4-api-2.1.0.jar `
  -ProviderJar C:\deps\umbra-rti-jni.jar `
  -NativeLibrary C:\deps\umbra_rti_jni.dll `
  -FactoryName 'Umbra JNI C++ RTI'
```

The JPype smoke lane is separate from the portable Java source set. It gates on
the direct result artifact, then discovers the same standard `RtiFactory`,
encoder factory, and ambassador through JPype.
