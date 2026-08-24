# Federation internals

This domain owns federation-scoped runtime state: joined federates, lifecycle
transitions, the federation registry, the embedded transport seam, and the
coordinator that prepares federation-management work. It is the home for
state that exists because several federates participate in one federation.

## Key files

- federation_registry files hold the central private state and per-federation
  indexes.
- federate_lifecycle files represent member connection, join, resignation, and
  removal transitions.
- federation_management_coordinator files prepare MIM/FOM-backed management
  operations before runtime state is committed.
- embedded_transport files provide the bounded in-process transport seam.

## Working here

- Put federation-owned state and cross-federate routing decisions here.
- Use [handles](../handles/README.md) for stable FOM-derived identifiers and
  [FOM](../fom/README.md) for validated model information.
- Keep logical-time scheduling in [time](../time/README.md), even when a
  federation operation triggers it.
- Keep callback queue mechanics in [callbacks](../callbacks/README.md) and
  diagnostic record encoding in [observability](../observability/README.md).
- Do not expose a private registry type through public headers.

## Tests and references

Start with federate_lifecycle_catch2.cpp, federation_registry_catch2.cpp, and
ieee1516_2025_federation_management_catch2.cpp under
[cpp/tests/](../../../tests/). The governing boundaries and evidence rules are
in [architecture](../../../../docs/architecture/ARCHITECTURE.md) and
[requirements and testing](../../../../docs/testing/REQUIREMENTS-AND-TESTING.md).
