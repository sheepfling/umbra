"""Explicit participant joins and unresolved legacy reporting perspectives.

This is observation provenance, never provider authority or a membership census.
Only the root named attribution record identifies the ordinary event subject.
"""

# pyright: strict
from __future__ import annotations

import json
from collections.abc import Mapping
from dataclasses import dataclass
from typing import Literal, cast

from .identities import (
    ParticipantIdentity,
    identity_text,
    observation_scope,
    participant_identity,
    participant_identity_from_fields,
)
from .models import (
    EventRecord,
    JsonObject,
    RawObservationSession,
    RawObserverEvent,
    RawParticipantAttribution,
    RawReportedParticipantMetadata,
    ReportedParticipantCandidate,
    to_json_object,
)

ObservationSessionField = Literal["time_observation", "declaration"]


def _label(value: object) -> str | None:
    if isinstance(value, str):
        return value.strip() or None
    if (
        isinstance(value, (int, float))
        and not isinstance(value, bool)
        and abs(value) <= 9007199254740991
        and value == int(value)
    ):
        return str(int(value))
    return None


def reported_federate(event: RawObserverEvent) -> str | None:
    return _reported_federate(
        event.get("federate"),
        event.get("participant"),
        event.get("federate_name"),
        event.get("participant_name"),
    )


def _reported_federate(
    federate: object,
    participant: object,
    federate_name: object,
    participant_name: object,
) -> str | None:
    value = federate if federate is not None else participant
    if isinstance(value, Mapping):
        values = cast(RawReportedParticipantMetadata, value)
        for candidate in (values.get("name"), values.get("id"), values.get("handle")):
            if result := _label(candidate):
                return result
    elif result := _label(value):
        return result
    return _label(federate_name) or _label(participant_name)


# opaque-boundary: Read raw nested provider observations before validation.
def _observation_session(
    event: RawObserverEvent, field: ObservationSessionField
) -> object:
    raw = event.get(field)
    return (
        cast(RawObservationSession, raw).get("session_id")
        if isinstance(raw, Mapping)
        else None
    )


def _session(value: object) -> str | None:
    return cast(str, identity_text(value, limit=256)) if value is not None else None


@dataclass(frozen=True, slots=True)
class ParticipantAttribution:
    session_id: str
    identity_namespace: str
    federation_id: str
    participant: ParticipantIdentity

    def to_json(self) -> JsonObject:
        return {
            "session_id": self.session_id,
            "identity_namespace": self.identity_namespace,
            "federation_id": self.federation_id,
            "participant": self.participant.to_json(),
        }


def participant_attribution_from_fields(
    session_id: object,
    identity_namespace: object,
    federation_id: object,
    participant: ParticipantIdentity,
) -> ParticipantAttribution:
    """Build normalized attribution from fields on an already named record."""
    normalized_participant = participant_identity_from_fields(
        participant.participant_id,
        participant.membership_id,
        participant.display_name,
        participant.handle,
    )
    return ParticipantAttribution(
        cast(str, identity_text(session_id, limit=256)),
        cast(str, identity_text(identity_namespace)),
        cast(str, identity_text(federation_id)),
        normalized_participant,
    )


def validate_participant_sessions(
    attribution: ParticipantAttribution | None, session_values: tuple[object, ...]
) -> None:
    """Require every supplied event/session ID to agree with its attribution."""
    sessions = tuple(_session(value) for value in session_values if value is not None)
    if len(set(sessions)) > 1:
        raise ValueError("normalized reporting session IDs must agree within an event")
    if attribution is not None and any(
        value != attribution.session_id for value in sessions
    ):
        raise ValueError("participant attribution and reported session IDs must agree")


@dataclass(frozen=True, slots=True)
class ParticipantEventMetadata:
    """Validated participant provenance fields for one event.

    Participant labels may arrive as text, a finite integral JSON number, or a
    named legacy participant record. Boolean and non-finite labels are rejected.
    """

    source: str | None = None
    event_type: str | None = None
    federation: str | None = None
    session_id: str | None = None
    federate: ReportedParticipantCandidate = None
    participant: ReportedParticipantCandidate = None
    federate_name: str | None = None
    participant_name: str | None = None
    time_observation_session_id: str | None = None
    declaration_session_id: str | None = None
    attribution: ParticipantAttribution | None = None


def _participant_attribution(
    raw: object, session_values: tuple[object, ...]
) -> ParticipantAttribution | None:
    validate_participant_sessions(None, session_values)
    if raw is None:
        return None
    if not isinstance(raw, Mapping):
        raise TypeError("participant_attribution must be a named attribution record")
    values = cast(RawParticipantAttribution, raw)
    if set(values) - {
        "session_id",
        "identity_namespace",
        "federation_id",
        "participant",
    }:
        raise ValueError("unknown participant attribution fields")
    scope = observation_scope(values)
    participant = participant_identity(values.get("participant"))
    attribution = participant_attribution_from_fields(
        scope.session_id,
        scope.identity_namespace,
        scope.federation_id,
        participant,
    )
    validate_participant_sessions(attribution, session_values)
    return attribution


