"""Provider-neutral state-space helpers for Python RTI surface tests.

The helpers in this module intentionally contain no RTI implementation logic.
They describe the dimensions that a provider test should exercise and provide
one small assertion for callback ordering.  A Java/JPype, JNI, or pybind test
can consume the same cases while translating the strings to its edition's
enum types.  Keeping the matrix here makes a transplanted provider test
portable without making the test-support package depend on either provider.
"""

from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from itertools import product


@dataclass(frozen=True, slots=True)
class SurfaceMatrixCase:
    """One provider-neutral callback/time/save/member state-space point."""

    callback_model: str
    time_implementation: str
    save_kind: str
    advance_service: str
    member_count: int

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for ``subTest`` and reports."""

        return ":".join(
            (
                self.callback_model,
                self.time_implementation,
                self.save_kind,
                self.advance_service,
                f"members={self.member_count}",
            )
        )


class SurfaceEventTrace:
    """Tiny event sink that can be embedded in any federate ambassador."""

    __slots__ = ("events",)

    def __init__(self) -> None:
        self.events: list[str] = []

    def record(self, event: str) -> None:
        if not isinstance(event, str) or not event:
            raise ValueError("event names must be non-empty strings")
        self.events.append(event)


@dataclass(frozen=True, slots=True)
class CallbackProvenanceVector:
    """One provider-neutral callback payload/provenance case.

    The 2010 and 2025 Java contracts place producer, region, timing, and
    ordering metadata in different callback positions (the 2010 surface uses
    supplemental-info records).  These vectors describe the semantic
    envelope only.  A route-specific test supplies the corresponding Java
    carrier shape and normalizes the result before asserting these values.
    ``None`` means that a field is not part of that callback case; empty
    payloads and tags are therefore still representable as bytes.
    """

    label: str
    callback: str
    timed: bool
    object_instance: bytes | None = None
    object_class: bytes | None = None
    interaction_class: bytes | None = None
    instance_name: str | None = None
    payload_handle: bytes | None = None
    payload: bytes | None = None
    tag: bytes | None = None
    transportation: bytes | None = None
    producing_federate: bytes | None = None
    sent_region: bytes | None = None
    time_implementation: str | None = None
    time_value: int | float | None = None
    sent_order: str | None = None
    received_order: str | None = None
    retraction: bytes | None = None

    def __post_init__(self) -> None:
        if self.callback not in {
            "discoverObjectInstance",
            "reflectAttributeValues",
            "receiveInteraction",
        }:
            raise ValueError(f"unsupported callback provenance name: {self.callback}")
        if not self.label:
            raise ValueError("callback provenance labels must be non-empty")
        if self.timed:
            if (
                self.time_implementation is None
                or self.time_value is None
                or self.sent_order is None
                or self.received_order is None
            ):
                raise ValueError("timed callback vectors need time and order metadata")
        elif any(
            value is not None
            for value in (
                self.time_implementation,
                self.time_value,
                self.sent_order,
                self.retraction,
            )
        ):
            raise ValueError("untimed callback vectors cannot carry timing metadata")
        if self.callback == "discoverObjectInstance":
            if any(
                value is not None
                for value in (
                    self.interaction_class,
                    self.payload_handle,
                    self.payload,
                    self.tag,
                    self.transportation,
                    self.sent_region,
                )
            ):
                raise ValueError("discovery vectors cannot carry value metadata")
            if self.object_instance is None or self.object_class is None:
                raise ValueError("discovery vectors need object and class handles")
            if self.instance_name is None or self.producing_federate is None:
                raise ValueError("discovery vectors need name and producer")
        else:
            if self.callback == "reflectAttributeValues":
                if self.object_instance is None:
                    raise ValueError("reflection vectors need an object handle")
            elif self.interaction_class is None:
                raise ValueError("interaction vectors need an interaction handle")
            if (
                self.payload_handle is None
                or self.payload is None
                or self.tag is None
                or self.transportation is None
                or self.producing_federate is None
            ):
                raise ValueError("value callbacks need payload, tag, transport, and producer")

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for ``subTest`` and reports."""

        return f"{self.callback}:{self.label}"


