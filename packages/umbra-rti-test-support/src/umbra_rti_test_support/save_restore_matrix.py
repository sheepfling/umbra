"""Provider-neutral save/restore outcome state-space helpers.

The standard API exposes three federate save outcomes and three federate
restore outcomes.  This module describes their Cartesian product together
with the callback, time, save-overload, advance-service, and member-count
dimensions used by the provider tests.  It contains no provider behavior;
Java/JPype, JNI, and native tests translate the service names to their own
surfaces and assert the resulting calls.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
from itertools import product

from .surface_matrix import (
    DEFAULT_ADVANCE_SERVICES,
    DEFAULT_CALLBACK_MODELS,
    DEFAULT_SAVE_KINDS,
    DEFAULT_TIME_IMPLEMENTATIONS,
)

DEFAULT_SAVE_OUTCOMES = ("complete", "not-complete", "abort")
DEFAULT_RESTORE_OUTCOMES = ("complete", "not-complete", "abort")

_SAVE_SERVICES = {
    "complete": "federateSaveComplete",
    "not-complete": "federateSaveNotComplete",
    "abort": "abortFederationSave",
}
_RESTORE_SERVICES = {
    "complete": "federateRestoreComplete",
    "not-complete": "federateRestoreNotComplete",
    "abort": "abortFederationRestore",
}


@dataclass(frozen=True, slots=True)
class SaveRestoreMatrixCase:
    """One provider-neutral save/restore outcome state-space point."""

    callback_model: str
    time_implementation: str
    save_kind: str
    advance_service: str
    save_outcome: str
    restore_outcome: str
    member_count: int

    @property
    def save_service(self) -> str:
        """Return the standard federate save service for this outcome."""

        return _SAVE_SERVICES[self.save_outcome]

    @property
    def restore_service(self) -> str:
        """Return the standard federate restore service for this outcome."""

        return _RESTORE_SERVICES[self.restore_outcome]

    @property
    def case_id(self) -> str:
        """Return a stable id suitable for subtests and reports."""

        return ":".join(
            (
                self.callback_model,
                self.time_implementation,
                self.save_kind,
                self.advance_service,
                f"save={self.save_outcome}",
                f"restore={self.restore_outcome}",
                f"members={self.member_count}",
            )
        )


def iter_save_restore_matrix(
    *,
    callback_models: Iterable[str] = DEFAULT_CALLBACK_MODELS,
    time_implementations: Iterable[str] = DEFAULT_TIME_IMPLEMENTATIONS,
    save_kinds: Iterable[str] = DEFAULT_SAVE_KINDS,
    advance_services: Iterable[str] = DEFAULT_ADVANCE_SERVICES,
    save_outcomes: Iterable[str] = DEFAULT_SAVE_OUTCOMES,
    restore_outcomes: Iterable[str] = DEFAULT_RESTORE_OUTCOMES,
    member_counts: Iterable[int] = (1, 2),
) -> tuple[SaveRestoreMatrixCase, ...]:
    """Build a deterministic, duplicate-free save/restore matrix.

    The defaults produce 720 cases (two callback models, two time carriers,
    two save overloads, five advance services, three save outcomes, three
    restore outcomes, and one/two members).  Inputs are copied before
    Cartesian expansion so generators are safe to pass from a transplanted
    provider test.
    """

    dimensions = (
        tuple(callback_models),
        tuple(time_implementations),
        tuple(save_kinds),
        tuple(advance_services),
        tuple(save_outcomes),
        tuple(restore_outcomes),
        tuple(member_counts),
    )
    if any(not values for values in dimensions):
        raise ValueError("save/restore matrix dimensions must not be empty")
    if any(
        not isinstance(value, str) or not value
        for values in dimensions[:6]
        for value in values
    ):
        raise ValueError("save/restore matrix labels must be non-empty strings")
    if any(value not in _SAVE_SERVICES for value in dimensions[4]):
        raise ValueError("save outcomes must be complete, not-complete, or abort")
    if any(value not in _RESTORE_SERVICES for value in dimensions[5]):
        raise ValueError("restore outcomes must be complete, not-complete, or abort")
    if any(
        isinstance(value, bool) or not isinstance(value, int) or value < 1
        for value in dimensions[6]
    ):
        raise ValueError("save/restore matrix member counts must be positive integers")
    cases = tuple(SaveRestoreMatrixCase(*values) for values in product(*dimensions))
    if len({case.case_id for case in cases}) != len(cases):
        raise ValueError("save/restore matrix dimensions produce duplicate case ids")
    return cases


__all__ = [
    "DEFAULT_RESTORE_OUTCOMES",
    "DEFAULT_SAVE_OUTCOMES",
    "SaveRestoreMatrixCase",
    "iter_save_restore_matrix",
]
