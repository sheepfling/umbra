"""Export/check the portable response schema and reference OpenAPI document."""

# pyright: strict
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import cast

from pydantic import BaseModel

from .alert_models import AlertConfiguration, AlertReport
from .command_models import CommandCatalog, CommandHistory, CommandRecord
from .contract import OBSERVER_CONTRACT_VERSION
from .models import JsonObject, JsonValue
from .recording_models import (
    CaptureStatus,
    CompactRecordingDocument,
    RecordingDocument,
    RecordingStatusResponse,
)
from .schemas import (
    WIRE_REQUIRED_FIELDS,
    AnalyticsResponse,
    ContractResponse,
    ErrorMessage,
    EventPageResponse,
    EventsMessage,
    GapMessage,
    HealthResponse,
    HeartbeatMessage,
    InspectorResponse,
    MetricsResponse,
    SnapshotMessage,
    StateResponse,
)
from .workspace_models import (
    WorkspaceDefaultsResponse,
    WorkspaceDocument,
    WorkspaceLibrary,
)

WIRE_MODELS: tuple[type[BaseModel], ...] = (
    WorkspaceDocument,
    WorkspaceLibrary,
    WorkspaceDefaultsResponse,
    AlertConfiguration,
    AlertReport,
    ContractResponse,
    HealthResponse,
    StateResponse,
    MetricsResponse,
    EventPageResponse,
    InspectorResponse,
    AnalyticsResponse,
    CommandCatalog,
    CommandHistory,
    CommandRecord,
    SnapshotMessage,
    EventsMessage,
    HeartbeatMessage,
    GapMessage,
    ErrorMessage,
    CaptureStatus,
    RecordingStatusResponse,
    RecordingDocument,
    CompactRecordingDocument,
)
SCHEMA_PATH = Path(__file__).with_name("static") / "observer-wire-v1.schema.json"
MAX_WIRE_SCHEMA_LINES = 15_000


def wire_schema_text() -> str:
    definitions: dict[str, JsonObject] = {}
    for model in WIRE_MODELS:
        schema = model.model_json_schema(
            mode="serialization", ref_template="#/$defs/{model}"
        )
        nested = cast(dict[str, JsonObject], schema.pop("$defs", {}))
        for name, definition in nested.items():
            if name in definitions and definitions[name] != definition:
                raise ValueError(f"conflicting wire definition: {name}")
            definitions[name] = definition
        definitions[model.__name__] = cast(JsonObject, schema)
    for name, required in WIRE_REQUIRED_FIELDS.items():
        definition = definitions[name]
        existing = cast(list[str], definition.get("required", []))
        definition["required"] = cast(JsonValue, sorted(set(existing) | required))
    text = (
        json.dumps(
            {
                "$schema": "https://json-schema.org/draft/2020-12/schema",
                "$id": "urn:umbra:observer:wire:v1",
                "contract_version": OBSERVER_CONTRACT_VERSION,
                "$defs": definitions,
            },
            indent=2,
            sort_keys=True,
            ensure_ascii=False,
        )
        + "\n"
    )
    if text.count("\n") > MAX_WIRE_SCHEMA_LINES:
        raise ValueError(
            f"Portable wire schema exceeds its {MAX_WIRE_SCHEMA_LINES:,}-line growth guard; review model ownership without hand-splitting or minifying the canonical artifact."
        )
    return text


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write", action="store_true", help="explicitly update the portable schema"
    )
    parser.add_argument(
        "--openapi",
        type=Path,
        help="export fresh OpenAPI for TypeScript generation/checking",
    )
    args = parser.parse_args(argv)
    text = wire_schema_text()
    if args.write:
        SCHEMA_PATH.write_text(text, encoding="utf-8")
        print(f"Updated {SCHEMA_PATH}")
    elif not SCHEMA_PATH.is_file() or SCHEMA_PATH.read_text(encoding="utf-8") != text:
        print(
            "Wire schema drift: review the model change, then run conformance_schema --write."
        )
        return 1
    else:
        print("Portable wire schema is current.")
    if args.openapi:
        from .core import ObserverStore
        from .server import create_app

        store = ObserverStore()
        try:
            args.openapi.parent.mkdir(parents=True, exist_ok=True)
            args.openapi.write_text(
                json.dumps(create_app(store).openapi(), sort_keys=True),
                encoding="utf-8",
            )
        finally:
            store.close()
        print(f"Exported reference OpenAPI to {args.openapi}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
