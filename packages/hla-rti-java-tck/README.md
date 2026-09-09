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

## Build

From the repository root, use the shell-free Python runner:

```text
python tools/run_java_tck.py build \
  --api-jar C:/path/to/hla-1516e-2025-api.jar \
  --output-directory out/java-tck/classes
```

Only the API JAR is needed at compile time.

## Run

At runtime, place the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```text
python tools/run_java_tck.py run \
  --api-jar C:/path/to/hla-1516e-2025-api.jar \
  --provider-jar C:/path/to/provider-rti.jar \
  --factory-name "Provider RTI" \
  --fom-path C:/path/to/RestaurantFOMmodule-2025.xml \
  --capability-profile packages/hla-rti-java-tck/profiles/provider.properties \
  --classes-directory out/java-tck/classes \
  --results-path out/java-tck/provider.results.json \
  --junit-path out/java-tck/provider.junit.xml
```

Use `--dependency-jar` for additional provider dependencies and
`--jvm-argument` for provider-owned JVM flags. The reusable runner does not
interpret those flags.

`matrix` runs the same compiled classes against two or more provider
configurations. `matrix.example.json` shows the portable configuration shape:

```text
python tools/run_java_tck.py matrix \
  --configuration packages/hla-rti-java-tck/matrix.example.json \
  --classes-directory out/java-tck/classes \
  --output-directory out/java-tck/matrix
```

The optional JPype handoff is available directly as
`python packages/hla-rti-java-tck/jpype_smoke.py`.

Scenario/API/requirement traceability is maintained in
`compliance/catalogs/java-tck-scenario-catalog.json` and validated by
`tools/java_tck.py`.
