# umbra-rti-native

## Start here

Use this provider when Python should call Umbra's C++ RTI directly. It is a
source-checkout development binding, not a standalone wheel ABI. Install the
provider-neutral API package first, then read
[CONTRIBUTING.md](../../CONTRIBUTING.md) for the native build profiles and
tests.

## Scope and development boundary

The pybind11 provider for the Umbra C++ RTI. Its CMake project adds Umbra's
repository root and links the exact native target under development. A
platform wheel can be built for CI and installed-package entry-point tests,
but the evolving C++ SDK does not yet promise a standalone redistributable
binary-wheel ABI.

It enables Umbra's source-tree-only embedded federation-management development
profile. The implemented Python slice includes factory creation,
connect/disconnect, callback enablement/evocation, scalar
`createFederationExecution`/`destroyFederationExecution`, and
`listFederationExecutions` with its typed `reportFederationExecutions`
callback. The build fetches Umbra's pinned libxml2 dependency when it is not
already available, so this package remains a development binding rather than a
redistributable binary-wheel profile.

The source-checkout native test suite runs against the built extension and
covers 61 provider tests (with one expected metadata-only discovery skip). It
also audits that every native Python façade call is exported by the loaded
pybind `NativeAmbassador`, preventing wrapper/binding drift.

The package also builds the separate `umbra._native.rti1516e._native_2010`
extension and registers `UmbraNative2010` in
`hla.rti1516e.factories`. It links the exact `rti1516e` C++ target and exposes
the 2010 `connect`/`disconnect`, federation membership, declaration lookup /
publish/subscribe, receive-order object register/discover/update/reflect,
receive-order interaction publish/subscribe/send, object-attribute
ownership query/inform, and synchronization-point registration/achievement
callback slices through
`Native2010RTIambassador`. It delegates the official scalar,
opaque-data, fixed-array, variable-array, fixed-record, and variant-record
encoder families plus both integer64/float64 logical-time representations
through `Native2010EncoderFactory`, `Native2010TimeFactory`, and
`Native2010Float64TimeFactory`. The remaining 2010 RTI/MOM services stay
explicit `NotImplementedError` capability gaps; surface presence and factory
binding do not imply that those services are implemented. The reference object
slice intentionally accepts standard FOM inputs through a deterministic
provider directory while the 2010 FOM loader is still a separate gate.

The direct-native 2010 test consumes the same 79-case basic data-element value
matrix as the JNI/JPype test (`umbra_rti_test_support.data_element_matrix`).
It constructs every value through `Native2010EncoderFactory`, checks the
provider-produced bytes by decoding them again, and compares byte/UTF-16
boundaries without hard-coding a second encoder. This keeps the two Python
routes aligned while leaving wire decisions in the official C++ and Java
carriers.

For a source-checkout surface audit, load the staged extension and run
`tools/verify_1516e_python_surface.py --check-native`; this checks every
contract method is present on `Native2010RTIambassador`. The
`test_native_provider_binds_every_2010_service_and_marks_gaps_explicitly`
pytest also checks the loaded façade's 150 standard method names and confirms
that generated, intentionally unimplemented methods fail with their named
`NotImplementedError` gate. The
`run-2010-tck.ps1` wrapper runs this surface-only gate before its optional
behavioral TCK/profile evidence, so unsupported service behavior cannot be
mistaken for a missing binding.