@dataclass(frozen=True, slots=True)
class CallbackDeliveryObservation:
    """A route-normalized callback delivery for differential comparisons.

    Provider handle encodings are deliberately not part of the comparison
    contract: a C++ provider and a Java provider are free to use different
    opaque bytes.  The adapter tests decode those handles to the semantic
    fields below before constructing an observation.  ``recipient`` and
    ``sequence`` identify the per-federate stream; cross-recipient delivery
    order is intentionally not prescribed by this helper.
    """

    recipient: str
    sequence: int
    vector_id: str
    callback: str
    timed: bool
    object_instance: bytes | None = None
    object_class: bytes | None = None
    interaction_class: bytes | None = None
    instance_name: str | None = None
    payload_handle: bytes | None = None
    payload: bytes | None = None
    tag: bytes | None = None
    transportation: bytes | None = None
    producing_federate: bytes | None = None
    sent_region: bytes | None = None
    time_implementation: str | None = None
    time_value: int | float | None = None
    sent_order: str | None = None
    received_order: str | None = None
    retraction: bytes | None = None

    def __post_init__(self) -> None:
        if not self.recipient:
            raise ValueError("callback delivery recipients must be non-empty")
        if isinstance(self.sequence, bool) or not isinstance(self.sequence, int):
            raise ValueError("callback delivery sequence must be an integer")
        if self.sequence < 0:
            raise ValueError("callback delivery sequence must be non-negative")
        if not self.vector_id:
            raise ValueError("callback delivery vectors need a stable id")
        if self.callback not in {
            "discoverObjectInstance",
            "reflectAttributeValues",
            "receiveInteraction",
        }:
            raise ValueError(f"unsupported callback delivery name: {self.callback}")
        if self.timed:
            if (
                self.time_implementation is None
                or self.time_value is None
                or self.sent_order is None
                or self.received_order is None
            ):
                raise ValueError("timed deliveries need time and order metadata")
        elif any(
            value is not None
            for value in (
                self.time_implementation,
                self.time_value,
                self.sent_order,
                self.retraction,
            )
        ):
            raise ValueError("untimed deliveries cannot carry timing metadata")

    @classmethod
    def from_vector(
        cls,
        vector: CallbackProvenanceVector,
        *,
        recipient: str,
        sequence: int,
    ) -> "CallbackDeliveryObservation":
        """Create the expected normalized observation for a shared vector."""

        return cls(
            recipient=recipient,
            sequence=sequence,
            vector_id=vector.case_id,
            callback=vector.callback,
            timed=vector.timed,
            object_instance=vector.object_instance,
            object_class=vector.object_class,
            interaction_class=vector.interaction_class,
            instance_name=vector.instance_name,
            payload_handle=vector.payload_handle,
            payload=vector.payload,
            tag=vector.tag,
            transportation=vector.transportation,
            producing_federate=vector.producing_federate,
            sent_region=vector.sent_region,
            time_implementation=vector.time_implementation,
            time_value=vector.time_value,
            sent_order=vector.sent_order,
            received_order=vector.received_order,
            retraction=vector.retraction,
        )

    @property
    def semantic_key(self) -> tuple[object, ...]:
        """Return the provider-independent payload/provenance tuple."""

        return (
            self.vector_id,
            self.callback,
            self.timed,
            self.object_instance,
            self.object_class,
            self.interaction_class,
            self.instance_name,
            self.payload_handle,
            self.payload,
            self.tag,
            self.transportation,
            self.producing_federate,
            self.sent_region,
            self.time_implementation,
            self.time_value,
            self.sent_order,
            self.received_order,
            self.retraction,
        )


