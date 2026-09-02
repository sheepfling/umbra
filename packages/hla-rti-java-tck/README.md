# HLA Java RTI Contract TCK

This package is a vendor-neutral executable test set for an IEEE 1516.1-2025
Java RTI provider. The Java sources import only JDK classes and the official
`hla.rti1516_2025` API. Provider selection happens through the standard
`RtiFactoryFactory`/Java service-loader boundary.

Use the [Java TCK guide](../../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
the evidence model and provider-matrix workflow.

## Scope

The catalog contains 21 scenarios. The default lane covers factory discovery,
standard encoding, federation creation/join/resign/destroy/query, ordinary
publication/subscription, object and attribute registration/reflection,
ordinary interaction delivery, support lookups, negative cases, relevance
advisories, directed interactions, and transportation/order controls.

Ownership, synchronization, DDM, time-managed delivery, save/restore, and MOM
are separate capability-profile lanes. A provider profile can enable a lane
only when the provider and supplied FOM support it.

The ordinary tests use the IEEE Restaurant example names by default, including
`HLAobjectRoot.Employee.Server`/`Efficiency`,
`HLAobjectRoot.Food.Drink`/`NumberCups`, and
`HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed`.
Directed/relevance scenarios default to
`HLAinteractionRoot.ServerAction.TakeOrder`. Supply a compatible FOM or
override these names with the `hla.rti.tck.*` system properties.

## Compile

PowerShell:

```powershell
.\build.ps1 `
  -ApiJar C:\path\to\hla-1516e-2025-api.jar `
  -OutputDirectory ..\..\out\java-tck\classes
```

Only the API JAR is needed at compile time.

## Run

At runtime, place the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```powershell
$api = 'C:\path\to\hla-1516e-2025-api.jar'
$provider = 'C:\path\to\provider-rti.jar'
$fom = 'C:\path\to\RestaurantFOMmodule-2025.xml'

.\run.ps1 `
  -ApiJar $api `
  -ProviderJar $provider `
  -FactoryName 'Provider RTI' `
  -FomPath $fom `
  -CapabilityProfile .\profiles\provider.properties `
  -ClassesDirectory ..\..\out\java-tck\classes
```

Use `-DependencyJar` for additional provider dependencies and
`-JvmArgument` for provider-owned JVM flags. The reusable runner does not
interpret those flags.

`run-matrix.ps1` runs the same compiled classes against two or more provider
configurations. `run-jpype.ps1` is an optional handoff check for environments
that use JPype.

Scenario/API/requirement traceability is maintained in
`compliance/catalogs/java-tck-scenario-catalog.json` and validated by
`tools/java_tck.py`.

