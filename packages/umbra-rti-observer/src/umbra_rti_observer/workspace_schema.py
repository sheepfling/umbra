"""Generate/check offline workspace contracts from the named Python models."""

# pyright: strict
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import cast

from pydantic import BaseModel, ValidationError

from .models import JsonObject, JsonValue
from .workspace_models import (
    WORKSPACE_VERSION,
    WorkspaceDocument,
    WorkspaceLibrary,
    default_library,
)

SCHEMA_PATH = Path(__file__).with_name("static") / "workspace-v1.schema.json"


def workspace_cases() -> JsonObject:
    """A shared policy/parity corpus; normalized expectations come from models."""

    def document(**values: JsonValue) -> JsonObject:
        return {"workspace_version": WORKSPACE_VERSION, "name": "Miguel", **values}

    library: JsonObject = {"library_version": "umbra-observer-workspace-library-v1"}
    charts: list[JsonValue] = [
        {"object_key": "same", "attribute": "speed"},
        {"scope": "Alice", "object_key": "same", "attribute": "height"},
    ]
    inputs: list[tuple[str, str, JsonObject, bool]] = [
        ("minimal workspace", "workspace", document(), True),
        (
            "nested defaults",
            "workspace",
            document(settings={"events": {"search": "load"}}),
            True,
        ),
        (
            "future filter stays dormant",
            "workspace",
            document(
                settings={
                    "objects": {"status": "future-status"},
                    "mom": {"scope": "unreported", "counter": "unreported-series"},
                }
            ),
            True,
        ),
        (
            "exact numeric perspectives",
            "workspace",
            document(settings={"object_charts": charts}),
            True,
        ),
        ("Unicode scalar length", "workspace", document(name="🔎" * 128), True),
        ("overlong scalar length", "workspace", document(name="🔎" * 129), False),
        ("BOM is not Unicode White_Space", "workspace", document(name="\ufeff"), True),
        (
            "NEXT LINE is Unicode White_Space",
            "workspace",
            document(name="\u0085"),
            False,
        ),
        (
            "Unicode whitespace only",
            "workspace",
            document(name="\t \u00a0\u1680\u2000\u2028\u202f\u205f\u3000"),
            False,
        ),
        ("unpaired surrogate", "workspace", document(name="\ud800"), False),
        (
            "numeric text is not a window",
            "workspace",
            document(settings={"window_seconds": "300"}),
            False,
        ),
        (
            "boolean is not a window",
            "workspace",
            document(settings={"window_seconds": True}),
            False,
        ),
        (
            "boolean text is not a flag",
            "workspace",
            document(settings={"time": {"grants_only": "false"}}),
            False,
        ),
        ("unknown version", "workspace", document(workspace_version="future"), False),
        ("captured state forbidden", "workspace", document(state={}), False),
        (
            "captured URL forbidden",
            "workspace",
            document(api_paths={"state": "https://never-follow.invalid"}),
            False,
        ),
        (
            "command draft forbidden",
            "workspace",
            document(settings={"commands": {"confirmed": True}}),
            False,
        ),
        (
            "duplicate chart target",
            "workspace",
            document(settings={"object_charts": charts + [charts[0]]}),
            False,
        ),
        (
            "separate federate settings",
            "workspace",
            document(
                settings={
                    "federate_views": [
                        {
                            "participant": "Alice",
                            "events": {"source": "source-a"},
                            "time": {"selected": "alice-scope"},
                        },
                        {
                            "participant": "Bob",
                            "events": {"source": "source-b"},
                            "time": {"selected": "bob-scope"},
                        },
                    ]
                }
            ),
            True,
        ),
        (
            "duplicate federate settings",
            "workspace",
            document(
                settings={
                    "federate_views": [
                        {"participant": "Alice"},
                        {"participant": "Alice"},
                    ]
                }
            ),
            False,
        ),
        (
            "prototype property forbidden",
            "workspace",
            document(__proto__={"polluted": True}),
            False,
        ),
        ("minimal library", "library", library, True),
        (
            "saved document defaults",
            "library",
            {**library, "saved": [document()]},
            True,
        ),
        (
            "exact case-sensitive names",
            "library",
            {**library, "saved": [document(name="A"), document(name="a")]},
            True,
        ),
        (
            "no Unicode normalization",
            "library",
            {**library, "saved": [document(name="é"), document(name="e\u0301")]},
            True,
        ),
        (
            "duplicate saved names",
            "library",
            {**library, "saved": [document(), document()]},
            False,
        ),
        (
            "too many saved documents",
            "library",
            {**library, "saved": [document(name=str(index)) for index in range(13)]},
            False,
        ),
    ]
    result: list[JsonObject] = []
    for name, kind, value, valid in inputs:
        try:
            normalized = (
                (WorkspaceDocument if kind == "workspace" else WorkspaceLibrary)
                .model_validate(value)
                .model_dump(mode="json")
            )
        except ValidationError:
            if valid:
                raise
            result.append({"name": name, "kind": kind, "input": value, "valid": False})
        else:
            if not valid:
                raise ValueError(f"workspace policy case unexpectedly accepted: {name}")
            result.append(
                {
                    "name": name,
                    "kind": kind,
                    "input": value,
                    "valid": True,
                    "normalized": normalized,
                }
            )
    return {
        "case_version": "umbra-observer-workspace-cases-v1",
        "cases": cast(JsonValue, result),
    }


