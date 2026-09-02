# Portable C++ HLA TCK

`hla-rti-cpp-tck` is a vendor-neutral IEEE 1516.1-2025 C++ test executable.
The reusable source includes only the official `RTI` headers and the C++
standard library. It does not include a provider transport, private provider
headers, a provider test fixture, or a test framework.
The test logic calls services through `RTIambassador` and receives them through
a direct `FederateAmbassador` implementation; the standard factory and
configuration types are used only to construct and configure that boundary.

The provider adapter supplies the official API include directory, provider
libraries, compile definitions, and link options at configure time. The
reusable target has no provider include-directory input and no provider-specific
setting embedded in its source.

## P0 and ownership slice

The executable covers these ordinary public-API workflows, under both
`HLA_EVOKED` and `HLA_IMMEDIATE` callback models by default:

| Scenario | Standard surface exercised |
| --- | --- |
| `java-tck.overloads-and-exceptions` | Create an ambassador, Connect, Disable/Enable Callbacks, Disconnect |
| `java-tck.federation-membership` | Create/Join/Resign/Destroy, federation and member reports, federate handle lookups, missing-federation report |
| `java-tck.support-services` | Object, attribute, interaction, parameter, order, and transportation handles in both directions |
| `java-tck.declaration-management` | Ordinary object publication/subscription and interaction publication/subscription, including withdrawal |
| `java-tck.object-management` | Ordinary and named registration, discovery, object-name/class lookups |
| `java-tck.attribute-interaction` | Ordinary attribute Update/Reflect and interaction Send/Receive, values, parameters, tags, producer, and transportation |
| `java-tck.ownership` | Ownership queries (including unowned reports), assumption offers, negotiated and If Wanted acquisition/divestiture, If Available acquisition, unconditional divestiture, denial, and cancellation |
| `cpp-tck.connection-loss-cleanup` | Adapter-triggered Connection Lost callback, fault description, survivor service, and cleanup |

The seven shared cases reuse the Java TCK scenario IDs. The connection-loss case
is a C++ adapter extension and is skipped when no adapter trigger is supplied.
The standard API has no fault-injection service, so the adapter either provides
a command run after both federates join or starts a provider-managed loss
fixture and passes `--connection-loss-server-managed`.

The ownership case uses the ordinary `DivestAcquire` attribute in the portable
FOM and runs the same transfer, denial, cancellation, ownership-query, and
user-tag assertions under both standard callback models.

## Build against a provider

The only required input is the directory containing the official
`RTI/RTI1516.h` and related headers. Provider inputs are intentionally explicit:

```powershell
.\build.ps1 `
  -ApiIncludeDirectory C:\path\to\official\include `
  -ProviderLibrary C:\path\to\rtI.lib `
  -ProviderCompileDefinition STATIC_RTI
```

For providers that expose a CMake package, an adapter can translate imported
targets into `-ProviderLibrary` values. The reusable package does not call
`find_package` for a named provider.

## Run

```powershell
.\run.ps1 `
  -Executable C:\path\to\hla_rti_cpp_tck.exe `
  -ProviderId provider-build-2025 `
  -Results .\p0-results.json `
  -JUnit .\p0-results.xml
```

Use `-Scenario` one or more times to select a slice. Use
`-CallbackModel evoked` or `-CallbackModel immediate` to run one callback
model. FOM names and the standard `RtiConfiguration` address/settings are
configurable through the runner for providers whose fixture uses different
names.

For an adapter-owned trigger command, the executable substitutes
`{federation}`, `{owner}`, and `{member}` before invoking it:

```powershell
.\run.ps1 `
  -Executable C:\path\to\hla_rti_cpp_tck.exe `
  -Scenario cpp-tck.connection-loss-cleanup `
  -ConnectionLossCommand 'C:\path\to\trigger-loss.exe {federation} {member}' `
  -ConnectionLossMarker C:\temp\connection-loss.ok
```

For an installed provider package, use the downstream adapter project. It
calls `find_package`, consumes the exported standard-header and provider-target
interfaces, and runs the P0 CTest without adding private include directories:
the adapter disables CMake package-registry fallback and uses only the explicit
`PackagePrefix`.

```powershell
.\adapters\current-package\build-and-test.ps1 `
  -PackagePrefix C:\path\to\installed-provider-package `
  -BuildDirectory .build\cpp-tck-installed `
  -Configuration Release
```

The JSON and JUnit outputs are suitable for CI artifact collection. A skipped
connection-loss case is explicit in both formats and does not masquerade as a
pass.
