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
