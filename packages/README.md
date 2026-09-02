# Package map

Umbra separates a provider-neutral Python contract from optional provider
transports. Applications depend on `hla-rti-api` and select one provider; no
provider distribution implements an independent RTI.

| Distribution or artifact | Role | Normal use |
| --- | --- | --- |
| `hla-rti-api` | Pure-Python, edition-specific `hla.*` API contract | Required by every Python provider and application. |
| `umbra-rti-test-support` | Shared provider conformance mixins | Development/test dependency only; never required by applications. |
| `umbra-rti-native` | Direct pybind11 providers for Umbra's edition-specific C++ RTIs | Use `Umbra` for 2025 or `UmbraNative2010` for the surface-complete/bindable `hla.rti1516e` native route, including bounded reference federation/declaration/object/interaction/ownership/synchronization proof slices. Remaining services are explicit capability gates. |
| `umbra-rti-jpype` | Java RTI transport, including the `UmbraJniRtiFactory` JNI preset | Use for a Java vendor RTI or Umbra's C++ → JNI → Java route. |
| `umbra-rti-java-mock` | Reference/test adapter for the repository's mock Java RTI | Test and adapter-development only; not a production dependency. |
| `umbra-rti-jni` | Java/JNI bridge artifact | Used by the JNI preset; it is not a Python distribution. |
| `umbra-rti-jni-2010` | IEEE 1516e-2010 Java/JNI bounded null bridge | Java binding bring-up only; connect/disconnect state is exercised, remaining RTI services throw `RTIinternalError`. |
| `hla-rti-java-tck` | Vendor-neutral IEEE 1516.1-2025 Java conformance TCK | Compile against the official API JAR; attach any compliant provider JAR at runtime. |
| `umbra-rti-java-tck` | Umbra integration wrapper and capability profile for `hla-rti-java-tck` | Compatibility entry point for the Umbra JNI product; the reusable tests live in `hla-rti-java-tck`. |
| `hla-rti-cpp-tck` | Vendor-neutral IEEE 1516.1-2025 C++ conformance TCK | Compile against the official API headers; supply provider libraries and adapter settings at configure time. |
| `umbra-rti-java-tck-2010` | Provider-neutral IEEE 1516.1-2010/1516e Java TCK and proxy fixture | Test artifact; it is not a Python distribution. |

## Java/JNI product bundles

The JNI bridge JARs keep the official IEEE API JAR as the authoritative
surface and do not shadow its classes. The bridge registers the standard
top-level providers through `ServiceLoader`: 2025 registers `RtiFactory`,
`AuthorizerFactory`, and `time.LogicalTimeFactory`; 2010 registers `RtiFactory`
and `LogicalTimeFactory`. The registered providers expose the bounded Umbra
surface; this is not a claim of complete RTI conformance.

Build a directly consumable product directory for either edition with the
Python builder (the API JAR must be supplied by the release owner):

```text
python build_umbra_jni.py --edition 2025 \
  --java-api-jar C:\path\to\ieee-1516.1-2025-java-api.jar \
  --run-smoke-test
```

Use `--edition 2010` with the 1516e API JAR for the older surface. The output
contains the provider JAR, native library, copied API JAR, dependency and
verification manifests, and the portable Python launcher `run.py`. Run
`python run.py` for the standard surface check; 2025 also accepts
`--mode native`, while 2010 accepts `--mode native` and `--mode types`.

The product is a standard-shaped directory, not a shaded/fat JAR: the
official API JAR remains authoritative and the bridge registers the declared
top-level factories through `ServiceLoader`.

## Develop from a checkout

All Python packages require Python 3.11 or newer. Start by installing the
provider-neutral API package in an isolated environment:

    python -m pip install -e packages/umbra-rti-api

Then install exactly one provider while working on it:

    python -m pip install -e packages/umbra-rti-jpype[jpype]

Provider conformance suites additionally use the development-only shared test
package:

    python -m pip install -e packages/umbra-rti-test-support

The direct native provider has an additional CMake/pybind11 build boundary;
see its README before installing it. The Java mock is a test/reference package,
and the JNI bridge and Java TCK are Java artifacts rather than Python
distributions.

Run Python tests with the package source roots on PYTHONPATH, or use the
commands in [Python conformance testing](../docs/python/PYTHON-CONFORMANCE-TESTING.md).
Use [CONTRIBUTING.md](../CONTRIBUTING.md) for the native baseline and common
review workflow.

## Python provider selection

```text
Python application
        |
        v
   hla-rti-api
    /         \\
   v           v
native       Java transport
pybind11     umbra-rti-jpype
   |          /             \\
   v         v               v
Umbra C++  vendor Java RTI  UmbraJniRtiFactory -> JNI -> Umbra C++
```

`umbra-rti-jpype` also exposes `Java2010RtiFactory` for a caller-supplied
standard 2010 Java RTI JAR. It selects `hla.rti1516e.RtiFactoryFactory` (with
the documented modern-JDK compatibility fallback) and never aliases the 2025
objects. `UmbraJniRtiFactory` is deliberately part of `umbra-rti-jpype`: it only
configures the IEEE API JAR, Umbra bridge JAR, native library, and named Java
factory. It does not provide RTI services or state of its own.
The separate `umbra-rti-jni-2010` artifact follows the same dependency rule
for `hla.rti1516e`, but is intentionally a null provider; it proves only the
JNI factory and connection boundary.
The transplantable Python smoke is `umbra-rti-java-tck-2010/jpype_smoke.py`;
it emits 34 catalog-linked scenarios covering factory discovery, provider-owned
scalar/complex encoding and lifecycle/time probes, generated API and callback
inventories, FOM/MIM-gated federation/declaration/object/interaction/DDM/
ownership/save-restore behavior, and the implemented standard MOM slices when a
real JPype runtime and provider JAR are supplied.
Pass `--native-provider` (or use
`packages/umbra-rti-native/run-2010-tck.ps1`) to run the same catalog against
`UmbraNative2010`; the direct route currently proves the common surface,
overloads/exceptions, scalar/opaque and composite encoders, integer/float
logical-time arithmetic, connection lifecycle, federation membership,
declaration management, receive-order object and interaction callback paths,
ownership queries, synchronization callbacks, installed-style factory
discovery, and explicit logical-time factory discovery while reporting its
remaining RTI/MOM service families as explicitly unsupported.
