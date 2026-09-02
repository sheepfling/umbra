"""Binary buffer aliases for the IEEE 1516.1-2010 Python boundary."""

from __future__ import annotations

from typing import TypeAlias

BytesLike: TypeAlias = bytes | bytearray | memoryview
WritableBytes: TypeAlias = bytearray | memoryview


def copy_bytes(value: BytesLike, *, name: str = "value") -> bytes:
    """Copy a readable buffer before it crosses a provider boundary."""

    try:
        return memoryview(value).tobytes()
    except TypeError as error:
        raise TypeError(f"{name} must support the buffer protocol") from error


__all__ = ["BytesLike", "WritableBytes", "copy_bytes"]
