"""Provider-specific encoding vectors that have a stable Python boundary.

The IEEE API does not define ``HLAextendableVariantRecord`` as a portable
factory in every edition.  Umbra's native 2025 provider nevertheless exposes
the C++ carrier as an explicit extension, and the JNI test exercises the same
wire shape.  Keeping the bytes here lets native and bridge tests share the
malformed-length cases without pretending that the extension is part of the
provider-neutral abstract contract.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class ExtendableVariantWireVector:
    """One known, unknown, or malformed extendable-variant payload."""

    label: str
    encoded: bytes
    valid: bool
    discriminant: bytes
    value: bytes | None
    encoded_length: int | None

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for subtests and reports."""

        return f"HLAextendableVariantRecord:{self.label}"


@dataclass(frozen=True, slots=True)
class VendorTimeArithmeticVector:
    """One provider-owned logical-time operation shape.

    The matrix names the receiver and argument carrier without assigning a
    standard numeric result.  A vendor may use a custom time implementation,
    so the adapter must leave the operation on that Java object instead of
    coercing it to one of the four standard Python time classes.
    """

    label: str
    receiver: str
    operation: str
    argument: str
    expected_marker: str

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for subtests and reports."""

        return f"VendorLogicalTime:{self.label}"


@dataclass(frozen=True, slots=True)
class VendorDataElementWireVector:
    """One fixed-size wire probe for an unknown provider data element."""

    label: str
    payload: bytes
    valid: bool

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for subtests and reports."""

        return f"VendorOpaqueData:{self.label}"


