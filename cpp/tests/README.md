# Native test guide

This directory contains Umbra's native C++ tests. A test name identifies the
runtime domain or standard-binding surface it exercises; the file stays beside
similar focused tests instead of being grouped by framework.

## Test layers

| Test kind | Purpose | Typical file name |
| --- | --- | --- |
| Smoke test | Compiles or exercises a narrow installed/baseline surface without Catch2. | ieee1516_2025_headers_smoke.cpp |
| Focused Catch2 test | Tests one private domain or bounded service slice. | federation_registry_catch2.cpp |
| Integration Catch2 test | Tests a standard-facing workflow across internal domains. | ieee1516_2025_federation_management_catch2.cpp |
| Contract/traceability test | CMake invokes a tool to confirm source and requirement mappings. | Registered in the root CMake file. |

The default build profile runs smoke, standards-integrity, traceability, and
package tests. Catch2 tests are available through the native-catch2 or
native-fom profiles described in [CONTRIBUTING.md](../../CONTRIBUTING.md).

## Add or change a test

1. Start with the README for the owning internal domain.
2. Add a focused area_catch2.cpp test when a behavior needs Catch2.
3. Add the source to the root CMake test target near similar tests.
4. Use a descriptive fixture from [data/](data/README.md) only when the model
   input is part of the behavior.
5. Add or update a Requirements Lab contract only for an official,
   source-traceable behavior.

Run the narrowest test while iterating, then run the baseline:

    ctest --test-dir out/cmake/catch2 -C Debug -R federation_registry --output-on-failure
    python tools/ci.py native

## Fixture and evidence boundaries

The [data/](data/README.md) directory contains small Umbra-owned XML fixtures.
It does not hold unreviewed external corpora. Requirements Lab contracts and
evidence inputs live under [compliance/](../../compliance/README.md), while
generated local evidence belongs under .compliance/ or out/.