def normalize_callback_delivery(
    observations: Iterable[CallbackDeliveryObservation],
) -> tuple[CallbackDeliveryObservation, ...]:
    """Canonicalize per-recipient callback streams for route comparison.

    A provider may interleave callbacks for different federates differently,
    so this operation sorts by recipient and then by the sequence supplied by
    that recipient's recorder.  It still rejects duplicate, missing, or
    non-contiguous sequence numbers: those are binding/normalization errors,
    not provider scheduling choices.
    """

    by_recipient: dict[str, list[CallbackDeliveryObservation]] = {}
    for observation in observations:
        if not isinstance(observation, CallbackDeliveryObservation):
            raise TypeError("callback deliveries must be observations")
        by_recipient.setdefault(observation.recipient, []).append(observation)

    normalized: list[CallbackDeliveryObservation] = []
    for recipient in sorted(by_recipient):
        stream = sorted(by_recipient[recipient], key=lambda item: item.sequence)
        expected = list(range(len(stream)))
        actual = [item.sequence for item in stream]
        if actual != expected:
            raise ValueError(
                f"callback stream for {recipient!r} is not contiguous: {actual!r}"
            )
        normalized.extend(stream)
    return tuple(normalized)


def assert_callback_delivery_parity(
    expected: Iterable[CallbackDeliveryObservation],
    actual: Iterable[CallbackDeliveryObservation],
) -> None:
    """Assert that two normalized callback streams carry the same semantics."""

    expected_normalized = normalize_callback_delivery(expected)
    actual_normalized = normalize_callback_delivery(actual)
    if len(expected_normalized) != len(actual_normalized):
        raise AssertionError(
            "callback delivery count differs: "
            f"expected {len(expected_normalized)}, got {len(actual_normalized)}"
        )
    for index, (expected_item, actual_item) in enumerate(
        zip(expected_normalized, actual_normalized, strict=True)
    ):
        if (
            expected_item.recipient,
            expected_item.sequence,
            expected_item.semantic_key,
        ) != (
            actual_item.recipient,
            actual_item.sequence,
            actual_item.semantic_key,
        ):
            raise AssertionError(
                "callback delivery mismatch at index "
                f"{index}: expected {expected_item!r}, got {actual_item!r}"
            )


DEFAULT_CALLBACK_MODELS = ("HLA_IMMEDIATE", "HLA_EVOKED")
DEFAULT_TIME_IMPLEMENTATIONS = ("HLAinteger64Time", "HLAfloat64Time")
DEFAULT_SAVE_KINDS = ("scalar", "timestamped")
DEFAULT_ADVANCE_SERVICES = (
    "timeAdvanceRequest",
    "timeAdvanceRequestAvailable",
    "nextMessageRequest",
    "nextMessageRequestAvailable",
    "flushQueueRequest",
)
DEFAULT_MEMBER_COUNTS = (1, 2, 3)

# Reflected standard Java callback declarations, including overloads.  The
# edition-specific JNI tests invoke each declaration for both logical-time
# families, so the corresponding carrier-matrix evidence count is twice these
# values.  Keeping the accounting here lets the catalog and transplanted
# provider tests share one explicit baseline without importing JPype.
CALLBACK_OVERLOAD_COUNTS = {"2010": 60, "2025": 62}


def iter_callback_provenance_matrix() -> tuple[CallbackProvenanceVector, ...]:
    """Return the shared callback payload/provenance parity vectors.

    The set deliberately includes discovery, untimed value delivery, a
    timestamped regional reflection, and a timestamped interaction without
    regions.  That exercises both supplemental-info branches in 2010 and the
    optional-region/timestamp branches in 2025 while remaining independent of
    a provider's scheduling policy.
    """

    return (
        CallbackProvenanceVector(
            label="discovery-untimed",
            callback="discoverObjectInstance",
            timed=False,
            object_instance=b"callback-object",
            object_class=b"callback-class",
            instance_name="callback-instance",
            producing_federate=b"callback-producer",
        ),
        CallbackProvenanceVector(
            label="reflection-untimed",
            callback="reflectAttributeValues",
            timed=False,
            object_instance=b"callback-object",
            payload_handle=b"callback-attribute",
            payload=b"callback-attribute-value",
            tag=b"callback-reflect-tag",
            transportation=b"callback-transport",
            producing_federate=b"callback-producer",
            received_order="RECEIVE",
        ),
        CallbackProvenanceVector(
            label="interaction-untimed",
            callback="receiveInteraction",
            timed=False,
            interaction_class=b"callback-interaction",
            payload_handle=b"callback-parameter",
            payload=b"callback-parameter-value",
            tag=b"callback-interaction-tag",
            transportation=b"callback-transport",
            producing_federate=b"callback-producer",
            received_order="RECEIVE",
        ),
        CallbackProvenanceVector(
            label="reflection-timed-region",
            callback="reflectAttributeValues",
            timed=True,
            object_instance=b"callback-object",
            payload_handle=b"callback-attribute",
            payload=b"callback-timed-value",
            tag=b"callback-timed-reflect-tag",
            transportation=b"callback-transport",
            producing_federate=b"callback-producer",
            sent_region=b"callback-region",
            time_implementation="HLAinteger64Time",
            time_value=7,
            sent_order="TIMESTAMP",
            received_order="TIMESTAMP",
            retraction=b"callback-reflect-retraction",
        ),
        CallbackProvenanceVector(
            label="interaction-timed-no-region",
            callback="receiveInteraction",
            timed=True,
            interaction_class=b"callback-interaction",
            payload_handle=b"callback-parameter",
            payload=b"callback-timed-parameter",
            tag=b"callback-timed-interaction-tag",
            transportation=b"callback-transport",
            producing_federate=b"callback-producer",
            time_implementation="HLAfloat64Time",
            time_value=7.5,
            sent_order="TIMESTAMP",
            received_order="RECEIVE",
            retraction=b"callback-interaction-retraction",
        ),
    )


