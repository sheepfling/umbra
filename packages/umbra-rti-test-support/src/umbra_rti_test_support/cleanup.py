"""Helpers for keeping provider-test cleanup failures visible."""

from __future__ import annotations

import logging
from collections.abc import Iterator
from contextlib import contextmanager

_LOGGER = logging.getLogger(__name__)


@contextmanager
def best_effort_cleanup(operation: str) -> Iterator[None]:
    """Log and suppress teardown errors so they do not mask test failures."""

    try:
        yield
    except Exception:
        _LOGGER.debug(
            "Best-effort provider-test cleanup failed during %s",
            operation,
            exc_info=True,
        )