def workspace_contract() -> JsonObject:
    defaults = cast(JsonObject, default_library().model_dump(mode="json"))
    example = cast(
        JsonObject,
        WorkspaceDocument(
            workspace_version=WORKSPACE_VERSION, name="Workspace"
        ).model_dump(mode="json"),
    )

    def schema(
        model: type[BaseModel], sample: JsonObject, identifier: str
    ) -> JsonObject:
        result = cast(JsonObject, model.model_json_schema(mode="validation"))
        definitions = cast(dict[str, JsonObject], result.get("$defs", {}))

        def populate(node: JsonObject, values: JsonObject) -> None:
            if "$ref" in node:
                node = definitions[str(node["$ref"]).split("/")[-1]]
            properties = cast(dict[str, JsonObject], node.get("properties", {}))
            required = cast(list[str], node.get("required", []))
            for name, prop in properties.items():
                if name in values:
                    if name not in required:
                        prop["default"] = values[name]
                    value = values[name]
                    if isinstance(value, dict):
                        populate(prop, cast(JsonObject, value))

        populate(result, sample)

        # An empty saved-workspace array has no sample element. Fill factory
        # defaults in every definition too, deriving only objects whose entire
        # shape is defaulted (never invent required names, versions or targets).
        def object_default(node: JsonObject) -> JsonObject | None:
            if "$ref" in node:
                node = definitions[str(node["$ref"]).split("/")[-1]]
            if node.get("type") != "object" or node.get("required"):
                return None
            values: JsonObject = {}
            for name, prop in cast(
                dict[str, JsonObject], node.get("properties", {})
            ).items():
                if "default" in prop:
                    values[name] = prop["default"]
                else:
                    value = object_default(prop)
                    if value is None:
                        return None
                    values[name] = value
            return values

        for definition in (result, *definitions.values()):
            required = cast(list[str], definition.get("required", []))
            for name, prop in cast(
                dict[str, JsonObject], definition.get("properties", {})
            ).items():
                if name not in required and "default" not in prop:
                    value = object_default(prop)
                    if value is not None:
                        prop["default"] = value
        result.update(
            {
                "$schema": "https://json-schema.org/draft/2020-12/schema",
                "$id": identifier,
            }
        )
        return result

    return {
        "workspace_schema": schema(
            WorkspaceDocument, example, "urn:umbra:observer:workspace:v1"
        ),
        "library_schema": schema(
            WorkspaceLibrary, defaults, "urn:umbra:observer:workspace-library:v1"
        ),
        "defaults": defaults,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    parser.add_argument(
        "--frontend",
        type=Path,
        help="optional frontend contract snapshot to write/check too",
    )
    parser.add_argument(
        "--cases",
        type=Path,
        help="optional cross-language policy corpus to write/check too",
    )
    args = parser.parse_args(argv)
    content = (
        json.dumps(workspace_contract(), indent=2, sort_keys=True, ensure_ascii=False)
        + "\n"
    )
    outputs = [
        (path, content)
        for path in (SCHEMA_PATH, *((args.frontend,) if args.frontend else ()))
    ]
    if args.cases:
        # Escaping deliberately preserves invalid surrogate test inputs as JSON
        # text; do not try to encode those inputs as literal UTF-8 characters.
        outputs.append(
            (args.cases, json.dumps(workspace_cases(), indent=2, sort_keys=True) + "\n")
        )
    for path, content in outputs:
        if args.write:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        elif not path.is_file() or path.read_text(encoding="utf-8") != content:
            print(
                f"Workspace contract drift: {path}. Review the models, then regenerate explicitly."
            )
            return 1
    print("Workspace contracts " + ("updated." if args.write else "are current."))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
