"""Capability metadata shared by adapters, backends, and the dashboard.

Capability state is deliberately descriptive rather than prescriptive. An
adapter may report that a provider feature is available, partially observed,
disabled by configuration, unsupported by the provider, or not known. The
frontend can then distinguish an empty result from a feature that cannot
produce a result.
"""

from __future__ import annotations

from collections.abc import Mapping
from typing import cast

from .models import (
    CapabilityRecord,
    CapabilityState,
    JsonObject,
    JsonValue,
    to_json_value,
)

CAPABILITY_STATES = (
    "available",
    "partial",
    "disabled",
    "unsupported",
    "unknown",
)

CAPABILITY_DEFINITIONS: dict[str, dict[str, str]] = {
    "lifecycle": {
        "label": "Runtime lifecycle",
        "description": "Connection, startup, shutdown, and runtime errors.",
    },
    "federates": {
        "label": "Federate roster",
        "description": "Participant join, update, and resignation observations.",
    },
    "objects": {
        "label": "Object observations",
        "description": "Object discovery, removal, and attribute updates.",
    },
    "attributes": {
        "label": "Attribute values",
        "description": "Decoded or opaque attribute update observations.",
    },
    "object_observations": {
        "label": "Object perspectives",
        "description": "Persistent attributed object snapshots and retained attribute comparisons.",
    },
    "interactions": {
        "label": "Interactions",
        "description": "Receive-order and directed interaction observations.",
    },
    "logical_time": {
        "label": "Logical time",
        "description": "Time regulation, constraints, grants, and timestamped delivery.",
    },
    "time_inspection": {
        "label": "Persistent time inspection",
        "description": "Last reported grants/state and bounded explicitly correlated requests, without MOM.",
    },
    "mom": {
        "label": "Management object model",
        "description": "Standard management and federation-management observations.",
    },
    "ownership": {
        "label": "Attribute ownership",
        "description": "Ownership acquisition, divestiture, and query observations.",
    },
    "regions": {
        "label": "Regions and DDM",
        "description": "Region creation and region-scoped publication or subscription.",
    },
    "persistence": {
        "label": "Save and restore",
        "description": "Federation save, restore, and related lifecycle observations.",
    },
    "event_history": {
        "label": "Event history",
        "description": "Bounded, sequence-numbered event retention and cursor pages.",
    },
    "live_inspectors": {
        "label": "Live inspectors",
        "description": "Derived object, interaction, and federate views.",
    },
    "metrics": {
        "label": "Observer metrics",
        "description": "Event counts, rates, retention, and adapter health metrics.",
    },
    "analytics": {
        "label": "Observation analytics",
        "description": "Windowed activity, participant attribution, and logical timestamp observations.",
    },
    "commands": {
        "label": "Session commands",
        "description": "Explicitly registered adapter actions and correlated execution history.",
    },
    "http_api": {
        "label": "HTTP API",
        "description": "HTTP resources for contract, state, metrics, and event pages.",
    },
    "websocket": {
        "label": "WebSocket stream",
        "description": "Live event delivery over a WebSocket connection.",
    },
    "sse": {
        "label": "Server-sent events",
        "description": "Live event delivery over an HTTP event stream.",
    },
}

OBSERVER_CAPABILITIES: dict[str, dict[str, JsonValue]] = {
    "time_inspection": {
        "state": "available",
        "owner": "observer",
        "reason": "Observer-side snapshots; missing IDs or session scope never imply correlated grants.",
    },
    "object_observations": {
        "state": "available",
        "owner": "observer",
        "reason": "Built from explicit event attribution; no MOM or provider queries required.",
    },
    "event_history": {
        "state": "available",
        "owner": "observer",
        "reason": "Provided by the provider-neutral observer store.",
    },
    "live_inspectors": {
        "state": "available",
        "owner": "observer",
        "reason": "Derived from normalized events without requiring provider APIs.",
    },
    "metrics": {
        "state": "available",
        "owner": "observer",
        "reason": "Provided by the provider-neutral observer store.",
    },
}


