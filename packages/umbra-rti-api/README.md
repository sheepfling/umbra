# hla-rti-api

## Start here

Use this package for the provider-neutral Python API contract. Install it
before any Umbra Python provider:

    python -m pip install -e packages/umbra-rti-api

Verify the contract from a checkout with Python 3.11 or newer:

    python -m unittest discover -s packages/umbra-rti-api/tests

For provider selection and editable-install order, see the
[package map](../README.md).

## Scope and API boundary

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
For Umbra's own C++-backed Java route, use `UmbraJniRtiFactory` from
`umbra-rti-jpype`; it configures the exact Java API/bridge artifacts and still
consumes them through JPype.

Provider authors can combine
`umbra_rti_test_support.ConnectionFoundationConformanceMixin`,
`ConnectionOverloadConformanceMixin`, and
`ProviderBindingParityConformanceMixin` to run the currently supported
contract against their own factory. The parity mixin is intended to be reused
unchanged by direct C++/pybind11 and Java/JPype providers, so both routes are
checked through the same Python value and lifecycle contract.
The mixins live in the separate `umbra-rti-test-support` development package;
they are deliberately not part of this runtime API distribution.

The implementation is split into abstract contracts (`abstract.py`), concrete
cross-provider values (`values.py`), and thin package export shims. Read-only
binary inputs use `BytesLike` and are copied to immutable `bytes` at provider
boundaries. See
[`PYTHON-RTI-CONTRACT-MAPPING.md`](../../docs/python/PYTHON-RTI-CONTRACT-MAPPING.md)
for the Java/C++ method mapping.
