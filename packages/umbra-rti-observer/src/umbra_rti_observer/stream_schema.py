"""Expose named stream DTOs in OpenAPI without pretending SSE is JSON HTTP."""

from __future__ import annotations

from .models import JsonObject, JsonValue, to_json_object
from .schemas import (
    ErrorMessage,
    EventsMessage,
    GapMessage,
    HeartbeatMessage,
    SnapshotMessage,
)


def add_stream_schemas(document: JsonObject) -> JsonObject:
    components = document.setdefault("components", {})
    if not isinstance(components, dict):
        raise TypeError("OpenAPI components must be an object")
    schemas = components.setdefault("schemas", {})
    if not isinstance(schemas, dict):
        raise TypeError("OpenAPI schemas must be an object")
    definitions: JsonObject = {}
    for model in (
        SnapshotMessage,
        EventsMessage,
        HeartbeatMessage,
        GapMessage,
        ErrorMessage,
    ):
        schema = to_json_object(model.model_json_schema(mode="serialization"))
        nested = schema.pop("$defs", {})
        if isinstance(nested, dict):
            definitions.update(nested)
        definitions[model.__name__] = schema
    names = {
        name: name + "-Output" if name + "-Output" in schemas else name
        for name in definitions
    }

    def rewrite(value: JsonValue) -> JsonValue:
        if isinstance(value, list):
            return [rewrite(item) for item in value]
        if isinstance(value, dict):
            return {
                key: "#/components/schemas/" + names[item.removeprefix("#/$defs/")]
                if key == "$ref"
                and isinstance(item, str)
                and item.startswith("#/$defs/")
                else rewrite(item)
                for key, item in value.items()
            }
        return value

    for name, schema in definitions.items():
        if names[name] not in schemas:
            schemas[names[name]] = rewrite(schema)
    document["x-observer-stream-messages"] = {
        name: {"$ref": "#/components/schemas/" + name}
        for name in (
            "SnapshotMessage",
            "EventsMessage",
            "HeartbeatMessage",
            "GapMessage",
            "ErrorMessage",
        )
    }
    return document
