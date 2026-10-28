"""Count-bounded collections sharing a serialized-evidence LRU budget.

Callers own synchronization. This accounts encoded JSON plus a small per-record
reserve, not allocator/RSS. Eviction removes whole records, never their values.
No provider, web or model-validation dependency is introduced into the core.
"""

# pyright: strict
from __future__ import annotations

import json
from collections import OrderedDict
from collections.abc import Callable, Hashable, Iterator, MutableMapping
from dataclasses import dataclass
from typing import Generic, Literal, TypeVar

from .models import JsonObject, JsonValue, to_json_object

MAX_EVIDENCE_BYTES = 6 * 1024 * 1024
DEFAULT_RECORD_BYTES = 1024 * 1024
DropReason = Literal["count", "bytes", "oversized", "encoding", "window"]
K = TypeVar("K", bound=Hashable)
R = TypeVar("R")


def positive_limit(name: str, value: object, upper: int | None = None) -> int:
    if (
        isinstance(value, bool)
        or not isinstance(value, int)
        or value < 1
        or upper is not None
        and value > upper
    ):
        raise ValueError(
            f"{name} must be a positive integer"
            + (f" at most {upper}" if upper is not None else "")
        )
    return value


@dataclass(frozen=True, slots=True)
class BudgetEntry:
    size: int
    serial: int
    expire: Callable[[DropReason], None]