# opaque-boundary: Validate provider-owned attribution records at event ingress.
def participant_attribution(event: RawObserverEvent) -> ParticipantAttribution | None:
    return _participant_attribution(
        event.get("participant_attribution"),
        (
            event.get("session_id"),
            _observation_session(event, "time_observation"),
            _observation_session(event, "declaration"),
        ),
    )


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
        values = (
            [
                "explicit-join",
                self.source,
                identity.session_id,
                identity.identity_namespace,
                identity.federation_id,
                identity.participant.participant_id,
                identity.participant.membership_id,
            ]
            if identity
            else [
                "legacy-provenance",
                self.source,
                self.session_id,
                self.federation,
                self.reported_name,
            ]
        )
        return json.dumps(values, ensure_ascii=False, separators=(",", ":"))

    @property
    def label(self) -> str:
        return (
            (self.attribution.participant.display_name if self.attribution else None)
            or self.reported_name
            or "Unnamed participant"
        )

    def __hash__(self) -> int:
        return hash(self.key)

    def __eq__(self, other: object) -> bool:
        return isinstance(other, ParticipantPerspective) and self.key == other.key

    def to_json(self) -> JsonObject:
        return to_json_object(
            {
                "key": self.key,
                "basis": self.basis,
                "source": self.source,
                "session_id": self.session_id,
                "federation": self.federation,
                "reported_name": self.reported_name,
                "attribution": self.attribution.to_json() if self.attribution else None,
            }
        )


def participant_event_metadata(event: RawObserverEvent) -> ParticipantEventMetadata:
    """Parse the open provider event map into the participant-specific contract."""
    event_type = event.get("event_type")
    if event_type is None:
        event_type = event.get("kind")
    return ParticipantEventMetadata(
        source=_label(event.get("source")) or _label(event.get("connection")),
        event_type=_label(event_type),
        federation=_label(event.get("federation")),
        session_id=_session(event.get("session_id")),
        federate=_reported_candidate(event.get("federate")),
        participant=_reported_candidate(event.get("participant")),
        federate_name=_label(event.get("federate_name")),
        participant_name=_label(event.get("participant_name")),
        time_observation_session_id=_session(
            _observation_session(event, "time_observation")
        ),
        declaration_session_id=_session(_observation_session(event, "declaration")),
        attribution=participant_attribution(event),
    )


def event_participant_metadata(event: EventRecord) -> ParticipantEventMetadata:
    """Read a normalized event's JSON extensions as the named participant view."""
    return participant_event_metadata(event.extensions)


def _reported_candidate(value: object) -> ReportedParticipantCandidate:
    if (
        value is None
        or isinstance(value, (str, int, float))
        and not isinstance(value, bool)
    ):
        return cast(ReportedParticipantCandidate, value)
    if isinstance(value, Mapping):
        mapping = cast(Mapping[object, object], value)
        if any(not isinstance(key, str) for key in mapping):
            raise TypeError("participant metadata field names must be strings")
        return cast(ReportedParticipantCandidate, value)
    if isinstance(value, bool):
        raise TypeError("participant labels must not be booleans")
    raise TypeError("participant labels must be text, numbers, or named metadata")


def event_perspective(event: RawObserverEvent) -> ParticipantPerspective | None:
    return participant_event_perspective(participant_event_metadata(event))


def participant_event_perspective(
    event: ParticipantEventMetadata,
) -> ParticipantPerspective | None:
    """Build provenance from named fields, without an intermediate event dict."""
    identity = event.attribution
    name = _reported_federate(
        event.federate,
        event.participant,
        event.federate_name,
        event.participant_name,
    )
    if identity is None and name is None:
        return None
    source = event.source or "unknown"
    session = (
        identity.session_id
        if identity
        else (
            _session(event.time_observation_session_id)
            or _session(event.declaration_session_id)
            or _session(event.session_id)
        )
    )
    return ParticipantPerspective(source, session, event.federation, name, identity)


def event_federate(event: RawObserverEvent) -> str | None:
    perspective = event_perspective(event)
    return perspective.label if perspective else None


def perspective_ended(
    perspective: ParticipantPerspective, event: RawObserverEvent
) -> bool:
    return participant_event_ended(perspective, participant_event_metadata(event))


def participant_event_ended(
    perspective: ParticipantPerspective, event: ParticipantEventMetadata
) -> bool:
    kind = event.event_type
    subject = participant_event_perspective(event)
    if kind == "federate.resigned":
        return subject is not None and subject == perspective
    if kind != "session.disconnected" or perspective.source != (
        event.source or "unknown"
    ):
        return False
    reported = event.attribution
    session = reported.session_id if reported else event.session_id
    if perspective.session_id is None or perspective.session_id != session:
        return False
    identity = perspective.attribution
    if identity is not None:
        return reported is not None and (
            identity.identity_namespace,
            identity.federation_id,
        ) == (
            reported.identity_namespace,
            reported.federation_id,
        )
    return reported is None and perspective.federation == _label(event.federation)