def iter_surface_matrix(
    *,
    callback_models: Iterable[str] = DEFAULT_CALLBACK_MODELS,
    time_implementations: Iterable[str] = DEFAULT_TIME_IMPLEMENTATIONS,
    save_kinds: Iterable[str] = DEFAULT_SAVE_KINDS,
    advance_services: Iterable[str] = DEFAULT_ADVANCE_SERVICES,
    member_counts: Iterable[int] = DEFAULT_MEMBER_COUNTS,
) -> tuple[SurfaceMatrixCase, ...]:
    """Build a deterministic, duplicate-free provider test matrix.

    Inputs are copied before Cartesian expansion so callers may provide
    generators.  Invalid member counts are rejected early: a zero-member case
    cannot exercise a federate callback boundary and negative counts are almost
    certainly a test authoring error.
    """

    dimensions = (
        tuple(callback_models),
        tuple(time_implementations),
        tuple(save_kinds),
        tuple(advance_services),
        tuple(member_counts),
    )
    if any(not values for values in dimensions):
        raise ValueError("surface matrix dimensions must not be empty")
    if any(
        not isinstance(value, str) or not value
        for values in dimensions[:4]
        for value in values
    ):
        raise ValueError("surface matrix labels must be non-empty strings")
    if any(
        isinstance(value, bool) or not isinstance(value, int) or value < 1
        for value in dimensions[4]
    ):
        raise ValueError("surface matrix member counts must be positive integers")
    cases = tuple(SurfaceMatrixCase(*values) for values in product(*dimensions))
    if len({case.case_id for case in cases}) != len(cases):
        raise ValueError("surface matrix dimensions produce duplicate case ids")
    return cases


def assert_event_order(events: Sequence[str], *required: str) -> None:
    """Assert that named events occur in order, allowing unrelated events.

    Provider callback queues may add advisory callbacks around a required
    sequence.  This assertion therefore checks a partial order rather than
    requiring an implementation-specific exact event list.
    """

    cursor = 0
    for expected in required:
        try:
            cursor = events.index(expected, cursor) + 1
        except ValueError as error:
            raise AssertionError(
                f"callback event {expected!r} is missing or out of order; events={list(events)!r}"
            ) from error


__all__ = [
    "DEFAULT_ADVANCE_SERVICES",
    "DEFAULT_CALLBACK_MODELS",
    "DEFAULT_MEMBER_COUNTS",
    "DEFAULT_SAVE_KINDS",
    "DEFAULT_TIME_IMPLEMENTATIONS",
    "CALLBACK_OVERLOAD_COUNTS",
    "CallbackDeliveryObservation",
    "CallbackProvenanceVector",
    "SurfaceEventTrace",
    "SurfaceMatrixCase",
    "assert_callback_delivery_parity",
    "assert_event_order",
    "normalize_callback_delivery",
    "iter_callback_provenance_matrix",
    "iter_surface_matrix",
]
