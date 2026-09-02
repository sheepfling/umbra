# Java RTI conformance TCK

`packages/hla-rti-java-tck` is a vendor-neutral executable test set for an
IEEE 1516.1-2025 Java RTI provider. Its Java sources import only JDK classes
and the official `hla.rti1516_2025` API. Provider selection happens outside
the test package:

```text
official API JAR + provider JAR(s) + factory name + FOM/MIM inputs
```

The compiled classes can therefore be copied to another product and run with
that product's API/provider JARs. Provider-specific JVM or native-loader flags
are supplied as launcher arguments and are never referenced by the test
sources.

## Scope

The catalog contains 21 scenarios:

- factory discovery, standard encoding, malformed input, and API/exception
  checks;
- complete federation execution create, join, query, resign, destroy, and
  disconnect lifecycle;
- ordinary object-class declaration, object registration/discovery, attribute
  update/reflection, interaction publication/delivery, callback control,
  support lookups, negative edges, relevance advisories, directed
  interactions, and transportation/order controls; and
- capability-profile lanes for DDM, time advance, save/restore, ownership,
  synchronization, and MOM.

All scenarios remain in the evidence output. A provider that does not expose a
required capability records `unsupported` or `not applicable`; the scenario is
not silently removed.

The ordinary scenarios use the IEEE Restaurant example names by default,
including `HLAobjectRoot.Employee.Server`/`Efficiency`,
`HLAobjectRoot.Food.Drink`/`NumberCups`, and
`HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed`.
Directed-interaction and relevance checks default to
`HLAinteractionRoot.ServerAction.TakeOrder`. Supply a compatible FOM or
override names with the `hla.rti.tck.*` system properties.

## Build

PowerShell:

```powershell
.\packages\hla-rti-java-tck\build.ps1 `
  -ApiJar C:\deps\hla-1516e-2025-api.jar `
  -OutputDirectory .\out\java-tck\classes
```

Only the official API JAR is needed at compile time.

## Run

At runtime, put the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```powershell
.\packages\hla-rti-java-tck\run.ps1 `
  -ApiJar C:\deps\hla-1516e-2025-api.jar `
  -ProviderJar C:\deps\provider-rti.jar `
  -FactoryName 'Provider RTI' `
  -FomPath C:\fom\RestaurantFOMmodule-2025.xml `
  -CapabilityProfile C:\fom\provider.properties `
  -ClassesDirectory .\out\java-tck\classes `
  -ResultsPath .\out\java-tck\provider.results.json `
  -JUnitPath .\out\java-tck\provider.junit.xml
```

Use `-DependencyJar` for additional provider JARs and `-JvmArgument` for
provider-owned JVM flags. The reusable runner passes those flags through
without interpreting them.

`run-matrix.ps1` runs the same compiled classes against two or more provider
configurations. `matrix.example.json` shows the portable configuration shape;
each provider supplies its own JAR paths, factory name, FOM, profile, and
optional JVM arguments.

## Traceability

`compliance/catalogs/java-tck-scenario-catalog.json` is the source of truth for
scenario IDs, standard Java API methods, requirements IDs, and contract files.
Validate the source boundary and catalog with:

```powershell
python tools/java_tck.py validate
```

The export command joins provider result artifacts to the catalog:

```powershell
python tools/java_tck.py export `
  --results .\out\java-tck\provider.results.json
```

The result contains stable scenario IDs, provider identity, capability profile,
standard API linkage, JUnit/evidence paths, and explicit portable exclusions.

## Optional JPype handoff

After a direct Java run passes, the same API/provider JARs can be checked
through JPype:

```powershell
.\packages\hla-rti-java-tck\run-jpype.ps1 `
  -DirectResults .\out\java-tck\provider.results.json `
  -ApiJar C:\deps\hla-1516e-2025-api.jar `
  -ProviderJar C:\deps\provider-rti.jar `
  -FactoryName 'Provider RTI'
```

This handoff is an integration check; it does not add provider code to the
portable Java source set.
