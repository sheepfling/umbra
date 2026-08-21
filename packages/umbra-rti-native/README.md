# umbra-rti-native

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
