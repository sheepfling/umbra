# Java RTI conformance TCK prioritized backlog

This backlog orders the next portable Java TCK work from small, frequently
used, low-state services toward specialized features that require more
federates, time management, regions, persistence, or provider configuration.
The goal is to keep the compiled Java test artifact transplantable to another
IEEE 1516.1-2025 RTI.

## Prioritization rule

Work moves upward in complexity only after the earlier item has a standard-API
implementation, a C++ source-vector mapping, a provider-neutral FOM/configuration
story, and a passing result under the checked-in JNI profile. A provider may
still report `unsupported` when the required capability is genuinely absent.

| Priority | Decision rule | Typical shape |
| --- | --- | --- |
| P0 | Already shipped and exercised by every ordinary provider run | One federation, ordinary declarations, opaque octets, standard callbacks |
| P1 | High-use behavior with little or no multi-federate state | Lookups, error matrices, declaration advisories, ordinary directed routes |
| P2 | Useful but stateful or less universal | Transport/order controls, ownership, synchronization |
| P3 | Specialized simulation behavior | Timestamped delivery, retraction, asynchronous/time transitions, DDM regions |
| P4 | Configuration-heavy or provider-dependent | Save/restore and MOM/service reporting |

## P0 — shipped baseline

These are complete enough to serve as the portable foundation:

- `java-tck.factory-discovery`, `java-tck.encoder-round-trip`, and
  `java-tck.overloads-and-exceptions` establish the standard factory,
  encoding, connection, callback-model, and exception boundary.
- `java-tck.federation-membership` covers create, all four ordinary Java join
  overloads, list/query, resign, destroy, and disconnect.
- `java-tck.declaration-management` and `java-tck.object-management` cover
  ordinary object-class/attribute publication and subscription, reservations,
  registration, discovery, updates, requests, reflection, and deletion.
- `java-tck.attribute-interaction` covers ordinary interaction lookup,
  publication/subscription, parameter/tag/producer delivery, passive-to-active
  promotion, unsubscription/unpublication, and `enableCallbacks` /
  `disableCallbacks` under both callback models.

The source of truth for these scenario IDs is the
[Java TCK scenario catalog](../../compliance/catalogs/java-tck-scenario-catalog.json).

## P1 — implemented, small and broadly reusable

### 1. Portable support lookups and handle identity

Implemented as `java-tck.support-services`. The scenario covers the standard
support services that are useful to any provider adapter and do not require
regions or time:

- order and transportation type lookup/name round trips;
- available-dimension lookup without creating or associating a region; and
- public handle encode/decode and normalization for federate, class,
  interaction, object, attribute, parameter, dimension, and retraction handles.

The acceptance boundary is stable cross-federate identities,
missing/invalid-handle exceptions, and no assumptions about the provider's
numeric handle representation. Useful C++ source vectors include the
interaction/parameter lookup cases and `Embedded public handle decoders enforce
lifecycle and preserve encoded identities`.

### 2. Ordinary edge and negative matrices

Implemented as `java-tck.ordinary-edges`, extending the shipped
object/attribute/interaction helpers with the inexpensive boundaries most
vendor integrations encounter:

- empty, duplicate, and mixed-validity attribute/parameter maps;
- idempotent repeated declarations and whole-class versus subset removal;
- not-published, not-owned, unknown-object, unknown-class, and
  not-subscribed failures; and
- callback tag copies, callback ordering, inherited-class promotion, and both
  `HLA_EVOKED` and `HLA_IMMEDIATE` where the service is callback-visible.

Each assertion names a standard exception or callback contract and does not
decode a provider-specific FOM data type. This is the low-cost ordinary lane
that precedes the larger feature families.

### 3. Ordinary declaration relevance and turn-on/turn-off callbacks

Implemented as `java-tck.relevance-advisories`, translating the non-regional
relevance path that covers the remaining ordinary “enable/disable” behavior:

- `get/setObjectClassRelevanceAdvisorySwitch` and
  `get/setInteractionRelevanceAdvisorySwitch`;
