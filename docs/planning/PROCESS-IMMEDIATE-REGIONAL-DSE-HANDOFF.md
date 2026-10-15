# 2025 process regional DSE: HLA_IMMEDIATE callback-entry handoff

Status: **open; strict regression red; no runtime fix or validation claim**. Search
handle: `process-delayed-regional-attribute-immediate`. Keep this separate from
the green `process-delayed-regional-attribute` HLA_EVOKED receive-fence lane.

## Contract and observed failure

The pinned 1516.1-2025 Lab candidates at `08-time-management-page-187.tex`
lines 25–30, 40–45, and 46–51 state, respectively, that an enabled Delay
Subscription Evaluation switch postpones subscription evaluation until message
delivery eligibility; that a disabled switch cannot revive a message excluded
at generation; and that *both* modes use the joined federate's current
subscription when delivery becomes eligible. Map these candidates to the
canonical **§8.1.8**, not the bundle's stale `8.1.10` metadata (RL-018).
The regional attribute path also draws on §§6.10, 6.11, 9.5, 9.6, and 9.8;
the existing evoked plan row has the exact direct requirement pairs.

A strict, uncommitted official-API C++ case is preserved in
`cpp/tests/ieee1516_2025_connection_delayed_regional_attribute_update_catch2.cpp`
in the isolated `2025-object-ddm` worktree. It uses
`ProcessFederationServiceOptions{true}` and HLA_IMMEDIATE; disabling then
enabling callbacks creates the callback-entry fence. MinGW built
`umbra_process_delayed_regional_attribute_catch2`. The focused CTest
`Process immediate delayed regional attribute delivery rechecks at callback entry`
failed reproducibly:

- Enabled: a known receiver unsubscribed at update generation and subscribed
  before callback entry with a range overlapping the original sent region
  received **0** reflections, expected **1** (`:259`).
- Disabled: a passel initially eligible under an overlapping declaration was
  still reflected after the receiver committed a disjoint range before
  callback entry: **1**, expected **0** (`:284`).

The existing HLA_EVOKED test remained green 1/1 after the fixture extension.
Do not check the red case into the default CTest suite or mark §8.1.8
validated until a correct callback-entry mechanism passes it.

## Why the current push path cannot satisfy this boundary

`process_federation_service_attribute_value_updates.cpp` materializes the
recipient and attribute subset during `UpdateAttributeValues` when
`pushReceiveOrderEvents` is true, then sends the complete event frame. Its
server-only `recheckAtReceive` flag and original sent-region snapshots are
deliberately not encoded by the process codec. The client buffers the frame;
`process_federation_client_callback_dispatch.cpp` submits it to
`process_federation_callback_bridge.cpp`, whose immediate task invokes the
FederateAmbassador without asking the authoritative service for current
subscription or region state. Once serialized, the client cannot recover a
recipient omitted at generation or suppress a now-disjoint passel correctly.

The next slice needs a **server-owned callback-entry decision**. Two candidate
protocols need design review, not an in-place predicate tweak:

1. Retain each accepted passel and its immutable producer-region snapshots in
   the service, push an ordered notification, and materialize via the existing
   receive operation only when the immediate callback task is admitted.
2. Retain the same service-owned state behind an event identity and make a
   one-shot callback-entry recheck request that returns the filtered event.

Both must define cancellation/lifetime, one-shot consumption, FIFO with other
pushed events, and the boundary against concurrent subscription changes. The
client must not become the authority for region snapshots. Likely touchpoints:
`process_federation_service_attribute_value_updates.cpp`,
`process_federation_service_receive.cpp`,
`process_federation_service_protocol.hpp` and codecs,
`process_federation_client.cpp`,
`process_federation_client_callback_dispatch.cpp`, and
`process_federation_callback_bridge.cpp`/`.hpp`. Confirm request/reply
re-entry and lock ordering before making an immediate callback task perform
network I/O. Keep timestamped/TSO and 2010 behavior outside this first slice.

## Acceptance for a dedicated implementation lane

Promote the strict case only with a separate plan row and direct
requirement-to-canonical-2025 mappings. Under HLA_IMMEDIATE process push,
prove enabled late overlap (one reflection with original sent region/tag),
late disjoint suppression, disabled generation-time exclusion, and current
subscription/range suppression in both modes. Include source-region mutation
after send, multiple queued passels/FIFO, resign or object-delete cancellation,
and a concurrent subscription/callback race test if the chosen protocol permits
it. Re-run the neighboring HLA_EVOKED receive-fence case and ordinary regional
push controls, focused MinGW CTest, and the lane mapping check. Do not claim
full §8.1.8 or general TSO parity from this bounded receive-order slice.
