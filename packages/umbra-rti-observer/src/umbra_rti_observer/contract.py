"""Stable wire-contract metadata for interchangeable observer backends."""

from __future__ import annotations

from collections.abc import Iterable, Mapping

from .capabilities import (
    CAPABILITY_DEFINITIONS,
    CAPABILITY_STATES,
    normalize_capabilities,
)
from .models import JsonObject, to_json_object

OBSERVER_CONTRACT_VERSION = "umbra-observer-contract-v1"
OBSERVER_JSON_MEDIA_TYPE = "application/vnd.umbra.observer+json"

STREAM_MESSAGE_TYPES = (
    "snapshot",
    "events",
    "heartbeat",
    "gap",
    "error",
)

API_PATHS = {
    "workspace_defaults": "/api/workspaces/defaults",
    "workspace_validate": "/api/workspaces/validate",
    "alerts": "/api/alerts",
    "alert_configuration": "/api/alerts/configuration",
    "alert_reset": "/api/alerts/reset",
    "alert_acknowledge": "/api/alerts/acknowledge",
    "recordings": "/api/recordings",
    "recording_start": "/api/recordings/start",
    "recording_stop": "/api/recordings/stop",
    "recording_export": "/api/recordings/export",
    "recording_validate": "/api/recordings/validate",
    "recording_analyze": "/api/recordings/analyze",
    "contract": "/api/contract",
    "state": "/api/state",
    "metrics": "/api/metrics",
    "analytics": "/api/analytics",
    "command_catalog": "/api/commands/catalog",
    "commands": "/api/commands",
    "events": "/api/events",
    "event_stream": "/api/events/stream",
    "websocket": "/ws/events",
}


def build_contract_document(
    event_types: Iterable[str],
    *,
    event_schema_version: str = "umbra-runtime-observer-v1",
    capabilities: Mapping[str, object] | None = None,
) -> JsonObject:
    """Return the implementation-neutral contract advertised to clients.

    This document is deliberately data-only. A service implemented in Python,
    Java, or another language can expose the same document and use the same
    frontend without importing this package.
    """

    normalized_capabilities = normalize_capabilities(capabilities)
    paths = dict(API_PATHS)
    for capability, resource in (("websocket", "websocket"), ("sse", "event_stream")):
        transport = normalized_capabilities.get(capability)
        if transport is None or transport.state not in {"available", "partial"}:
            paths.pop(resource, None)
    workspaces = normalized_capabilities.get("workspace_validation")
    if workspaces is None or workspaces.state not in {"available", "partial"}:
        paths.pop("workspace_defaults", None)
        paths.pop("workspace_validate", None)
    alerts = normalized_capabilities.get("alerts")
    if alerts is None or alerts.state not in {"available", "partial"}:
        for name in tuple(paths):
            if name == "alerts" or name.startswith("alert_"):
                paths.pop(name)
    alert_controls = normalized_capabilities.get("alert_controls")
    if alert_controls is None or alert_controls.state not in {"available", "partial"}:
        for name in tuple(paths):
            if name.startswith("alert_"):
                paths.pop(name)
    recording = normalized_capabilities.get("recording")
    if recording is None or recording.state not in {"available", "partial"}:
        for name in tuple(paths):
            if name == "recordings" or name.startswith("recording_"):
                paths.pop(name)
    analytics = normalized_capabilities.get("analytics")
    if analytics is None or analytics.state not in {"available", "partial"}:
        paths.pop("analytics", None)
    commands = normalized_capabilities.get("commands")
    if commands is None or commands.state not in {"available", "partial"}:
        paths.pop("commands", None)
        paths.pop("command_catalog", None)
    return to_json_object({
        "contract_version": OBSERVER_CONTRACT_VERSION,
        "media_type": OBSERVER_JSON_MEDIA_TYPE,
        "event_schema_version": event_schema_version,
        "api_paths": paths,
        "capability_schema": {
            "states": list(CAPABILITY_STATES),
            "fields": [
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
            ],
            "known": sorted(CAPABILITY_DEFINITIONS),
            "state_meanings": {
                "available": "The feature is supported and enabled for observation.",
                "partial": "Only some of the feature surface is available.",
                "disabled": "The feature is supported or configured but turned off.",
                "unsupported": "The connected provider or backend cannot provide it.",
                "unknown": "The provider or backend did not advertise its support.",
            },
        },
        "capabilities": {
            name: capability.to_json()
            for name, capability in normalized_capabilities.items()
        },
        "snapshot": {
            "method": "GET",
            "path": API_PATHS["state"],
            "query": {"include_events": "boolean; optional; default true"},
            "required_fields": [
                "contract_version",
                "schema_version",
                "status",
                "context",
                "live_metrics",
                "retention",
                "adapters",
                "capabilities",
                "federate_roster",
                "inspectors",
            ],
        },
        "events": {
            "method": "GET",
            "path": API_PATHS["events"],
            "query": {
                "after": "non-negative sequence cursor",
                "instance_id": "optional observer lifetime; foreign/future cursors reset with explicit reset=true",
                "limit": "1..2000",
                "event_type": "optional comma-separated filter",
                "family": "optional comma-separated filter",
                "source": "optional comma-separated filter",
            },
            "required_fields": [
                "contract_version",
                "events",
                "next_after",
                "has_more",
                "gap",
                "first_retained_sequence",
                "last_sequence",
                "retained_events",
                "dropped_events",
            ],
        },
        "event_envelope": {
            "required_fields": ["sequence", "event_type", "family", "source", "observed_at"],
            "extension_rule": "Unknown fields are retained and must remain JSON-compatible.",
        },
        "stream": {
            "websocket_path": API_PATHS["websocket"],
            "sse_path": API_PATHS["event_stream"],
            "message_types": list(STREAM_MESSAGE_TYPES),
            "messages": {
                "snapshot": "{type, state, cursor?, resume?}",
                "events": "{type, events, state, cursor?}",
                "heartbeat": "{type, cursor?}",
                "gap": "{type, retention, observer_lifetime?}",
                "error": "{type, message}",
            },
        },
        "event_types": list(event_types),
        "backend_rule": "Provider and language-specific code ends at normalized events; the frontend consumes only this contract.",
    })


__all__ = [
    "API_PATHS",
    "OBSERVER_CONTRACT_VERSION",
    "OBSERVER_JSON_MEDIA_TYPE",
    "STREAM_MESSAGE_TYPES",
    "build_contract_document",
]
