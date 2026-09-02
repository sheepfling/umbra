# Portable C++ RTI conformance TCK

The first portable C++ TCK slice is in
[`packages/hla-rti-cpp-tck`](../../packages/hla-rti-cpp-tck/). It is a
separate consumer package, not a collection of source-tree implementation
tests. Its source depends only on the official IEEE 1516.1-2025 C++ API
headers and the C++ standard library.
The runner uses a standard `RTIambassador` plus a direct
`FederateAmbassador` implementation. Factory and `RtiConfiguration` objects
are used only through the same official API to construct and configure those
objects.

## Green P0 and ownership slice

The initial green slice exercises:

- connection and both standard callback models;
- federation Create, Join, query, Resign, Destroy, and missing-execution reporting;
- federate, object, attribute, interaction, parameter, order, and transportation handle lookups;
- ordinary object and interaction declarations, including withdrawal;
- ordinary object registration and discovery;
- ordinary attribute Update/Reflect with value, tag, producer, and transportation checks;
- ordinary interaction Send/Receive with parameter, tag, producer, and transportation checks;
- named object registration and discovery;
- adapter-triggered connection loss, lost-member cleanup, and survivor usability.
- ordinary attribute ownership queries and checks;
- unowned query reports and unconditional-divestiture assumption offers;
- negotiated acquisition/divestiture with confirmation tags;
- direct Divestiture If Wanted transfer to a pending acquirer;
- If Available acquisition after unconditional divestiture;
- owner denial, requester Unavailable notification, acquisition cancellation,
  and negotiated-divestiture cancellation.

The executable runs the seven shared scenarios under `HLA_EVOKED` and
`HLA_IMMEDIATE`. Their runner IDs are the same IDs used by the Java TCK.
Connection loss is explicitly marked `skipped` unless the adapter supplies a
fault trigger. The IEEE API defines the callback and cleanup surface but does
not define a fault-injection service.

The ownership scenario is `java-tck.ownership`. It keeps the reusable source
on the standard `RTIambassador`/`FederateAmbassador` boundary and uses only the
ordinary `DivestAcquire` attribute in the portable FOM. It verifies ownership
reports and user tags across the negotiated, immediate-availability, denial,
and cancellation paths; no DDM, time-management, MOM, or provider-private
surface is required.

## Portability boundary

The generic CMake project accepts these adapter inputs:

- `HLA_RTI_TCK_API_INCLUDE_DIR`: the official API include directory;
- `HLA_RTI_TCK_PROVIDER_LIBRARIES`: provider library files or imported targets;
- `HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS`: provider ABI/build definitions;
- `HLA_RTI_TCK_PROVIDER_LINK_OPTIONS`: provider-specific linker inputs.

The reusable source does not name or discover a provider. A provider adapter is
responsible for its runtime search path, endpoint configuration, and any
provider-owned process fixture. FOM class and interaction names are command
line parameters, so a provider can reuse the test logic with an equivalent
portable fixture without editing the test source.

Run the source-boundary validator from the repository root:

```powershell
python tools/cpp_tck.py
```

Build and execute with the package scripts:

```powershell
.\packages\hla-rti-cpp-tck\build.ps1 `
  -ApiIncludeDirectory C:\path\to\official\include `
  -ProviderLibrary C:\path\to\provider.lib `
  -ProviderCompileDefinition STATIC_RTI `
  -BuildDirectory .build\cpp-tck

.\packages\hla-rti-cpp-tck\run.ps1 `
  -Executable .build\cpp-tck\Release\hla_rti_cpp_tck.exe `
  -ProviderId provider-build `
  -Results .build\cpp-tck\p0-results.json `
  -JUnit .build\cpp-tck\p0-results.xml
```

The machine-readable evidence can be checked with:

```powershell
python tools/cpp_tck.py --results .build\cpp-tck\p0-results.json
```

To prove the downstream boundary, configure the installed-package adapter
against a provider installation. The adapter obtains the standard include path
and provider link targets through `find_package`; the portable TCK source does
not include provider headers or link private libraries directly. Package
registry lookup is disabled; only the explicit installation prefix is used:

```powershell
.\packages\hla-rti-cpp-tck\adapters\current-package\build-and-test.ps1 `
  -PackagePrefix .build-fom-services\package-smoke-install `
  -BuildDirectory .build\cpp-tck-installed `
  -Configuration Debug
```

This runs the `hla_rti_cpp_tck_installed_p0` CTest with the adapter’s FOM and
callback configuration. `RtiAddress`, `RtiConfigurationName`, and
`AdditionalSettings` are available as adapter options and are passed only
through the standard `RtiConfiguration` API.

The catalog is
[`compliance/catalogs/cpp-tck-scenario-catalog.json`](../../compliance/catalogs/cpp-tck-scenario-catalog.json).
Each entry maps a runner ID to the standard API methods, a source symbol, and
the pinned Requirements Lab contracts.