- `startRegistrationForObjectClass` and `stopRegistrationForObjectClass`;
- `turnInteractionsOn` / `turnInteractionsOff`; and
- `turnUpdatesOnForObjectInstance` / `turnUpdatesOffForObjectInstance`,
  including the named update-rate callback overload.

Use a small standard FOM/configuration that explicitly enables the advisory
switches, keep all `RegionHandle` and `*WithRegions` routes out, and test
passive declarations as non-relevance state. The C++ anchors are
`Embedded declaration relevance advisories follow ordinary 2025 publication
and subscription transitions` and
`Embedded attribute relevance advisories follow scope transitions`.

### 4. Ordinary directed interactions

Implemented as `java-tck.directed-interactions`, a separate scenario for
directed interaction publication/subscription and delivery:

- `publishObjectClassDirectedInteractions` /
  `unpublishObjectClassDirectedInteractions`;
- targeted and universal directed subscriptions; and
- `sendDirectedInteraction` with `receiveDirectedInteraction`, including
  target identity and standard failure paths.

Keep timestamped and regional directed interactions in P3. The primary C++
vector is `Embedded directed interactions route to known object-class
subscribers`.

## P2 — implemented, useful but more stateful

### 5. Ordinary transportation and order controls

Implemented as `java-tck.transport-order`, covering default and per-instance
attribute order/transportation changes,
interaction order changes, queries, and their confirmation callbacks. Start
with receive-order/default-region behavior; leave timestamped retraction and
mixed fanout for P3.

Primary vectors: `Embedded transportation type control commits at callbacks
and preserves FOM defaults` and `Embedded order type control captures defaults,
instance overrides, and publisher interaction order`.

### 6. Basic attribute ownership

Implemented in the capability-gated `java-tck.ownership` scenario in this
order: query ownership, unconditional divestiture, acquisition, acquisition
notifications, and denial/cancellation. It uses a two-federate ordinary FOM;
negotiated and partial-transfer races remain later.

### 7. Synchronization points

Implemented in `java-tck.synchronization` with multiple members, late joiners,
failed registration, callback ordering, explicit membership, and resign
cleanup. It remains a relatively small multi-federate feature ahead of the
time/save state machines.

## P3 — specialized simulation behavior

### 8. Basic time-managed delivery, then timestamped data

Expand `java-tck.time-advance` in layers:

1. time roles and advance/grant lifecycle;
2. timestamped ordinary attribute updates and interactions;
3. retraction and receive-order/TSO metadata; and
4. asynchronous delivery plus time-regulation/time-constrained disable and
   re-enable transitions.

Do not mix save/restore, regions, or ownership transfer into the first time
scenario. The C++ suite's ordinary timestamped attribute/interaction cases are
the parity source.

### 9. DDM regions

Expand `java-tck.ddm` only after the non-regional routes are stable:

- region creation, bounds, commit, association, and deletion;
- regional object-attribute and interaction subscriptions;
- overlap/disjoint filtering and passive regional declarations; and
- conveyed-region callback metadata.

This is intentionally later: it multiplies every object and interaction route
and depends on a compatible dimension-bearing FOM. It remains a separate
scenario and must not leak region types into the ordinary Java source set.

## P4 — last and explicitly capability-dependent

### 10. Save and restore

Expand `java-tck.save-restore` after the live object, interaction, ownership,
and time state models are stable. Test save/restore interlocks first, then
ordinary live state, queued timestamped state, and cancellation/resign edges.
Keep provider snapshot format and persistence implementation details outside the
portable Java source.

### 11. MOM and service reporting

Keep `java-tck.mom` last. Add public MOM lookup/status behavior first, then
service invocation reports, reporting switches, malformed report inputs, and
file/report lifecycle. MOM depends on the provider's MIM/FOM and configuration,
so every result must remain explicitly `pass`, `unsupported`, or `not
applicable`; it should never determine whether the ordinary portable TCK can
run.

## Working queue

P0 through P2 are now implemented and passing under the checked-in Umbra JNI
profile. The next implementation queue begins at P3: basic time-managed
delivery and then DDM regions, while save/restore and MOM remain P4. The P1/P2
Java sources deliberately contain no DDM region or time-management routes.
