# 2025 process regional DSE: HLA_IMMEDIATE callback-entry handoff

Status: **bounded implementation and focused MinGW evidence complete; broader
conformance open**. Search handle: `process-delayed-regional-attribute-immediate`.
Keep this separate from the `process-delayed-regional-attribute` HLA_EVOKED
receive-fence lane.

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

A strict official-API C++ case in
`cpp/tests/ieee1516_2025_connection_delayed_regional_attribute_update_catch2.cpp`
uses
`ProcessFederationServiceOptions{true}` and HLA_IMMEDIATE; disabling then
enabling callbacks creates the callback-entry fence. MinGW built
`umbra_process_delayed_regional_attribute_catch2`. Before the fix, the focused CTest
`Process immediate delayed regional attribute delivery rechecks at callback entry`
failed reproducibly:

- Enabled: a known receiver unsubscribed at update generation and subscribed
  before callback entry with a range overlapping the original sent region
  received **0** reflections, expected **1** (`:259`).
- Disabled: a passel initially eligible under an overlapping declaration was
  still reflected after the receiver committed a disjoint range before
  callback entry: **1**, expected **0** (`:284`).

After the callback-entry claim implementation, this case passes all 47
assertions. The focused target passes **4/4** CTests (immediate, HLA_EVOKED,
ordinary overlapping push, and disjoint regional push). Its exact plan row
has 10 direct requirement-to-canonical-section pairs; the focused lane check
passes with no source-unlocated tests. This is private process-profile
evidence, not validation of all §8.1.8.

## Why the old push path failed, and the bounded implementation

The old `process_federation_service_attribute_value_updates.cpp` push path
materialized a recipient and attribute subset during `UpdateAttributeValues`,
then sent the complete event frame. Server-only `recheckAtReceive` and the
original sent-region snapshots did not cross the wire. Once serialized, the
client could neither recover a recipient omitted at generation nor suppress a
now-disjoint passel at callback entry.

The chosen private operation is `claim_deferred_attribute_update = 144`.
For regional receive-order attribute updates under process push, the service
retains each accepted passel and original producer-region snapshots in its
joined-federate queue, then sends a readiness marker carrying the federation
name and receiver's joined-federate ID. The client puts that marker into the
same heterogeneous FIFO as other pushed events. When its
HLA_IMMEDIATE callback task is admitted, it claims **one** passel. The
authoritative service rechecks the receiver's current subscription/committed
region, returns the filtered event or a suppressed result, and consumes that
one queue slot. At callback admission, the client rejects a marker whose join
identity no longer matches. The callback bridge holds a client lifetime lease
during the claim and cancels queued work on resignation or close; the producer
snapshot never becomes client-supplied authority. Timestamped/TSO and 2010
paths were not changed.

The strict case checks enabled late overlap, late disjoint suppression,
disabled generation-time exclusion, current subscription/range suppression
in both modes, source-region mutation after send, two queued markers/FIFO,
and object deletion before callback entry. Neighboring ordinary nonregional
push behavior remains on its prior path and is checked separately.

## Next bounded handoff

Prove concurrent subscription changes against the network claim-to-user
callback window with a deterministic interlock, then add fresh installed-
package/process-host evidence. Audit ordinary nonregional attribute updates,
prove stale-marker rejection across resign/rejoin with a dedicated case, and
check concurrent producers' readiness-marker ordering before broadening DSE
coverage. The callback-entry decision is made at task admission, but a
separate concurrent service call could still mutate subscription after the
claim response and before user callback invocation. Do not claim that race,
full §8.1.8, general TSO parity, or external conformance from this lane.
