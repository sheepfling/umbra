"""Implemented credential values for the 2025 connection foundation."""

from __future__ import annotations

from ..byte_types import BytesLike, copy_bytes

class Credentials:
    """Immutable Python representation of the Java ``Credentials`` value."""

    def __init__(self, credentialType: str, data: BytesLike) -> None:
        self._type = str(credentialType)
        self._data = copy_bytes(data, name="data")

    def getType(self) -> str:
        return self._type

    def getData(self) -> bytes:
        return self._data


class HLAnoCredentials(Credentials):
    HLA_NO_CREDENTIALS_TYPE = "HLAnoCredentials"

    def __init__(self) -> None:
        super().__init__(self.HLA_NO_CREDENTIALS_TYPE, b"")


__all__ = ["Credentials", "HLAnoCredentials"]
