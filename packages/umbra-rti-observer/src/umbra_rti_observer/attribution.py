"""Explicit participant joins and unresolved legacy reporting perspectives.

This is observation provenance, never provider authority or a membership census.
Only the root named attribution record identifies the ordinary event subject.
"""

# pyright: strict
from __future__ import annotations

import json
from collections.abc import Mapping
from dataclasses import dataclass
from typing import cast

from .identities import (
    ParticipantIdentity,
    identity_text,
    observation_scope,
    participant_identity,
)
from .models import JsonObject, to_json_object


def _label(value: object) -> str | None:
    if isinstance(value, str):
        return value.strip() or None
    if isinstance(value, (int, float)) and not isinstance(value, bool) and abs(value) <= 9007199254740991 and value == int(value):
        return str(int(value))
    return None


def reported_federate(event: Mapping[str, object]) -> str | None:
    value = event.get("federate")
    if value is None:
        value = event.get("participant")
    if isinstance(value, Mapping):
        values = cast(Mapping[str, object], value)
        for field in ("name", "id", "handle"):
            if result := _label(values.get(field)):
                return result
    elif result := _label(value):
        return result
    return _label(event.get("federate_name")) or _label(event.get("participant_name"))


def _observation_session(event: Mapping[str, object], field: str) -> object:
    raw = event.get(field)
    return cast(Mapping[str, object], raw).get("session_id") if isinstance(raw, Mapping) else None


def _session(value: object) -> str | None:
    return cast(str, identity_text(value, limit=256)) if value is not None else None


@dataclass(frozen=True, slots=True)
class ParticipantAttribution:
    session_id: str
    identity_namespace: str
    federation_id: str
    participant: ParticipantIdentity

    def to_json(self) -> JsonObject:
        return {"session_id": self.session_id, "identity_namespace": self.identity_namespace,
                "federation_id": self.federation_id, "participant": self.participant.to_json()}


def participant_attribution(event: Mapping[str, object]) -> ParticipantAttribution | None:
    sessions = tuple(_session(value) for value in (event.get("session_id"), _observation_session(event, "time_observation"),
        _observation_session(event, "declaration")) if value is not None)
    if len(set(sessions)) > 1:
        raise ValueError("normalized reporting session IDs must agree within an event")
    raw = event.get("participant_attribution")
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("participant_attribution must be a named attribution record")
    values = cast(Mapping[str, object], raw)
    if set(values) - {"session_id", "identity_namespace", "federation_id", "participant"}:
        raise ValueError("unknown participant attribution fields")
    scope = observation_scope(values)
    participant = participant_identity(values.get("participant"))
    if any(value != scope.session_id for value in sessions):
        raise ValueError("participant attribution and reported session IDs must agree")
    return ParticipantAttribution(scope.session_id, scope.identity_namespace, scope.federation_id, participant)


@dataclass(frozen=True, slots=True, eq=False)
class ParticipantPerspective:
    source: str
    session_id: str | None
    federation: str | None
    reported_name: str | None
    attribution: ParticipantAttribution | None

    @property
    def basis(self) -> str:
        return "explicit-join" if self.attribution else "legacy-provenance"

    @property
    def key(self) -> str:
        identity = self.attribution
        values = (["explicit-join", self.source, identity.session_id, identity.identity_namespace,
                   identity.federation_id, identity.participant.participant_id, identity.participant.membership_id]
                  if identity else ["legacy-provenance", self.source, self.session_id, self.federation, self.reported_name])
        return json.dumps(values, ensure_ascii=False, separators=(",", ":"))

    @property
    def label(self) -> str:
        return (self.attribution.participant.display_name if self.attribution else None) or self.reported_name or "Unnamed participant"

    def __hash__(self) -> int:
        return hash(self.key)

    def __eq__(self, other: object) -> bool:
        return isinstance(other, ParticipantPerspective) and self.key == other.key

    def to_json(self) -> JsonObject:
        return to_json_object({"key": self.key, "basis": self.basis, "source": self.source,
            "session_id": self.session_id, "federation": self.federation, "reported_name": self.reported_name,
            "attribution": self.attribution.to_json() if self.attribution else None})


def event_perspective(event: Mapping[str, object]) -> ParticipantPerspective | None:
    identity = participant_attribution(event)
    name = reported_federate(event)
    if identity is None and name is None:
        return None
    source = _label(event.get("source")) or _label(event.get("connection")) or "unknown"
    session = identity.session_id if identity else (_session(_observation_session(event, "time_observation"))
        or _session(_observation_session(event, "declaration")) or _session(event.get("session_id")))
    return ParticipantPerspective(source, session, _label(event.get("federation")), name, identity)


def event_federate(event: Mapping[str, object]) -> str | None:
    perspective = event_perspective(event)
    return perspective.label if perspective else None


def perspective_ended(perspective: ParticipantPerspective, event: Mapping[str, object]) -> bool:
    kind = event.get("event_type") or event.get("kind")
    subject = event_perspective(event)
    if kind == "federate.resigned":
        return subject is not None and subject == perspective
    if kind != "session.disconnected" or perspective.source != (_label(event.get("source")) or "unknown"):
        return False
    reported = participant_attribution(event)
    session = reported.session_id if reported else _session(event.get("session_id"))
    if perspective.session_id is None or perspective.session_id != session:
        return False
    identity = perspective.attribution
    if identity is not None:
        return reported is not None and (identity.identity_namespace, identity.federation_id) == (reported.identity_namespace, reported.federation_id)
    return reported is None and perspective.federation == _label(event.get("federation"))