class EvidenceBudget:
    def __init__(
        self, max_bytes: int = MAX_EVIDENCE_BYTES, max_record_bytes: int | None = None
    ) -> None:
        self.max_bytes = positive_limit(
            "max_evidence_bytes", max_bytes, MAX_EVIDENCE_BYTES
        )
        self.max_record_bytes = positive_limit(
            "max_evidence_record_bytes",
            min(DEFAULT_RECORD_BYTES, self.max_bytes)
            if max_record_bytes is None
            else max_record_bytes,
            self.max_bytes,
        )
        self.used_bytes = 0
        self._serial = 0
        self._entries: OrderedDict[tuple[str, Hashable], BudgetEntry] = OrderedDict()
        self._collections: dict[str, Callable[[], JsonObject]] = {}
        self._groups: dict[str, OrderedDict[Hashable, BudgetEntry]] = {}
        self._group_bytes: dict[str, int] = {}
        self._reservations: dict[str, int] = {}

    def register(self, name: str, coverage: Callable[[], JsonObject]) -> None:
        if name in self._collections:
            raise ValueError("evidence collection names must be unique within a budget")
        self._collections[name] = coverage
        self._groups[name] = OrderedDict()
        self._group_bytes[name] = 0
        # Soft floors preserve diversity, not empty space. They can be breached
        # when whole records cannot collectively fit. No family is inferred.
        preferred = {
            "events": 512 * 1024,
            "object_snapshots": 384 * 1024,
            "object_changes": 256 * 1024,
            "objects": 256 * 1024,
            "federates": 64 * 1024,
            "time_snapshots": 64 * 1024,
            "time_requests": 64 * 1024,
            "interactions": 64 * 1024,
            "fom_snapshots": 256 * 1024,
            "mom_snapshots": 128 * 1024,
            "regions_snapshots": 128 * 1024,
            "coordination_snapshots": 128 * 1024,
            "declarations": 128 * 1024,
            "relationships": 128 * 1024,
            "interaction_flow": 128 * 1024,
            "ownership_states": 128 * 1024,
            "ownership_requests": 64 * 1024,
        }
        self._reservations[name] = min(preferred.get(name, 0), self.max_bytes // 4)

    def reservation(self, name: str) -> int:
        return self._reservations[name]

    def forget(self, name: str, key: Hashable) -> None:
        entry = self._entries.pop((name, key), None)
        if entry is not None:
            self.used_bytes -= entry.size
            self._groups[name].pop(key, None)
            self._group_bytes[name] -= entry.size

    def note(
        self, name: str, key: Hashable, size: int, expire: Callable[[DropReason], None]
    ) -> None:
        self.forget(name, key)
        if size > self.max_record_bytes:
            expire("oversized")
            return
        self._serial += 1
        entry = BudgetEntry(size, self._serial, expire)
        self._entries[(name, key)] = entry
        self._groups[name][key] = entry
        self._group_bytes[name] += size
        self.used_bytes += size
        while self.used_bytes > self.max_bytes:
            eligible: list[tuple[int, str, Hashable, BudgetEntry]] = []
            for group, entries in self._groups.items():
                if entries:
                    first_key, first = next(iter(entries.items()))
                    if (
                        self._group_bytes[group] - first.size
                        >= self._reservations[group]
                    ):
                        eligible.append((first.serial, group, first_key, first))
            if eligible:
                _, group, first_key, oldest = min(eligible, key=lambda item: item[0])
            else:
                (group, first_key), oldest = next(iter(self._entries.items()))
            self.forget(group, first_key)
            oldest.expire("bytes")

    def coverage(self, *, unbudgeted: tuple[str, ...] = ()) -> JsonObject:
        return to_json_object(
            {
                "basis": "encoded-json-plus-record-reserve",
                "policy": "shared-evidence-lru-with-soft-reservations",
                "max_bytes": self.max_bytes,
                "max_record_bytes": self.max_record_bytes,
                "accounted_bytes": self.used_bytes,
                "record_reserve_bytes": 64,
                "collections": [
                    coverage() for _, coverage in sorted(self._collections.items())
                ],
                "unbudgeted_collections": list(unbudgeted),
            }
        )


class EvidenceIndex(MutableMapping[K, R], Generic[K, R]):
    """Owners must reassign/refresh after mutating a retained record in place."""

    def __init__(
        self,
        name: str,
        capacity: int,
        budget: EvidenceBudget,
        encode: Callable[[K, R], JsonValue],
        *,
        unit: str = "records",
        on_expire: Callable[[K, R], None] | None = None,
    ) -> None:
        self.name, self.capacity, self.budget, self.encode = (
            name,
            positive_limit("capacity", capacity),
            budget,
            encode,
        )
        self.unit, self.on_expire = unit, on_expire
        self._rows: OrderedDict[K, R] = OrderedDict()
        self._sizes: dict[K, int] = {}
        self._dropped: dict[DropReason, int] = {
            reason: 0
            for reason in ("count", "bytes", "oversized", "encoding", "window")
        }
        self.dropped_bytes = 0
        budget.register(name, self.coverage)

    def __getitem__(self, key: K) -> R:
        return self._rows[key]

    def __iter__(self) -> Iterator[K]:
        return iter(self._rows)

    def __len__(self) -> int:
        return len(self._rows)

    def __delitem__(self, key: K) -> None:
        del self._rows[key]
        self._sizes.pop(key, None)
        self.budget.forget(self.name, key)

    def __setitem__(self, key: K, value: R) -> None:
        self._rows[key] = value
        self._rows.move_to_end(key)
        try:
            # Keep invalid/non-UTF8 values out of retained wire evidence; ingress
            # still completes and reports the explicit encoding loss.
            size = (
                len(
                    json.dumps(
                        self.encode(key, value),
                        ensure_ascii=False,
                        allow_nan=False,
                        separators=(",", ":"),
                    ).encode("utf-8")
                )
                + 64
            )
        except (ValueError, TypeError, UnicodeError, RecursionError):
            self._expire(key, "encoding")
            return
        self._sizes[key] = size
        self.budget.note(self.name, key, size, lambda reason: self._expire(key, reason))
        while len(self._rows) > self.capacity:
            self._expire(next(iter(self._rows)), "count")

    @property
    def dropped(self) -> int:
        return sum(self._dropped.values())

    def refresh(self, key: K) -> None:
        if key in self._rows:
            self[key] = self._rows[key]

    def _expire(self, key: K, reason: DropReason) -> None:
        if key not in self._rows:
            return
        row = self._rows[key]
        self._dropped[reason] += 1
        self.dropped_bytes += self._sizes.get(key, 0)
        del self[key]
        if self.on_expire is not None:
            self.on_expire(key, row)

    def expire(self, key: K, reason: DropReason = "window") -> None:
        self._expire(key, reason)

    def clear(self) -> None:
        for key in tuple(self._rows):
            del self[key]
        self._dropped = {reason: 0 for reason in self._dropped}
        self.dropped_bytes = 0

    def coverage(self) -> JsonObject:
        return to_json_object(
            {
                "collection": self.name,
                "unit": self.unit,
                "capacity": self.capacity,
                "retained_records": len(self),
                "accounted_bytes": sum(self._sizes.values()),
                "soft_reservation_bytes": self.budget.reservation(self.name),
                "dropped_records": self.dropped,
                "dropped_bytes": self.dropped_bytes,
                "count_limit_drops": self._dropped["count"],
                "byte_limit_drops": self._dropped["bytes"],
                "oversized_drops": self._dropped["oversized"],
                "encoding_drops": self._dropped["encoding"],
                "window_drops": self._dropped["window"],
            }
        )


class EvidenceQueue(Generic[R]):
    def __init__(
        self,
        name: str,
        capacity: int,
        budget: EvidenceBudget,
        encode: Callable[[R], JsonValue],
        *,
        unit: str = "records",
        contiguous: bool = False,
    ) -> None:
        self.capacity, self.contiguous, self._next = capacity, contiguous, 0
        self._index = EvidenceIndex[int, R](
            name, capacity, budget, lambda _, row: encode(row), unit=unit
        )
        if contiguous:
            self._index.on_expire = self._expire_prefix

    def _expire_prefix(self, key: int, _: R) -> None:
        while self._index:
            earlier = next(iter(self._index))
            if earlier >= key:
                break
            self._index.expire(earlier, "window")

    def append(self, row: R) -> None:
        self._next += 1
        self._index[self._next] = row

    def __iter__(self) -> Iterator[R]:
        return iter(self._index.values())

    def __len__(self) -> int:
        return len(self._index)

    def __getitem__(self, index: int) -> R:
        return tuple(self._index.values())[index]

    @property
    def dropped(self) -> int:
        return self._index.dropped

    def trim(self, keep: Callable[[R], bool]) -> None:
        for key, row in tuple(self._index.items()):
            if not keep(row):
                self._index.expire(key, "window")

    def clear(self) -> None:
        self._index.clear()

    def extend(self, rows: Iterator[R] | list[R]) -> None:
        for row in rows:
            self.append(row)
