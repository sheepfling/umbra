# Runtime composition internals

This domain is the private composition root for the standard-facing RTI
implementation. It contains the concrete ambassador that orchestrates the
specialized domains, the reference authorizer bridge, and shared UTF-8
conversion helpers.

## Key files

- umbra_rti_ambassador files implement the private concrete ambassador behind
  the standard API surface and coordinate the other internal domains.
- reference_authorizer.hpp contains the bounded reference authorization
  implementation.
- utf8_string files provide shared conversion and validation helpers for
  private runtime input and report paths.

## Working here

- Add service entry-point orchestration, cross-domain validation, or a narrow
  shared runtime helper here.
- Put detailed state machines back in their owning domain:
  [federation](../federation/README.md),
  [time](../time/README.md),
  [FOM](../fom/README.md),
  [handles](../handles/README.md), or
  [callbacks](../callbacks/README.md).
- Keep public standard binding definitions in cpp/src/. This directory remains
  private even though it implements their behavior.
- Do not turn the ambassador into a second registry, queue, or report store.

## Tests and references

The main integration coverage is in
ieee1516_2025_federation_management_catch2.cpp, with focused authorization and
UTF-8 coverage in ieee1516_2025_authorization_catch2.cpp and
utf8_string_catch2.cpp under [cpp/tests/](../../../tests/). Read
[architecture](../../../../docs/architecture/ARCHITECTURE.md) and
[authorization design](../../../../docs/design/AUTHORIZATION-DESIGN.md) before
changing a public service boundary.
