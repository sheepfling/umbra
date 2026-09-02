# Allow Relaxed DDM Policy

## Status

Implemented for the embedded IEEE 1516.1-2025 development profile. This is an
Umbra implementation policy, not a claim that every conforming RTI uses the
same relaxation.

## Standard basis

IEEE 1516.1-2025 §9.1.4 defines strict range overlap using half-open ranges.
Section 9.1.8 requires strict comparison when Allow Relaxed DDM is disabled,
and permits implementation-specific expansion when it is enabled, provided
the enabled overlap set remains a superset of strict overlap. Annex E.9.1.8
also permits an RTI to leave filtering unchanged; Umbra intentionally chooses
a small, deterministic expansion instead.

The local Requirements Lab corpus exposes the official getter requirement but
does not currently export an immutable requirement candidate for the §9.1.4 /
§9.1.8 delivery-policy prose. `RL-030` in
`../testing/REQUIREMENTS-LAB-OBSERVATIONS.md` records that gap. The implementation
therefore keeps direct source evidence distinct from the getter contract rather
than attaching the getter requirement to unrelated routing semantics.

## Umbra policy

For every dimension shared by two committed, non-empty explicit regions:

- A strict overlap qualifies exactly as specified by the standard.
- If strict overlap is absent and Allow Relaxed DDM is disabled, the regions do
  not overlap.
- If strict overlap is absent and Allow Relaxed DDM is enabled, the ranges
  qualify only when one upper bound is exactly equal to the other lower bound.

The regions overlap only when every shared dimension qualifies. Regions with no
shared dimensions still do not overlap. A positive numerical gap never
qualifies, and Umbra does not use an epsilon, a configurable distance, range
growth, or any inferred geographic interpretation. Region validation remains
unchanged: routing sees only committed ranges with `lower < upper` inside the
FDD dimension bound.

The private default region needs no separate relaxation: every valid explicit
region already strictly overlaps its full-range realization. This policy is
applied at the central explicit-region predicate, so currently bounded regional
interaction and object-attribute planners use the same decision.

## Evidence and limits

`Embedded Allow Relaxed DDM expands only touching regional interaction ranges`,
`Embedded Allow Relaxed DDM expands only touching regional object-attribute
ranges`, and `Embedded regional Auto Provide applies Allow Relaxed DDM to
touching source projections and suppresses positive gaps under HLA_EVOKED and
HLA_IMMEDIATE` use a standard Restaurant FOM (or the isolated regional
ownership fixture) plus an isolated FOM switch module. Together they prove:

1. exact boundary-touching ranges deliver only with the switch enabled;
2. a positive gap never delivers; and
3. strict delivery remains available in both modes.

The Auto Provide case adds the discovery boundary: a touching source and
subscription range is eligible only with the enabled switch, induces one
provider callback with the required empty tag, and delivers one scoped
response. Moving the known recipient to a positive gap suppresses the update;
restoring strict overlap permits one ordinary regional update without a second
discovery or provider solicitation. The same assertions run under both
HLA_EVOKED and HLA_IMMEDIATE.

The object-attribute case repeats the state transition through regional object
registration, discovery, and reflection. It confirms that a recipient made
known by a relaxed boundary stops receiving after a nonzero gap, and that a
strict-overlap change discovers the previously filtered recipient in the
disabled configuration.

The evidence is limited to receive-order regional interaction,
object-attribute delivery, and receive-order Auto Provide discovery/response
in the embedded profile. Broader
multi-dimension, timestamped, advisory, update-rate, transport, package, and
conformance matrices remain separate work.
