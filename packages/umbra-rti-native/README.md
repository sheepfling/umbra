# umbra-rti-native

The pybind11 provider for the Umbra C++ RTI. It is currently a source-checkout
build: its CMake project adds Umbra's repository root and links the exact
native target under development. This avoids claiming that the evolving C++
SDK has a standalone binary-wheel ABI yet.

It enables Umbra's source-tree-only embedded federation-management development
profile. The implemented Python slice includes factory creation,
connect/disconnect, callback enablement/evocation, scalar
`createFederationExecution`/`destroyFederationExecution`, and
`listFederationExecutions` with its typed `reportFederationExecutions`
callback. The build fetches Umbra's pinned libxml2 dependency when it is not
already available, so this package remains a development binding rather than a
redistributable binary-wheel profile.
