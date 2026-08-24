# Time and ordered-delivery internals

This domain owns private logical-time coordination: per-federate time state,
federation bounds, grant policy, scheduled message queues, update-rate gating,
and reference-time selection. It coordinates when bounded work may progress;
it does not define the public logical-time value classes in cpp/src/.

## Key files

- federate_time_state files retain a joined federate's time-related state.
- federation_time_bounds, federation_time_coordinator, and
  federation_time_grant_policy cooperate to calculate and issue bounded
  progress decisions.
- tso_message_queue files retain timestamp-ordered work until it is eligible.
- reference_time_selection chooses the supported reference representation.
- update_rate_gate contains the private update-rate reduction boundary.

## Working here

- Put scheduling, eligibility, and ordered-queue behavior here.
- Treat [federation](../federation/README.md) as a peer domain. Time behavior
  may read federation state, but membership lifecycle remains owned there.
- Keep dispatch mechanics in [callbacks](../callbacks/README.md); a time grant
  can make work eligible without deciding how the callback is invoked.
- Keep public encodings and standard value definitions in cpp/src/.

## Tests and references

The focused tests are federate_time_state_catch2.cpp,
federation_time_bounds_catch2.cpp, federation_time_coordinator_catch2.cpp,
federation_time_grant_policy_catch2.cpp, and tso_message_queue_catch2.cpp
under [cpp/tests/](../../../tests/). Read
[embedded time coordination](../../../../docs/design/EMBEDDED-TIME-COORDINATION-DESIGN.md)
and [logical time](../../../../docs/design/LOGICAL-TIME-DESIGN.md) before
expanding the supported time-management slice.
