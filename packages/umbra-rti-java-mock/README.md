# umbra-rti-java-mock

## Start here

This is a reference/test adapter for authors of vendor-specific Java packages,
not the normal production provider. Install the provider-neutral API and
JPype transport first; then use this package to study named factory
registration, explicit JAR configuration, and integration-test shape. See the
[package map](../README.md) for the intended dependency direction.

## Scope and reference pattern

This is a working reference for a vendor-specific Java RTI adapter package.
It registers the stable Python provider alias `umbra-mock-java`, resolves one
known Java JAR, and selects the Java `RtiFactory` by its actual vendor name.

It deliberately does **not** bundle a JAR. The mock JAR is a repository test
fixture built from `../umbra-rti-jpype/test-fixtures/mock-java-rti`; an actual
vendor adapter should bundle JARs only when redistribution is permitted by the
vendor's license.

For the mock fixture, configure the JAR explicitly:

```python
from umbra._java.mock_rti1516_2025 import MockJavaRtiFactory

factory = MockJavaRtiFactory(r"C:\path\to\umbra-mock-java-rti.jar")
ambassador = factory.getRtiAmbassador()
```

Or use the provider alias plus a vendor-specific path:

```powershell
$env:HLA_RTI_FACTORY_NAME = 'umbra-mock-java'
$env:UMBRA_MOCK_JAVA_RTI_JAR = 'C:\path\to\umbra-mock-java-rti.jar'
```

The pattern to copy for a real vendor is intentionally small:

1. Rename the distribution and private implementation module.
2. Replace the provider alias, JAR environment variable, and Java
   `RtiFactory.rtiName()` constant.
3. Add vendor JAR/natural-library configuration and integration tests.
4. Keep all normal application code on `hla.rti1516_2025`, not this module.
