# Mock IEEE 1516e Java RTI fixture

This fixture is intentionally tiny and provider-neutral. It registers one
`hla.rti1516e.RtiFactory` through Java `ServiceRegistry`, returns proxy-backed
standard ambassadors, the standard primitive encoder family, an integer
logical-time factory,
and one Java-to-federate callback. It exists only to exercise the TCK's
classloader/factory/Java-surface path. It is not an RTI implementation.

```powershell
.\build.ps1 -ApiJar C:\path\to\IEEE1516-2010-Java-API.jar
```
