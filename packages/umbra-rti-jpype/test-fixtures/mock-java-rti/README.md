# Mock Java 2025 RTI fixture

This is a **test fixture**, not an IEEE 1516.1-2025 implementation and not a
vendor RTI. It contains only the Java API surface consumed by the current
Python JPype adapter:

- `RtiFactoryFactory` discovered through Java `ServiceLoader`;
- `RtiFactory` and `RTIambassador` creation;
- all four connect forms, disconnect, callback enablement/evocation, and the
  scalar federation create/destroy/list service path;
- `RtiConfiguration`, `auth.Credentials`, `auth.HLAnoCredentials`,
  `ConfigurationResult`, callback-model, and basic exception values; and
- queued `connectionLost` and `reportFederationExecutions` callbacks.

The intentionally limited surface makes it possible to verify a real JVM,
classloader, `ServiceLoader`, JPype proxy, Java exception, and Java-to-Python
callback path without adding a proprietary Java RTI JAR to the repository.

Build and run the Java-only smoke test on Windows:

```powershell
.\build.ps1 -RunSmokeTest
```

The generated JAR is placed below `build/`, which is ignored by Git. The
Python test suite builds the same fixture in a temporary directory. If JPype
is installed, it also executes the adapter against this JAR.
