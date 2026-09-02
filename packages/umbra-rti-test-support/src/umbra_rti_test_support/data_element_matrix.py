"""Provider-neutral basic data-element value vectors.

The vectors contain values only; they intentionally do not contain encoded
octets.  Each provider must construct the value through its own standard
``EncoderFactory`` and is responsible for the wire representation.  This
keeps the native and Java/JPype tests on one edge-value contract without
duplicating (and potentially drifting from) an encoder implementation.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

DataElementEdition = Literal["2010", "2025"]


@dataclass(frozen=True, slots=True)
class DataElementValueVector:
    """One standard basic data-element value to construct and round-trip."""

    kind: str
    label: str
    value: object

    @property
    def case_id(self) -> str:
        return f"{self.kind}:{self.label}"


_COMMON_VALUES: tuple[tuple[str, tuple[tuple[str, object], ...]], ...] = (
    (
        "HLAinteger16BE",
        (("minimum", -(2**15)), ("negative", -1), ("zero", 0), ("maximum", 2**15 - 1)),
    ),
    (
        "HLAinteger16LE",
        (("minimum", -(2**15)), ("negative", -1), ("zero", 0), ("maximum", 2**15 - 1)),
    ),
    (
        "HLAinteger32BE",
        (("minimum", -(2**31)), ("negative", -1), ("zero", 0), ("maximum", 2**31 - 1)),
    ),
    (
        "HLAinteger32LE",
        (("minimum", -(2**31)), ("negative", -1), ("zero", 0), ("maximum", 2**31 - 1)),
    ),
    (
        "HLAinteger64BE",
        (("minimum", -(2**63)), ("negative", -1), ("zero", 0), ("maximum", 2**63 - 1)),
    ),
    (
        "HLAinteger64LE",
        (("minimum", -(2**63)), ("negative", -1), ("zero", 0), ("maximum", 2**63 - 1)),
    ),
    (
        "HLAfloat32BE",
        (
            ("negative-zero", -0.0),
            ("zero", 0.0),
            ("finite", 1.25),
            ("maximum-finite", float.fromhex("0x1.fffffep+127")),
        ),
    ),
    (
        "HLAfloat32LE",
        (
            ("negative-zero", -0.0),
            ("zero", 0.0),
            ("finite", 1.25),
            ("maximum-finite", float.fromhex("0x1.fffffep+127")),
        ),
    ),
    (
        "HLAfloat64BE",
        (
            ("negative-zero", -0.0),
            ("zero", 0.0),
            ("finite", 1.25),
            ("maximum-finite", float.fromhex("0x1.fffffffffffffp+1023")),
        ),
    ),
    (
        "HLAfloat64LE",
        (
            ("negative-zero", -0.0),
            ("zero", 0.0),
            ("finite", 1.25),
            ("maximum-finite", float.fromhex("0x1.fffffffffffffp+1023")),
        ),
    ),
    (
        "HLAbyte",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("high-positive", 0x7F),
            ("high-bit", 0x80),
            ("maximum", 0xFF),
        ),
    ),
    (
        "HLAoctet",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("high-positive", 0x7F),
            ("high-bit", 0x80),
            ("maximum", 0xFF),
        ),
    ),
    (
        "HLAASCIIchar",
        (("zero", 0), ("letter", ord("A")), ("high-ascii", 0x7F)),
    ),
    (
        "HLAunicodeChar",
        (
            ("zero", 0),
            ("greek", 0x03A9),
            ("signed-boundary", 0x7FFF),
            ("maximum", 0xFFFF),
        ),
    ),
    (
        "HLAoctetPairBE",
        (
            ("zero", 0),
            ("middle", 0x1234),
            ("signed-boundary", 0x7FFF),
            ("maximum", 0xFFFF),
        ),
    ),
    (
        "HLAoctetPairLE",
        (
            ("zero", 0),
            ("middle", 0x1234),
            ("signed-boundary", 0x7FFF),
            ("maximum", 0xFFFF),
        ),
    ),
    (
        "HLAboolean",
        (("false", False), ("true", True)),
    ),
    (
        "HLAASCIIstring",
        (
            ("empty", ""),
            ("single", "A"),
            ("element-count-boundary", "A" * 127),
            ("long-count", "~" * 255),
        ),
    ),
    (
        "HLAunicodeString",
        (
            ("empty", ""),
            ("unicode", "Ω"),
            ("surrogate-pair", "A😀"),
            ("maximum-code-unit", "\uffff"),
        ),
    ),
    (
        "HLAopaqueData",
        (
            ("empty", b""),
            ("single", b"\x00"),
            ("mixed-octets", b"\x00\x7f\x80\xff"),
            ("long-count", bytes(range(256))),
        ),
    ),
)


_UNSIGNED_VALUES: tuple[tuple[str, tuple[tuple[str, object], ...]], ...] = (
    (
        "HLAunsignedInteger16BE",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("signed-boundary", 0x7FFF),
            ("maximum", 0xFFFF),
        ),
    ),
    (
        "HLAunsignedInteger16LE",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("signed-boundary", 0x7FFF),
            ("maximum", 0xFFFF),
        ),
    ),
    (
        "HLAunsignedInteger32BE",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("signed-boundary", 0x7FFFFFFF),
            ("maximum", 0xFFFFFFFF),
        ),
    ),
    (
        "HLAunsignedInteger32LE",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("signed-boundary", 0x7FFFFFFF),
            ("maximum", 0xFFFFFFFF),
        ),
    ),
    (
        "HLAunsignedInteger64BE",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("signed-boundary", 0x7FFFFFFFFFFFFFFF),
            ("maximum", 0xFFFFFFFFFFFFFFFF),
        ),
    ),
    (
        "HLAunsignedInteger64LE",
        (
            ("zero", 0),
            ("low-positive", 1),
            ("signed-boundary", 0x7FFFFFFFFFFFFFFF),
            ("maximum", 0xFFFFFFFFFFFFFFFF),
        ),
    ),
)


def iter_data_element_value_matrix(
    edition: DataElementEdition = "2010",
) -> tuple[DataElementValueVector, ...]:
    """Return deterministic edge values for the requested standard edition.

    IEEE 1516e-2010 has the common basic family.  IEEE 1516.1-2025 adds the
    six unsigned integer carriers; requesting an unknown edition is an input
    error so a test cannot silently exercise the wrong surface.
    """

    if edition not in {"2010", "2025"}:
        raise ValueError(f"unsupported data-element edition: {edition}")
    groups = (
        _COMMON_VALUES if edition == "2010" else (*_COMMON_VALUES, *_UNSIGNED_VALUES)
    )
    return tuple(
        DataElementValueVector(kind, label, value)
        for kind, values in groups
        for label, value in values
    )


__all__ = [
    "DataElementEdition",
    "DataElementValueVector",
    "iter_data_element_value_matrix",
]
