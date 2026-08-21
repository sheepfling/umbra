# hla-rti-api

Pure-Python, edition-specific HLA API contracts. This distribution contains no
native implementation. It reserves the same package families as the IEEE Java
bindings:

```text
hla.rti1516e
hla.rti1516_2025
```

The initial 2025 contract covers the implemented connection and callback-control
foundation, including `RtiConfiguration`, `Credentials`, and the four
`RTIambassador.connect` forms. It also includes the bounded federation-execution
create/destroy/list callback slice. Install `umbra-rti-native` to discover Umbra's
pybind11 provider, or `umbra-rti-jpype[jpype]` to adapt a configured Java 2025
RTI through the same contract.
For Umbra's own C++-backed Java route, install the separate
`umbra-rti-jni-python` adapter; it configures the exact Java API/bridge
artifacts and still consumes them through JPype.

Provider authors can combine
`hla.rti1516_2025.testing.ConnectionFoundationConformanceMixin`,
`ConnectionOverloadConformanceMixin`, and
`ProviderBindingParityConformanceMixin` to run the currently supported
contract against their own factory. The parity mixin is intended to be reused
unchanged by direct C++/pybind11 and Java/JPype providers, so both routes are
checked through the same Python value and lifecycle contract.
