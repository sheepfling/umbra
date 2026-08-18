# Python provider conformance testing

## Purpose

The Python binding has a small public surface today, so it should test that
surface as a provider contract rather than repeat provider-specific examples.
`hla.rti1516_2025.testing.ConnectionFoundationConformanceMixin` and
`ConnectionOverloadConformanceMixin` are reusable `unittest` mixins. A
provider supplies `make_factory()`; together they verify the observable RTI
connection state machine and all four currently supported `connect` forms
through the public Python API.

This is intentionally a binding conformance suite, not an assertion that any
provider implements the full IEEE standard.

## C++ test mapping

The initial contract mirrors the available portions of
`cpp/tests/ieee1516_2025_connection_catch2.cpp`.

| C++ assertion family | Python conformance assertion | Status |
| --- | --- | --- |
| Base `connect` with immediate and evoked models | Both `CallbackModel` values connect and yield a well-formed `ConfigurationResult` | Implemented |
| Duplicate `connect` | Raises `AlreadyConnected` at the Python provider edge | Implemented |
| Unsupported callback enum | A non-`CallbackModel` value raises `UnsupportedCallbackModel`, then the ambassador remains connectable | Implemented |
| `disconnect` before connection | Raises `NotConnected` | Implemented |
| Empty-queue callback controls | `evokeCallback`/`evokeMultipleCallbacks` return false around enable/disable transitions | Implemented |
| Disconnect then reconnect | A fresh connection succeeds after disconnect | Implemented |
| C++ configuration/credential overloads | Base, `RtiConfiguration`, `HLAnoCredentials`, and both together each connect and yield a well-formed `ConfigurationResult` | Implemented |
| C++ `RtiConfiguration` value semantics | Java-shaped Python builder retains name, address, and additional-settings values | Implemented |
| `listFederationExecutions` / `reportFederationExecutions` | Both callback models deliver a typed `FederationExecutionInformationSet`; unconnected use raises `NotConnected` | Implemented |
| Scalar federation create/destroy with a FOM module | Native provider creates a real federation, lists its typed record, then destroys it; Java adapters perform the same method path against their configured RTI | Implemented |
| C++ `VariableLengthData` value semantics | Provider-private conversion to the future portable Python `bytes` boundary; do not expose C++ pointer ownership | Deferred |

All 100 concrete exception names in the 2025 Java exception package are
available as Python subclasses of `RTIexception`. Provider boundaries map a
known C++ or Java simple name to its specific Python type; individual services
are still added and tested only when bound.

Federation management, logical time, handles, encoding, and the remaining
callback families receive their own corresponding conformance mixins only once
their public Python value types and provider bindings exist.

## Current provider runs

The native provider combines the mixin with its pybind11 wheel test:

~~~text
NativeConnectionFoundationConformanceTest
    -> UmbraRtiFactory
    -> pybind11
    -> Umbra C++ RTI
~~~

The mock Java vendor adapter combines the same mixin with a real JVM fixture:

~~~text
MockVendorJPypeIntegrationTest
    -> MockJavaRtiFactory
    -> JPype
    -> Java ServiceLoader / MockRtiFactory
~~~

The Java fixture additionally queues a `connectionLost` event so its test
proves an actual Java-to-Python callback, not merely method invocation. Both
provider runs include the overload mixin, so they exercise the C++ and Java
connection dispatches rather than only their Python method signatures.

## Adding a provider

An adapter package adds a short test class:

~~~python
import unittest

from hla.rti1516_2025.testing import (
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
)


class VendorConnectionConformance(
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    unittest.TestCase,
):
    def make_factory(self):
        return VendorRtiFactory(configured_jar)
~~~

The provider's normal integration test configures any native dependencies,
JARs, or broker endpoint before the mixin runs. This keeps the behavioral
contract the same while preserving provider-specific setup.

## Running the current suites

The pure API and Java adapter unit suites require no Java vendor JAR:

~~~powershell
$env:PYTHONPATH = 'packages/umbra-rti-api/src;packages/umbra-rti-jpype/src'
python -m unittest discover -s packages/umbra-rti-api/tests
python -m unittest discover -s packages/umbra-rti-jpype/tests
~~~

The native conformance suite runs against an installed `umbra-rti-native`
wheel plus the API source. The mock vendor's real-JVM suite additionally needs
`JPype1`, a JDK, and its generated fixture JAR; its test builds that fixture in
a temporary directory automatically. These setup requirements are deliberate:
they ensure the test exercises the same packaging and bridge boundaries that
an application uses.