def normalize_capabilities(
    capabilities: Mapping[str, object] | None,
    *,
    owner: str = "provider",
    source: str | None = None,
) -> dict[str, CapabilityRecord]:
    """Normalize a capability manifest into the wire-compatible shape."""

    if capabilities is None:
        return {}
    if not isinstance(capabilities, Mapping):
        raise TypeError("capabilities must be a mapping")
    if "capabilities" in capabilities and isinstance(capabilities["capabilities"], Mapping):
        capabilities = capabilities["capabilities"]

    normalized: dict[str, CapabilityRecord] = {}
    for raw_name, raw_value in capabilities.items():
        name = str(raw_name).strip()
        if not name:
            raise ValueError("capability names must not be empty")
        if isinstance(raw_value, str):
            item: JsonObject = {"state": raw_value}
        elif isinstance(raw_value, bool):
            item = {"state": "available" if raw_value else "disabled"}
        elif raw_value is None:
            item = {"state": "unknown"}
        elif isinstance(raw_value, Mapping):
            item = {str(key): to_json_value(value) for key, value in raw_value.items()}
        else:
            raise TypeError(f"capability {name!r} must be a state or mapping")

        state = str(item.get("state", "unknown")).strip().lower()
        if state not in CAPABILITY_STATES:
            allowed = ", ".join(CAPABILITY_STATES)
            raise ValueError(f"capability {name!r} has invalid state {state!r}; expected one of {allowed}")
        item["state"] = state
        definition = CAPABILITY_DEFINITIONS.get(name, {})
        item.setdefault("label", definition.get("label", name.replace("_", " ").title()))
        item.setdefault("description", definition.get("description", "Provider or backend capability."))
        item.setdefault("owner", owner)
        if source is not None:
            item.setdefault("source", source)
        if "event_types" in item:
            value = item["event_types"]
            if isinstance(value, str):
                item["event_types"] = [value]
            elif isinstance(value, (list, tuple, set, frozenset)):
                item["event_types"] = [
                    str(event_type)
                    for event_type in sorted({str(event_type) for event_type in value})
                ]
            else:
                raise TypeError(f"capability {name!r} event_types must be a sequence")
        known_keys = {
            "state",
            "label",
            "description",
            "owner",
            "source",
            "owners",
            "sources",
            "observations",
            "reason",
            "reasons",
            "event_types",
        }
        event_types_value = item.get("event_types", [])
        reasons_value = item.get("reasons", [])
        event_types = (
            (event_types_value,)
            if isinstance(event_types_value, str)
            else tuple(str(value) for value in event_types_value)
            if isinstance(event_types_value, list)
            else ()
        )
        reasons = (
            (reasons_value,)
            if isinstance(reasons_value, str)
            else tuple(str(value) for value in reasons_value)
            if isinstance(reasons_value, list)
            else ()
        )
        normalized[name] = CapabilityRecord(
            state=cast(CapabilityState, state),
            label=str(item["label"]),
            description=str(item["description"]),
            owner=str(item["owner"]),
            source=str(item["source"]) if item.get("source") is not None else None,
            event_types=event_types,
            reason=str(item["reason"]) if item.get("reason") is not None else None,
            reasons=reasons,
            extensions={key: value for key, value in item.items() if key not in known_keys},
        )
    return normalized


def _aggregate_state(states: list[CapabilityState]) -> CapabilityState:
    unique = set(states)
    if len(unique) == 1:
        return states[0]
    if "partial" in unique or "available" in unique:
        return "partial"
    if "unknown" in unique:
        return "unknown"
    return "partial"


def merge_capabilities(*manifests: Mapping[str, object] | None) -> dict[str, CapabilityRecord]:
    """Merge capability observations while preserving their provenance."""

    observations: dict[str, list[CapabilityRecord]] = {}
    for manifest in manifests:
        for name, item in normalize_capabilities(manifest).items():
            observations.setdefault(name, []).append(item)

    merged: dict[str, CapabilityRecord] = {}
    for name, entries in observations.items():
        first = entries[0]
        states: list[CapabilityState] = [entry.state for entry in entries]
        sources = sorted({entry.source for entry in entries if entry.source})
        event_types = sorted(
            {
                str(event_type)
                for entry in entries
                for event_type in entry.event_types
            }
        )
        reasons = sorted({entry.reason for entry in entries if entry.reason})
        if len(reasons) == 1:
            reason = reasons[0]
            multiple_reasons: tuple[str, ...] = ()
        else:
            reason = None
            multiple_reasons = tuple(reasons)
        merged[name] = CapabilityRecord(
            state=_aggregate_state(states),
            label=first.label,
            description=first.description,
            owner=first.owner,
            owners=tuple(sorted({entry.owner for entry in entries})),
            sources=tuple(sources),
            observations=tuple(
                {
                    key: value
                    for key, value in entry.to_json().items()
                    if key in {"state", "owner", "source", "reason", "event_types"}
                }
                for entry in entries
            ),
            reason=reason,
            reasons=multiple_reasons,
            event_types=tuple(event_types),
        )
    return dict(sorted(merged.items()))


__all__ = [
    "CAPABILITY_DEFINITIONS",
    "CAPABILITY_STATES",
    "OBSERVER_CAPABILITIES",
    "merge_capabilities",
    "normalize_capabilities",
]
