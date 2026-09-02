# Umbra RTI test support

This development-only package contains the provider-neutral Python conformance
mixins used by the native, JPype, and mock-Java test suites. It is not a
runtime dependency of applications using `hla-rti-api`.

It also contains `surface_matrix.py`. `iter_surface_matrix()` yields the same
callback-model, logical-time, save-kind, advance-service, and
one/two/three-member cases for a transplanted provider. The matrix only
describes test dimensions; it does not implement federation scheduling.
`assert_event_order()` checks the required callback partial order while
allowing provider-specific advisory callbacks around it.

`iter_callback_provenance_matrix()` supplies the shared five-case callback
payload vector: discovery, untimed reflection/interaction, timestamped
regional reflection, and timestamped no-region interaction.  The fake Java
2010 and 2025 callback-proxy tests consume the same vector and normalize their
edition-specific carrier layouts to producer, tag, region, time, order, and
retraction assertions.  The 2010 and 2025 JNI lanes also consume the vector
with C++-backed Java carrier factories. It proves transport parity only;
callback scheduling and federation policy remain provider-owned.

`CallbackDeliveryObservation` and `normalize_callback_delivery()` provide the
next comparison layer for multi-federate tests.  Each adapter records a
per-recipient sequence plus normalized semantic handles, payload, producer,
region, time, order, and retraction values.  Normalization ignores only
cross-recipient interleaving (which is provider-defined); it rejects duplicate
or missing sequence numbers and `assert_callback_delivery_parity()` reports
the first semantic mismatch.  The 2010 and 2025 Java-shaped fixtures consume
the same two-recipient stream, so a transplanted provider can reuse the
ordering check without depending on its private handle bytes.

The callback-overload carrier gate complements those semantic vectors.  The
2010 JNI route invokes all 60 reflected ``FederateAmbassador`` overloads for
both integer and floating logical-time carriers; the 2025 JNI route invokes
all 62 overloads the same way.  Each invocation starts with standard Java
handles, typed sets/maps, enums, status arrays, information sets, byte arrays,
or logical times and verifies the corresponding Python value type and payload.
This is a carrier/dispatch check only: callback scheduling, federation policy,
and provider-owned handle encodings remain outside the transplantable
contract.

`wire_matrix.py` provides the companion logical-time vectors. They include
fixed-width offset encodings, signed-zero and large-integer boundaries,
non-finite/negative payloads, truncation/trailing-octet failures, and
provider-owned add/subtract/distance precision cases. A transplanted adapter
test can consume these vectors without copying an implementation's codec.

`data_element_matrix.py` provides the shared basic-value vectors for the
2010/2025 encoder surfaces. The 2010 matrix has 79 deterministic edge cases
across the twenty basic carriers (signed limits, byte high bits, UTF-16,
length counts, and opaque octets); the 2025 view adds the six unsigned
integer carriers. It contains values only—not expected wire bytes—so direct
C++ and Java/JPype tests must use their own standard `EncoderFactory` and
compare provider-produced round-trips. This is the portable evidence a new
RTI adapter can transplant without copying a codec.

`provider_extension_matrix.py` contains the explicitly provider-scoped
`HLAextendableVariantRecord` vectors used by the native 2025 test. It covers
known alternatives, an unknown future alternative, malformed length/payload
cases, and trailing-byte rejection. This module is deliberately not part of
the abstract RTI contract; it records the extension boundary so a bridge can
transplant the evidence without claiming that every edition exposes it. The
same module records seven vendor-owned logical-time operation shapes and
four fixed-size unknown-`DataElement` decode probes plus four exact/offset/
undersized destination-window encode probes. Those vectors verify raw-carrier
and typed-error transport only; they do not define portable arithmetic or
wire semantics.

`save_restore_matrix.py` adds the lifecycle outcome cross-product. Its 720
default cases exercise complete, not-complete, and abort outcomes for both save
and restore across the callback, logical-time, save-overload, all five standard
advance-service forms, and one/two-member dimensions. Provider tests assert the
corresponding standard Java/C++ service names, keeping the matrix portable while
making the lifecycle boundary explicit.

Install it from a checkout when running provider conformance tests:

    python -m pip install -e packages/umbra-rti-api
    python -m pip install -e packages/umbra-rti-test-support
