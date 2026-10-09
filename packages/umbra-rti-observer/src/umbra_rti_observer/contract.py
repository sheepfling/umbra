"""Stable wire-contract metadata for interchangeable observer backends."""

from __future__ import annotations

from collections.abc import Iterable
from typing import Any

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
    "contract": "/api/contract",
    "state": "/api/state",
    "metrics": "/api/metrics",
    "events": "/api/events",
    "event_stream": "/api/events/stream",
    "websocket": "/ws/events",
}


def build_contract_document(
    event_types: Iterable[str],
    *,
    event_schema_version: str = "umbra-runtime-observer-v1",
) -> dict[str, Any]:
    """Return the implementation-neutral contract advertised to clients.

    This document is deliberately data-only. A service implemented in Python,
    Java, or another language can expose the same document and use the same
    frontend without importing this package.
    """

    return {
        "contract_version": OBSERVER_CONTRACT_VERSION,
        "media_type": OBSERVER_JSON_MEDIA_TYPE,
        "event_schema_version": event_schema_version,
        "api_paths": dict(API_PATHS),
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
                "federate_roster",
                "inspectors",
            ],
        },
        "events": {
            "method": "GET",
            "path": API_PATHS["events"],
            "query": {
                "after": "non-negative sequence cursor",
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
                "snapshot": "{type, state}",
                "events": "{type, events, state}",
                "heartbeat": "{type}",
                "gap": "{type, retention}",
                "error": "{type, message}",
            },
        },
        "event_types": list(event_types),
        "backend_rule": "Provider and language-specific code ends at normalized events; the frontend consumes only this contract.",
    }


__all__ = [
    "API_PATHS",
    "OBSERVER_CONTRACT_VERSION",
    "OBSERVER_JSON_MEDIA_TYPE",
    "STREAM_MESSAGE_TYPES",
    "build_contract_document",
]
