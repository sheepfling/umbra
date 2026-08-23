# Umbra RTI test support

This development-only package contains the provider-neutral Python conformance
mixins used by the native, JPype, and mock-Java test suites. It is not a
runtime dependency of applications using `hla-rti-api`.

Install it from a checkout when running provider conformance tests:

    python -m pip install -e packages/umbra-rti-api
    python -m pip install -e packages/umbra-rti-test-support
