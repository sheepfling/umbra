# Python Java RTI adapter

## Purpose

`umbra-rti-jpype` is an optional provider adapter, not a second Python RTI and
not a replacement for the native Umbra binding. It allows code written against
the public `hla.rti1516_2025` Python contract to use a vendor's Java 2025 RTI
through JPype:

~~~text
Python application
        |
        v
hla.rti1516_2025                  public, provider-neutral contract
        |
        +-------------------------+
        |                         |
        v                         v
umbra._native.rti1516_2025  umbra._java.rti1516_2025
pybind11 / C++              JPype / Java vendor JAR
~~~

Only the public contract is portable. The two provider implementation
namespaces are deliberately private and may contain C++ or Java objects with
their own lifetime rules.

## Choosing a Java provider

Programmatic configuration is preferred because the Java RTI classpath and
JVM options are process-wide:

~~~python
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory

factory = JavaRtiFactory(
    JavaProviderConfiguration(
        classpath=("vendor-rti.jar", "vendor-dependency.jar"),
        rti_factory_name="Vendor RTI",
        jvm_options=("-Xmx1g",),
    )
)
~~~

For a normal one-JAR installation, the concise equivalent is
`JavaRtiFactory.from_jar("vendor-rti.jar", factory_name="Vendor RTI")`.
This still delegates to the exact standard Java `RtiFactoryFactory` and
`ServiceLoader` path; dependency JARs and a native-library path can also be
supplied through the helper.

The equivalent environment configuration is intentionally separate from the
Java-standard default-provider environment variable:

| Setting | Meaning |
| --- | --- |
| `UMBRA_JAVA_RTI_CLASSPATH` | Path-separator-delimited Java RTI JAR classpath. |
| `UMBRA_JAVA_RTI_FACTORY_NAME` | Java vendor `RtiFactory.rtiName()` to select. |
| `UMBRA_JAVA_RTI_JVM_PATH` | Optional path to the JVM shared library. |
| `HLA_RTI_FACTORY_NAME=java` | Select the installed JPype transport through Python factory discovery. |

When discovery selects `java`, set `UMBRA_JAVA_RTI_FACTORY_NAME` for the
vendor-specific Java factory. Keeping those values distinct prevents the
Python alias `java` from being passed accidentally to Java's own
`RtiFactoryFactory`.

JPype runs one JVM per Python process. The adapter starts it lazily when it
first needs the Java factory. If an application has already started the JVM,
it must already have supplied the required Java RTI classpath and options; the
adapter rejects a conflicting late configuration rather than silently using an
unknown classpath.

## Values and migration access

The public API must not become a mixture of Java objects, pybind11 objects,
and Python objects. Each standard value family will have one canonical Python
representation, with Java and C++ adapters converting at their respective
boundaries. That is why the public shared `EncoderFactory`, handles,
logical-time values, and authorization values are not exposed as partial
successes. The Java provider does expose exact handle/logical-time
`EncoderFactory` creators as provider-scoped extension methods: they require
the Java-backed ambassador and return Python shells whose encoding and
validation remain C++-owned. The raw Java escape hatch remains available for
standard Java surfaces outside the Python façade, such as authorization,
when the JNI bridge is selected.

During a controlled migration, a Java-specific caller can use:

~~~python
java_factory = factory.unwrap_java_factory()
java_encoder_factory = factory.unwrap_java_encoder_factory()
java_ambassador = ambassador.unwrap_java_object()
~~~

Those methods are explicitly non-portable. They are a bridge for an existing
Java investment, not an alternative public API. Equivalent pybind11 internals
remain provider-private as well.

## Vendor adapter packages

`umbra-rti-jpype` is the generic bring-your-own-JAR transport. A supported
Java RTI should instead receive a thin, separate adapter distribution. The
repository's `umbra-rti-java-mock` package is a working reference:

~~~text
umbra-rti-java-vendorx
    ├── dependency on umbra-rti-jpype
    ├── entry point: vendorx -> VendorXRtiFactory
    ├── one vendor-specific JAR resolver
    ├── Java RtiFactory name: "VendorX RTI"
    └── vendor integration tests
~~~

The entry-point alias selects the Python transport without JVM startup. The
adapter then supplies the vendor JAR and the Java factory's real
`RtiFactory.rtiName()` to `JavaRtiFactory`. This is why the alias and Java
factory name may differ.

The default policy is external JAR configuration, not bundling. A vendor
adapter can bundle a JAR as package data only when its license permits
redistribution; if it does, it must resolve a physical resource path before
the JVM starts and package any native libraries with their required
`java.library.path` configuration. Do not use a process-global `CLASSPATH` or
put arbitrary vendor JARs in this repository.

## Callback boundary

The adapter turns the full Python `FederateAmbassador` callback surface into a
Java `FederateAmbassador` interface proxy. The 1516.1-2025 surface has 56
method names and 62 overloads, including scalar and timestamped
`initiateFederateSave` forms. It retains the proxy and its Python callback
target for the full connection lifetime; the JNI route audits that each Java
overload has a C++ trampoline and Python marshaller.

## Verification status

The package has fake-runtime tests for Java factory selection, callback-model
conversion, `ConfigurationResult` conversion, callback delivery, every official
exception-name mapping, and explicit raw-object access. It also contains a deliberately
minimal compileable Java fixture at
`packages/umbra-rti-jpype/test-fixtures/mock-java-rti`. Its Java smoke test
proves `ServiceLoader` discovery, state transitions, Java exceptions, and a
queued Java callback. If JPype is installed, the optional integration test
starts a real JVM with that fixture JAR and verifies the full adapter path.

The fixture is not an IEEE API or RTI implementation: it defines only the
small Java surface that the current Python contract calls. A vendor integration
test remains separate because no vendor Java RTI JAR is committed to this
repository.

For an additional end-to-end confidence lane, `packages/umbra-rti-jni` builds
one Umbra C++ ambassador into a JNI library, presents its connection and
federation-reporting slice through a Java `RtiFactory`, and selects that
factory through this same JPype adapter. Its opt-in test proves C++ → JNI →
Java → JPype → Python callbacks (including typed timed-save initiation),
configuration results, callback controls, and typed connection exceptions. It
is deliberately a test façade, not the default Java provider and not a
substitute for a vendor-JAR integration test.
