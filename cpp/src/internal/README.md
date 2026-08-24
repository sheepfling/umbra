# Private runtime map

This directory contains Umbra's implementation-only runtime code. Nothing
below this directory is part of the public IEEE API or Umbra's public helper
headers. Public standard-binding definitions live one level up in src/.

Start with the [native C++ guide](../../README.md) for the full source-tree
map, then choose a domain below.

## Domains

| Directory | Owns | Start with |
| --- | --- | --- |
| [handles/](handles/README.md) | Private handle values, directories, and handle encodings | a handle type or directory implementation |
| [fom/](fom/README.md) | FOM/FDD validation, composition, and the retained catalog | a model-input or schema question |
| [federation/](federation/README.md) | Joined-member lifecycle, federation registry, embedded transport, and management coordination | federation-owned runtime state |
| [time/](time/README.md) | Time state, bounds, scheduling, queues, and reference-time selection | a logical-time or ordered-delivery question |
| [callbacks/](callbacks/README.md) | Callback dispatch and callback-session bookkeeping | immediate versus evoked callback delivery |
| [observability/](observability/README.md) | Instrumentation, service reports, and MOM report encoding | diagnostics or durable service evidence |
| [runtime/](runtime/README.md) | Ambassador orchestration, authorization bridge, and shared text helpers | a standard RTI entry point or cross-domain orchestration |

## Dependency direction

Keep dependencies intentional:

- The standard-binding source files in src/ call into this directory.
- runtime/ is the composition root. It may coordinate the other domains but
  should not become a home for their detailed state machines.
- federation/ and time/ are peer domains; they cooperate for membership-aware
  time progression and ordered delivery.
- federation/ uses FOM and handle data to materialize a federation.
- callbacks/ schedules callback work and uses observability; it does not own
  federation or service semantics.
- observability/ records facts supplied by the runtime; it must not decide
  whether a service is allowed.

When a change would introduce a surprising dependency, prefer a narrow value,
callback, or query interface rather than reaching into another domain's
private state.

## Adding a file

1. Choose the domain that owns the behavior, not the caller that first needs
   it.
2. Place a private header next to its implementation when both are needed.
3. Include it with the full internal path, for example
   internal/domain/file.
4. Add the implementation source to the root CMake source list and add a
   focused test under cpp/tests/.
5. Update Requirements Lab source references when an implementation file is
   added or moved; those contracts deliberately verify source locations.

For test authority, evidence promotion, and focused test lanes, see
[requirements and testing](../../../docs/testing/REQUIREMENTS-AND-TESTING.md).
