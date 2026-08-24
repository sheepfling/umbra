# Python bindings design

## Decision

Umbra can support Python without creating a second RTI implementation. The
public contract and provider transports are intentionally split into small
Python distributions:

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

UmbraJniRtiFactory (named preset inside umbra-rti-jpype)
        |
        | API JAR + bridge JAR + native library
        v
umbra-rti-jpype -> Java ServiceLoader -> umbra::rti
~~~

Umbra's Java-provider route crosses both native boundaries while retaining
C++ as the only RTI semantics:

~~~text
umbra::rti  ->  JNI  ->  standard-shaped Java RTI façade  ->  JPype  ->  Python
~~~

`packages/umbra-rti-jni` owns Umbra's staged Java façade. It is not a third
RTI implementation: pybind is the direct Umbra Python provider, and JPype
selects either this Umbra Java façade or an independent vendor JAR. The
`UmbraJniRtiFactory` is a named configuration preset in
`packages/umbra-rti-jpype`; it supplies no service methods or Java shadow
state.
The façade must never reimplement RTI semantics in Java or inherit them from a
mock. It exposes a C++-bound service or an explicit
`RTIinternalError` until that service is bound and tested. The ordinary JPype
path must continue to accept arbitrary vendor Java RTI JARs independently.

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
`umbra._java.rti1516_2025` for JPype, including `UmbraJniRtiFactory`) and
register an `RtiFactory` through Python entry points. This mirrors the 2025 Java
`RtiFactoryFactory`/`ServiceLoader` discovery model without exposing native or
Java objects as public API.

Within `hla-rti-api`, `abstract.py` owns provider-facing abstract contracts and
`values.py` owns concrete cross-provider value objects. `core.py` is retained as
a compatibility façade for existing imports. The complete Java/C++ surface
mapping and binary-boundary rules are maintained in
[Python RTI contract mapping](PYTHON-RTI-CONTRACT-MAPPING.md).

The `java` entry-point alias selects the JPype transport before it has to start
a JVM. It is intentionally distinct from `JavaRtiFactory.rtiName()`, which
returns the selected vendor Java factory's real standard name. This avoids
loading every Java RTI during unrelated Python provider lookup.

Both provider routes are held to the same Python parity gate. The native lane
calls the C++ implementation through pybind11; the Java lane calls the exact
2025 Java interfaces through JPype and, in the external-API lane, verifies
assignability to `hla.rti1516_2025.RTIambassador` from the independently
obtained IEEE API JAR. The gate is a bridge check, not a second Java RTI
implementation.

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
`reportFederationExecutions` callback. It also adds
`listFederationExecutionMembers` with typed
`reportFederationExecutionMembers` and
`reportFederationExecutionDoesNotExist` callbacks. It does not imply that
member-management, FOM-management variants, or the rest of
federation management are ready for Python.

The scalar join/resign pair is also available: `joinFederationExecution`
supports the two Java overloads without additional FOM modules, and
`resignFederationExecution` accepts the standard `ResignAction` values. Its
returned `FederateHandle` is an immutable copy of the standard encoded handle
bytes, never a C++ pointer or Java proxy. Additional-FOM join overloads remain
unbound until their sequence boundary is designed and tested.

`ConfigurationResult`, `RtiConfiguration`, `auth.Credentials`,
`auth.HLAnoCredentials`, callback-model values, and the small connection error
family preserve their Java API spelling and package location. Umbra's current
native slice accepts the no-credentials marker; credential mechanisms beyond
that remain provider capabilities rather than a fabricated shared feature. The native provider derives an
internal C++ bridge from the official `NullFederateAmbassador`, holds the
Python callback object for the connection lifetime, and overrides only the
currently exposed `connectionLost`, federation-execution, and
federation-member callback family. Future callback overrides are added
alongside their native service and API contract—never pre-declared as
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

The staged service and callback coverage strategy is maintained in
[Python API coverage plan](PYTHON-API-COVERAGE-PLAN.md). It records only
end-to-end bound capability as coverage and establishes the next vertical
slice, so public Python declarations cannot run ahead of the native and Java
provider implementations.

The optional Java adapter's configuration, lifecycle, and raw-object migration
boundary are described in [Python Java adapter](PYTHON-JAVA-ADAPTER.md).
The staged native-backed Umbra Java façade is described in
[`packages/umbra-rti-jni`](../../packages/umbra-rti-jni/README.md).
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

The JNI lane is opt-in because it builds the C++ embedded development profile
and starts a separate JVM process:

~~~powershell
$env:UMBRA_ENABLE_JNI_INTEGRATION_TESTS = '1'
$env:UMBRA_JNI_REQUIRE_RUNTIME_SERVICE_COVERAGE = '1'
python -m unittest packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py
~~~

The reusable provider conformance suite and its mapping to the C++ connection
tests are in [Python conformance testing](PYTHON-CONFORMANCE-TESTING.md).
