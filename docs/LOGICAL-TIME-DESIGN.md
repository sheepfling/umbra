# Logical-time foundation

## Non-negotiable standard behavior

Umbra’s time policy follows the 2025 source corpus, not the adjacent Python
reference implementation. The Create Federation Execution argument is optional,
but when it is omitted the RTI-provided `HLAfloat64Time` representation shall be
used. The 2025 binding also states that every RTI must implement reference time
types named `HLAinteger64Time` and `HLAfloat64Time`.

The relevant Requirements-Lab source records are:

- `requirement-candidate-content-clauses-04-federation-management-page-053-l87-18`
  for the omitted-argument `HLAfloat64Time` default; and
- `requirement-candidate-content-clauses-04-federation-management-page-049-l133-42`
  for the two default logical-time implementations.

## Current state

The immutable private `FomCatalog` projection now retains the FOM’s
`logicalTime` and `logicalTimeInterval` data-type declarations. This is model
metadata only. Umbra now provides the complete official reference-time ABI:

- `HLAinteger64Time` / `HLAinteger64Interval` with HLAinteger64BE encoding,
  final value `2^63 - 1`, and epsilon `1`;
- `HLAfloat64Time` / `HLAfloat64Interval` with HLAfloat64BE encoding, finite
  final value `0x1.FFFFFFFFFFFFFP+1023`, and epsilon
  `0x0.0000000000001P-1022`; and
- both official factories, `HLAlogicalTimeFactoryFactory`, and the separate
  static `umbra::fedtime` forwarding entry point.

The official `HLAlogicalTime` and `HLAlogicalTimeInterval` encoding helpers
now delegate their opaque byte representation to the selected factory. They
retain a factory and an internally copied value, select the initial time or
zero interval at construction, and do not define an Umbra-specific time wire
layout. The current `decodeFrom` implementation is intentionally only for the
fixed eight-octet reference profiles: `LogicalTimeFactory` does not provide a
consumed-length result that could delimit arbitrary third-party variable-width
time encodings in a containing byte stream. The wrappers are an SDK encoding
foundation, not custom-time interoperability or public-service evidence.

The float epsilon arithmetic uses the next representable value when native
floating-point rounding would otherwise leave a time unchanged, as required by
the source text. The opt-in embedded federation-management profile now selects
and rechecks a compatible reference factory before public Create/Join state is
committed. A joined federate can call `getTimeFactory` to receive a new,
caller-owned instance of that immutable federation selection. The default
packaged profile still keeps Create/Join and `getTimeFactory` on the fallback.

The Python `LogicalTimeFactory.add`, `subtract`, and `difference` operations
remain provider-owned: the native binding delegates to the C++ logical-time
operators and the Java adapter invokes the selected Java time/interval objects.
Each returns a new immutable Python snapshot, so Python does not reimplement or
silently change vendor arithmetic semantics.

Each joined federate in the development profile has a private state object
holding only official `LogicalTime` and `LogicalTimeInterval` instances. It
starts at the selected factory's initial time. Time Advance Request remains
pending until Time Advance Grant dispatches; the initial Enable Time Regulation
and Enable Time Constrained paths likewise remain pending until their respective
callbacks dispatch. A successful regulation callback retains its official
lookahead for Query Lookahead. The current profile has five bounded public TSO
producers—the timestamped `Send Interaction`, `Update Attribute Values`,
`Delete Object Instance`, `Send Directed Interaction`, and
`Send Interaction With Regions` overloads with retraction—while its
private federation coordinator carries queued, in-transit, and delivered-
since-last-advance timestamp state. Role callbacks and the other service
families still use the current logical time because broader public timestamped
delivery is not enabled. Separately, the official asynchronous-delivery switch
now gates receive-order callbacks for idle time-constrained federates: it is
disabled by default, releases deferred receive-order work when enabled or when
Time Advancing begins, and restores the normal gate when disabled. Timestamped
messages remain time-advance gated, and deferred callback closures are not part
of the in-memory save/restore snapshot.
Read-only GALT/LITS include the private incoming state when present; without
it, both are the smallest other regulator's current (or pending-advance) time
plus actual lookahead. A forward Time Advance Request from a zero-lookahead
regulator makes that boundary exclusive, so it is advanced by the selected
factory's epsilon. GALT remains undefined without another regulator, while
LITS may be defined by a queued incoming TSO timestamp.

The private registry now owns a `FederationTimeCoordinator` for each
federation. Public embedded joins atomically register a federate's selected
time state with membership, and `timeSnapshotFor` returns the FDD definition,
including the default-disabled or explicitly enabled Non-Regulated-Grant
switch, together with all runtime-backed federate snapshots. This is the
necessary shared input boundary for the GALT/LITS algorithm and the limited TAR
scheduler. The shared view now includes the private queue's queued, in-transit,
and delivered-since-last-advance timestamp states; the scheduler still exposes
only the bounded TAR service plus the separate bounded interaction TSO path.

The supplied Restaurant FOM declares `HLAinteger64Time`; that declaration is
recorded, but it must not be treated as an implicit override of the standard
empty-argument Create default. The compatibility relationship between a
selected factory and FDD time tables is enforced by the development profile
before it stores a definition or admits an additional-FOM join.

## Build order

1. Keep the private selector's empty-name default and FDD compatibility test
   ahead of every mutation in the development profile.
2. Keep `getTimeFactory` tied to the selected federation definition rather
   than a process-wide default or an Umbra-specific factory.
3. Extend GALT/LITS and the limited TAR scheduler with timestamped-queue
   coordination before adding any transport or TSO producer.
4. Add the remaining time-advance variants only with their full cross-federate
   constraints and callback-ordering tests. Modify Lookahead, the bounded
   currently-queued-message `Next Message Request`, the two Available forms,
   and the bounded in-process `Flush Queue Request`/`Flush Queue Grant` path
   are now covered by their own exact 2025 contracts and scenarios. Future
   transport and complete cross-federate coordination remain separate work.

The Python implementation is useful as a list of eventual scenarios, but its
integer default is deliberately not imported into Umbra’s semantics.