@dataclass(frozen=True, slots=True)
class VendorDataElementEncodeVector:
    """One destination window for an unknown provider data element.

    ``length`` is the writable cursor window, not the backing-array size.
    Valid cases prove offset writes and trailing-byte preservation; invalid
    cases prove that a provider encoder reports a typed failure before it
    mutates the caller's cursor or bytes.
    """

    label: str
    backing: bytes
    offset: int
    length: int
    valid: bool

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for subtests and reports."""

        return f"VendorOpaqueDataEncode:{self.label}"


def iter_extendable_variant_wire_matrix() -> tuple[ExtendableVariantWireVector, ...]:
    """Return the provider extension's valid and malformed wire vectors.

    The two known alternatives use the octet discriminant prototype from the
    native test.  The unknown alternative demonstrates the extension's
    forward-compatible skip rule; all remaining vectors must fail before a
    partial payload is exposed.
    """

    return (
        ExtendableVariantWireVector(
            "known-integer",
            bytes.fromhex("01 00 00 00 00 00 00 04 10 20 30 40"),
            True,
            b"\x01",
            bytes.fromhex("10 20 30 40"),
            12,
        ),
        ExtendableVariantWireVector(
            "known-ascii",
            bytes.fromhex("02 00 00 00 00 00 00 05 00 00 00 01 41"),
            True,
            b"\x02",
            bytes.fromhex("00 00 00 01 41"),
            13,
        ),
        ExtendableVariantWireVector(
            "unknown-future",
            bytes.fromhex("03 00 00 00 00 00 00 03 61 62 63"),
            True,
            b"\x03",
            None,
            8,
        ),
        ExtendableVariantWireVector(
            "unknown-empty",
            bytes.fromhex("03 00 00 00 00 00 00 00"),
            True,
            b"\x03",
            None,
            8,
        ),
        ExtendableVariantWireVector(
            "malformed-discriminant-padding",
            bytes.fromhex("01 00 00 01 00 00 00 04 00 00 00 01"),
            False,
            b"\x01",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-truncated-discriminant-padding",
            bytes.fromhex("01 00 00"),
            False,
            b"\x01",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-reserved-length",
            bytes.fromhex("03 00 01 00 00 00 00 00"),
            False,
            b"\x03",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-negative-length",
            bytes.fromhex("03 00 00 00 FF FF FF FF"),
            False,
            b"\x03",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-truncated-value",
            bytes.fromhex("03 00 00 00 00 00 00 04 61 62"),
            False,
            b"\x03",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-truncated-length",
            bytes.fromhex("03 00 00 00 00 00 00"),
            False,
            b"\x03",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-known-value-length",
            bytes.fromhex("01 00 00 00 00 00 00 03 00 00 00"),
            False,
            b"\x01",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-known-zero-length",
            bytes.fromhex("01 00 00 00 00 00 00 00"),
            False,
            b"\x01",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-known-overlong-value",
            bytes.fromhex("01 00 00 00 00 00 00 05 00 00 00 01 00"),
            False,
            b"\x01",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-positive-length-overflow",
            bytes.fromhex("03 00 00 00 7F FF FF FF"),
            False,
            b"\x03",
            None,
            None,
        ),
        ExtendableVariantWireVector(
            "malformed-trailing-byte",
            bytes.fromhex("02 00 00 00 00 00 00 05 00 00 00 01 41 00"),
            False,
            b"\x02",
            None,
            None,
        ),
    )


def iter_vendor_time_arithmetic_matrix() -> tuple[VendorTimeArithmeticVector, ...]:
    """Return operation shapes for an unknown vendor time implementation.

    ``expected_marker`` belongs to the deliberately tiny fake provider used
    by the adapter tests.  It is not a portable arithmetic result; asserting
    the marker only proves that the call reached the vendor carrier intact.
    """

    return (
        VendorTimeArithmeticVector("time-add", "time", "add", "interval", "vendor-add"),
        VendorTimeArithmeticVector(
            "time-subtract", "time", "subtract", "interval", "vendor-subtract"
        ),
        VendorTimeArithmeticVector(
            "time-distance", "time", "distance", "time", "vendor-distance"
        ),
        VendorTimeArithmeticVector(
            "time-compare", "time", "compareTo", "time", "vendor-time-compare"
        ),
        VendorTimeArithmeticVector(
            "interval-add", "interval", "add", "interval", "vendor-interval-add"
        ),
        VendorTimeArithmeticVector(
            "interval-subtract",
            "interval",
            "subtract",
            "interval",
            "vendor-interval-subtract",
        ),
        VendorTimeArithmeticVector(
            "interval-compare",
            "interval",
            "compareTo",
            "interval",
            "vendor-interval-compare",
        ),
    )


def iter_vendor_data_element_wire_matrix() -> tuple[VendorDataElementWireVector, ...]:
    """Return exact, truncated, empty, and overlong opaque-provider probes.

    The payload length is a property of the fake/vendor carrier, not an IEEE
    encoding rule.  The matrix therefore checks only that the Python façade
    preserves the carrier's accept/reject behavior and does not reinterpret
    its bytes as a standard HLA data element.
    """

    return (
        VendorDataElementWireVector("exact", b"wxyz", True),
        VendorDataElementWireVector("empty", b"", False),
        VendorDataElementWireVector("truncated", b"wxy", False),
        VendorDataElementWireVector("overlong", b"wxyz!", False),
    )


def iter_vendor_data_element_encode_matrix() -> tuple[VendorDataElementEncodeVector, ...]:
    """Return exact, offset, and undersized destination-window probes.

    The four-octet carrier is intentionally provider-owned.  These vectors
    therefore assert only the standard ``DataElement.encode(ByteWrapper)``
    transport behavior: a successful write advances by four octets, while an
    undersized window raises the edition's encoder exception without a
    partial write.
    """

    return (
        VendorDataElementEncodeVector(
            "exact-offset-trailing", b"\0" * 6, 1, 5, True
        ),
        VendorDataElementEncodeVector("exact-origin", b"\0" * 4, 0, 4, True),
        VendorDataElementEncodeVector(
            "truncated-offset", b"\0" * 6, 1, 3, False
        ),
        VendorDataElementEncodeVector("empty-offset", b"\0" * 4, 1, 0, False),
    )


__all__ = [
    "ExtendableVariantWireVector",
    "VendorDataElementWireVector",
    "VendorDataElementEncodeVector",
    "VendorTimeArithmeticVector",
    "iter_extendable_variant_wire_matrix",
    "iter_vendor_data_element_wire_matrix",
    "iter_vendor_data_element_encode_matrix",
    "iter_vendor_time_arithmetic_matrix",
]
