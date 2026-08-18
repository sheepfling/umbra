# Python bindings design

## Decision

Umbra can support Python without creating a second RTI implementation. The
initial implementation is intentionally split into two Python distributions:

~~~text
hla-rti-api                   (pure Python; standard-shaped contracts)
        ^                              ^
        | Python dependency            | Python dependency
        |                              |
umbra-rti-native              umbra-rti-jpype
(pybind11 provider)           (optional JPype Java-provider adapter)
        |                              |
        | C++ link                      | Java API / vendor JAR
        v                              v
umbra::rti                    hla.rti1516_2025
(official C++ binding)        (official Java binding)
~~~

The distributions live in `packages/` so additional Python packages use the
same layout without turning the C++ root project into an unstructured Python
package. The public API follows the official Java package structure:

~~~text
hla.rti1516e
hla.rti1516e.encoding
hla.rti1516e.exceptions

hla.rti1516_2025
hla.rti1516_2025.auth
hla.rti1516_2025.encoding
hla.rti1516_2025.exceptions
hla.rti1516_2025.time
~~~

`hla-rti-api` owns every public `hla.*` package. Provider distributions own
only an Umbra-specific implementation namespace (currently
`umbra._native.rti1516_2025` for pybind11 and
`umbra._java.rti1516_2025` for JPype) and register an `RtiFactory` through
Python entry points. This mirrors the 2025 Java
`RtiFactoryFactory`/`ServiceLoader` discovery model without exposing native or
Java objects as public API.

The `java` entry-point alias selects the JPype transport before it has to start
a JVM. It is intentionally distinct from `JavaRtiFactory.rtiName()`, which
returns the selected vendor Java factory's real standard name. This avoids
loading every Java RTI during unrelated Python provider lookup.

## First capability slice

The 2025 API contract is deliberately limited to the completed native
connection foundation plus one bounded federation-execution discovery slice:

- create an ambassador;
- connect and disconnect it, including the four Java/C++ connection forms
  (base, `RtiConfiguration`, credentials, and both);
- choose immediate or evoked callback delivery;
- enable/disable callbacks; and
- evoke one or multiple callbacks.

The federation-execution slice adds the scalar
`createFederationExecution`, `destroyFederationExecution`, and
`listFederationExecutions` services with the typed
`reportFederationExecutions` callback. It does not imply that joining,
membership, FOM-management variants, or the rest of federation management are
ready for Python.

`ConfigurationResult`, `RtiConfiguration`, `auth.Credentials`,
`auth.HLAnoCredentials`, callback-model values, and the small connection error
family preserve their Java API spelling and package location. Umbra's current
native slice accepts the no-credentials marker; credential mechanisms beyond
that remain provider capabilities rather than a fabricated shared feature. The native provider derives an
internal C++ bridge from the official `NullFederateAmbassador`, holds the
Python callback object for the connection lifetime, and overrides only the
currently exposed `connectionLost` callback. Future callback overrides are
added alongside their native service and API contract—never pre-declared as
successful Python behavior.

This makes the pure package useful for application typing, fake providers, and
unit tests while ensuring the native package delegates all RTI state and
semantics to C++. It does not copy the legacy Python RTI model and does not
make a standards/conformance claim.

## Extension rules

1. Keep `hla-rti-api` free of `pybind11`, CMake, and compiled dependencies.
2. Add capability families at their official edition-specific locations, each
   only after the C++ vertical slice has a
   documented native boundary and tests.
3. Add matching provider support in `umbra-rti-native`; a provider may support
   either edition independently, and no cross-edition compatibility façade is
   implied.
4. Translate every official 2025 C++/Java exception simple name at the provider
   edge, even when the associated service has not yet been bound. Do not leak C++
   objects, pointers, containers, or lifetime obligations into the pure API.
   Apply the same rule to Java objects: convert to the shared Python value
   type, or leave the capability unavailable until such a type exists.
   Encoding follows this rule especially strictly: Python `bytes` is the
   portable payload, while C++ `VariableLengthData` and Java `ByteWrapper`
   remain provider-private implementation details.
5. Keep C++ callback trampolines narrow. They must hold Python references for
   the connection lifetime, acquire the GIL before calling Python, and turn a
   Python callback failure into the declared C++ callback error. Java callback
   proxies follow the same lifetime rule and retain both the proxy and its
   Python target for the connection.
6. The package version pair is locked exactly during pre-1.0 development.
   Once a compatibility policy exists, the native provider will declare a
   compatible API range instead.

## Build and packaging boundary

`umbra-rti-native` uses `scikit-build-core` and `pybind11`. During this spike
its CMake project adds Umbra's source root and links `umbra::rti`, which proves
the binding path against the exact C++ checkout. It is *not* yet a standalone
wheel ABI: the C++ SDK is static, and the Python build deliberately enables
the embedded federation-management development profile. That profile depends
on Umbra's private libxml2 FOM validator, so it is not a redistributable
binary-wheel ABI; the overall RTI remains under active development.

The graduation gate for independently buildable wheels is a packaged C++ SDK
contract (headers, library linkage, runtime dependencies, ABI/version policy,
and platform CI). At that point the native package will switch from
`add_subdirectory` to `find_package(umbra_rti CONFIG REQUIRED)` or consume a
dedicated native-SDK artifact. The Python public API stays unchanged.

The complete Java-factory inventory and its native implementation gates are in
[Python factory inventory](PYTHON-FACTORY-INVENTORY.md). The provider factory
is implemented now; the remaining factory families are added only with their
native value and lifecycle prerequisites.

The optional Java adapter's configuration, lifecycle, and raw-object migration
boundary are described in [Python Java adapter](PYTHON-JAVA-ADAPTER.md).
The binding-first encoding mapping and its native implementation gate are in
[Python encoding binding design](PYTHON-ENCODING-BINDING-DESIGN.md).

## Developer validation

The pure contract has no third-party runtime dependency:

~~~powershell
$env:PYTHONPATH = 'packages/umbra-rti-api/src'
python -m unittest discover -s packages/umbra-rti-api/tests
~~~

Build the native wheel from a checkout after installing its build requirements
(`scikit-build-core` and `pybind11`):

~~~powershell
python -m pip wheel ./packages/umbra-rti-native --no-deps
~~~

Run the native test with both source trees on `PYTHONPATH` and the installed
wheel available. The first test asserts only the native foundation and must
not be widened into a claim that Create/Join or other development-only C++
services are Python-ready.

The reusable provider conformance suite and its mapping to the C++ connection
tests are in [Python conformance testing](PYTHON-CONFORMANCE-TESTING.md).
