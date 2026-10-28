"""Typed latest-report and arrival-history storage, used under the store lock."""

# pyright: strict
from __future__ import annotations

from collections.abc import Hashable
from typing import Generic, Protocol, TypeVar

from .models import JsonObject
from .storage import EvidenceBudget, EvidenceIndex, EvidenceQueue

K = TypeVar("K", bound=Hashable)


class EncodedReport(Protocol):
    def to_json(self) -> JsonObject: ...


R = TypeVar("R", bound=EncodedReport)


def _capacity(name: str, value: object) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 1:
        raise ValueError(f"{name} must be a positive integer")
    return value


class LatestHistory(Generic[K, R]):
    def __init__(
        self,
        capacity: int,
        history_capacity: int,
        *,
        name: str,
        budget: EvidenceBudget | None = None,
    ) -> None:
        self.capacity = _capacity("capacity", capacity)
        self.history_capacity = _capacity("history_capacity", history_capacity)
        shared = budget if budget is not None else EvidenceBudget()
        self._latest = EvidenceIndex[K, R](
            name + "_snapshots", capacity, shared, lambda _, row: row.to_json()
        )
        self._history = EvidenceQueue[R](
            name + "_history", history_capacity, shared, lambda row: row.to_json()
        )

    @property
    def dropped_snapshots(self) -> int:
        return self._latest.dropped

    @property
    def dropped_transitions(self) -> int:
        return self._history.dropped

    @property
    def snapshots(self) -> tuple[R, ...]:
        return tuple(self._latest.values())

    @property
    def transitions(self) -> tuple[R, ...]:
        return tuple(self._history)

    def record(self, key: K, row: R) -> None:
        self._latest[key] = row
        self._history.append(row)

    def clear(self) -> None:
        self._latest.clear()
        self._history.clear()
