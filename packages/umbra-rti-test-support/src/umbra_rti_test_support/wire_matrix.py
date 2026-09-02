"""Provider-neutral logical-time wire and arithmetic vectors.

These vectors intentionally describe only standard carrier behavior.  A
provider test can feed them to its own factory and assert that the Python
boundary preserves values, offsets, and typed decode failures without
reimplementing a Java or C++ encoder in the test itself.
"""

from __future__ import annotations

import math
import struct
from collections.abc import Iterable
from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class LogicalTimeWireVector:
    """One fixed-width logical-time or interval encoding vector."""

    implementation: str
    label: str
    encoded: bytes
    expected: int | float | None
    valid: bool

    @property
    def case_id(self) -> str:
        return f"{self.implementation}:{self.label}"


@dataclass(frozen=True, slots=True)
class LogicalTimeArithmeticVector:
    """One provider-owned logical-time arithmetic vector."""

    implementation: str
    base: int | float
    interval: int | float
    expected_sum: int | float
    expected_difference: int | float
    expected_distance: int | float

    @property
    def case_id(self) -> str:
        return f"{self.implementation}:base={self.base}:interval={self.interval}"


def iter_logical_time_wire_matrix(
    implementations: Iterable[str] = ("HLAinteger64Time", "HLAfloat64Time"),
) -> tuple[LogicalTimeWireVector, ...]:
    """Return deterministic valid, invalid, offset, and boundary vectors."""

    vectors: list[LogicalTimeWireVector] = []
    for implementation in implementations:
        if implementation == "HLAinteger64Time":
            values = (
                ("zero", struct.pack(">q", 0), 0, True),
                ("one", struct.pack(">q", 1), 1, True),
                ("large-exact", struct.pack(">q", 2**53 + 1), 2**53 + 1, True),
                ("finite-final", struct.pack(">q", 2**63 - 1), 2**63 - 1, True),
                ("negative", struct.pack(">q", -1), None, False),
                ("truncated", b"\x00" * 7, None, False),
                ("trailing", b"\x00" * 9, None, False),
            )
        elif implementation == "HLAfloat64Time":
            final = float.fromhex("0x1.fffffffffffffp+1023")
            values = (
                ("zero", struct.pack(">d", 0.0), 0.0, True),
                ("signed-zero", struct.pack(">d", -0.0), 0.0, True),
                ("one", struct.pack(">d", 1.0), 1.0, True),
                ("finite-final", struct.pack(">d", final), final, True),
                ("negative", struct.pack(">d", -1.0), None, False),
                ("positive-infinity", struct.pack(">d", math.inf), None, False),
                ("not-a-number", struct.pack(">d", math.nan), None, False),
                ("truncated", b"\x00" * 7, None, False),
                ("trailing", b"\x00" * 9, None, False),
            )
        else:
            raise ValueError(
                f"unsupported logical-time implementation: {implementation}"
            )
        vectors.extend(
            LogicalTimeWireVector(implementation, label, encoded, expected, valid)
            for label, encoded, expected, valid in values
        )
    return tuple(vectors)


def iter_logical_time_arithmetic_matrix(
    implementations: Iterable[str] = ("HLAinteger64Time", "HLAfloat64Time"),
) -> tuple[LogicalTimeArithmeticVector, ...]:
    """Return exact and precision-boundary arithmetic vectors."""

    vectors: list[LogicalTimeArithmeticVector] = []
    for implementation in implementations:
        if implementation == "HLAinteger64Time":
            vectors.extend(
                (
                    LogicalTimeArithmeticVector(
                        implementation, 2**53 + 1, 7, 2**53 + 8, 2**53 - 6, 7
                    ),
                    LogicalTimeArithmeticVector(implementation, 0, 0, 0, 0, 0),
                )
            )
        elif implementation == "HLAfloat64Time":
            epsilon = math.nextafter(0.0, 1.0)
            vectors.extend(
                (
                    LogicalTimeArithmeticVector(
                        implementation,
                        1.0,
                        epsilon,
                        math.nextafter(1.0, math.inf),
                        math.nextafter(1.0, 0.0),
                        math.nextafter(1.0, math.inf) - 1.0,
                    ),
                    LogicalTimeArithmeticVector(
                        implementation, 0.0, 0.0, 0.0, 0.0, 0.0
                    ),
                )
            )
        else:
            raise ValueError(
                f"unsupported logical-time implementation: {implementation}"
            )
    return tuple(vectors)


__all__ = [
    "LogicalTimeArithmeticVector",
    "LogicalTimeWireVector",
    "iter_logical_time_arithmetic_matrix",
    "iter_logical_time_wire_matrix",
]
