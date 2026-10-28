"""Opaque adapter identities shared by explicit observation models."""

# pyright: strict
from __future__ import annotations

import math
from collections.abc import Mapping
from dataclasses import dataclass
from typing import cast

from .models import (
    JsonObject,
    RawParticipantIdentity,
    to_json_object,
)


def identity_text(
    value: object, *, optional: bool = False, limit: int = 512
) -> str | None:
    if value is None and optional:
        return None
    if isinstance(value, str) and value.strip() and len(value) <= limit:
        value.encode("utf-8")  # Opaque identifiers must still be valid JSON text.
        return value
    raise ValueError("observation identifiers must be bounded nonblank strings")


def observation_reason(value: object) -> str | None:
    if value is None or isinstance(value, str) and len(value) <= 4096:
        return value
    raise ValueError("observation reason must be at most 4096 text characters")


def finite_observation_number(value: object) -> int | float | None:
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError("numeric observations must be finite JSON numbers")  # noqa: TRY004 -- Pydantic must return a validation error, not HTTP 500
    try:
        finite = math.isfinite(value)
    except OverflowError:
        finite = False
    if not finite:
        raise ValueError("numeric observations must be finite")
    return value


def identifier_array(value: object) -> tuple[str, ...] | None:
    if value is None:
        return None
    if not isinstance(value, (list, tuple)):
        raise TypeError("identifier collection must be an explicit array or unknown")
    items = cast(list[object] | tuple[object, ...], value)
    if len(items) > 4096:
        raise ValueError("identifier arrays contain at most 4096 entries")
    result = tuple(cast(str, identity_text(item)) for item in items)
    if len(set(result)) != len(result):
        raise ValueError("identifier arrays must not contain duplicates")
    return result


def decimal_bound(value: object, *, optional: bool = False) -> str | None:
    if value is None and optional:
        return None
    if (
        isinstance(value, str)
        and 1 <= len(value) <= 128
        and value.isascii()
        and value.isdigit()
        and (value == "0" or not value.startswith("0"))
    ):
        return value
    raise ValueError(
        "range bounds must be canonical nonnegative decimal strings of at most 128 digits"
    )


@dataclass(frozen=True, slots=True)
class ObservationScope:
    session_id: str
    identity_namespace: str
    federation_id: str

    def key(self, source: str) -> tuple[str, str, str, str]:
        return source, self.session_id, self.identity_namespace, self.federation_id

    def to_json(self) -> JsonObject:
        return {
            "session_id": self.session_id,
            "identity_namespace": self.identity_namespace,
            "federation_id": self.federation_id,
        }


def observation_scope_from_fields(
    session_id: object, identity_namespace: object, federation_id: object
) -> ObservationScope:
    """Build a validated observation scope from its explicit fields."""
    return ObservationScope(
        cast(str, identity_text(session_id, limit=256)),
        cast(str, identity_text(identity_namespace)),
        cast(str, identity_text(federation_id)),
    )


# opaque-boundary: Validate raw participant identity values from provider events.
def observation_scope(values: Mapping[str, object]) -> ObservationScope:
    """Validate the named scope fields in a raw participant-attribution record."""
    return observation_scope_from_fields(
        values.get("session_id"),
        values.get("identity_namespace"),
        values.get("federation_id"),
    )


@dataclass(frozen=True, slots=True)
class ParticipantIdentity:
    participant_id: str
    membership_id: str
    display_name: str | None
    handle: str | None

    def to_json(self) -> JsonObject:
        return to_json_object(
            {
                "participant_id": self.participant_id,
                "membership_id": self.membership_id,
                "display_name": self.display_name,
                "handle": self.handle,
            }
        )


def participant_identity_from_fields(
    participant_id: object,
    membership_id: object,
    display_name: object = None,
    handle: object = None,
) -> ParticipantIdentity:
    """Build a validated identity from fields already exposed by a named model."""
    return ParticipantIdentity(
        cast(str, identity_text(participant_id)),
        cast(str, identity_text(membership_id)),
        identity_text(display_name, optional=True),
        identity_text(handle, optional=True),
    )


def participant_identity(raw: object) -> ParticipantIdentity:
    if not isinstance(raw, Mapping):
        raise TypeError(
            "participant identity requires explicit participant and membership IDs"
        )
    values = cast(RawParticipantIdentity, raw)
    if set(values) - {"participant_id", "membership_id", "display_name", "handle"}:
        raise ValueError("unknown participant identity fields")
    return participant_identity_from_fields(
        values.get("participant_id"),
        values.get("membership_id"),
        values.get("display_name"),
        values.get("handle"),
    )


@dataclass(frozen=True, slots=True)
class ParticipantReference:
    identity: ParticipantIdentity | None
    display_name: str | None
    handle: str | None

    def to_json(self) -> JsonObject:
        return {
            "identity": self.identity.to_json() if self.identity else None,
            "display_name": self.display_name,
            "handle": self.handle,
        }


def participant_reference_from_fields(
    identity: ParticipantIdentity | None,
    display_name: object,
    handle: object,
) -> ParticipantReference:
    """Build a participant reference from explicit validated domain fields."""
    name = identity_text(display_name, optional=True)
    normalized_handle = identity_text(handle, optional=True)
    if identity is None and name is None and normalized_handle is None:
        raise ValueError(
            "participant reference needs explicit identity or reported name/handle metadata"
        )
    return ParticipantReference(identity, name, normalized_handle)


def participant_reference(raw: object) -> ParticipantReference:
    if not isinstance(raw, Mapping):
        raise TypeError("participant reference must be a named record")
    values = cast(Mapping[str, object], raw)
    if set(values) - {"identity", "display_name", "handle"}:
        raise ValueError("unknown participant reference fields")
    identity = (
        participant_identity(values["identity"])
        if values.get("identity") is not None
        else None
    )
    return participant_reference_from_fields(
        identity,
        values.get("display_name"),
        values.get("handle"),
    )
