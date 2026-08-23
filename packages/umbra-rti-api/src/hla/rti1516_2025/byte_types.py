"""Binary buffer types used at the provider boundary.

``BytesLike`` describes the common Python forms callers normally have for a
read-only RTI payload. ``copy_bytes`` deliberately goes through ``memoryview``
so other buffer-protocol providers can be copied without leaking a borrowed
view across a provider boundary.
"""

from __future__ import annotations

from typing import TypeAlias

BytesLike: TypeAlias = bytes | bytearray | memoryview
WritableBytes: TypeAlias = bytearray | memoryview


def copy_bytes(value: BytesLike, *, name: str = "value") -> bytes:
    """Copy a readable buffer into immutable Python bytes."""

    try:
        return memoryview(value).tobytes()
    except TypeError as error:
        raise TypeError(f"{name} must support the buffer protocol") from error
