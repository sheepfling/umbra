# Package map

Umbra separates a provider-neutral Python contract from optional provider
transports. Applications depend on `hla-rti-api` and select one provider; no
provider distribution implements an independent RTI.

| Distribution or artifact | Role | Normal use |
| --- | --- | --- |
| `hla-rti-api` | Pure-Python, edition-specific `hla.*` API contract | Required by every Python provider and application. |
| `umbra-rti-test-support` | Shared provider conformance mixins | Development/test dependency only; never required by applications. |
| `umbra-rti-native` | Direct pybind11 provider for Umbra's C++ RTI | Use when Python should call the native C++ RTI directly. |
| `umbra-rti-jpype` | Java RTI transport, including the `UmbraJniRtiFactory` JNI preset | Use for a Java vendor RTI or Umbra's C++ → JNI → Java route. |
| `umbra-rti-java-mock` | Reference/test adapter for the repository's mock Java RTI | Test and adapter-development only; not a production dependency. |
| `umbra-rti-jni` | Java/JNI bridge artifact | Used by the JNI preset; it is not a Python distribution. |
| `umbra-rti-java-tck` | Provider-neutral Java conformance TCK | Test artifact; it is not a Python distribution. |

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

`UmbraJniRtiFactory` is deliberately part of `umbra-rti-jpype`: it only
configures the IEEE API JAR, Umbra bridge JAR, native library, and named Java
factory. It does not provide RTI services or state of its own.
