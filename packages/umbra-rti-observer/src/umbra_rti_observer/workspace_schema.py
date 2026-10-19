"""Generate/check offline workspace contracts from the named Python models."""

# pyright: strict
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import cast

from pydantic import BaseModel

from .workspace_models import (
    WORKSPACE_VERSION,
    WorkspaceDocument,
    WorkspaceLibrary,
    default_library,
)

SCHEMA_PATH = Path(__file__).with_name("static") / "workspace-v1.schema.json"


def workspace_contract() -> dict[str, object]:
    defaults = default_library().model_dump(mode="json")
    example = WorkspaceDocument(workspace_version=WORKSPACE_VERSION, name="Workspace").model_dump(mode="json")

    def schema(model: type[BaseModel], sample: dict[str, object], identifier: str) -> dict[str, object]:
        result = cast(dict[str, object], model.model_json_schema(mode="validation"))
        definitions = cast(dict[str, dict[str, object]], result.get("$defs", {}))

        def populate(node: dict[str, object], values: dict[str, object]) -> None:
            if "$ref" in node:
                node = definitions[str(node["$ref"]).split("/")[-1]]
            properties = cast(dict[str, dict[str, object]], node.get("properties", {}))
            required = cast(list[str], node.get("required", []))
            for name, prop in properties.items():
                if name in values:
                    if name not in required:
                        prop["default"] = values[name]
                    value = values[name]
                    if isinstance(value, dict):
                        populate(prop, cast(dict[str, object], value))
        populate(result, sample)
        # An empty saved-workspace array has no sample element. Fill factory
        # defaults in every definition too, deriving only objects whose entire
        # shape is defaulted (never invent required names, versions or targets).
        def object_default(node: dict[str, object]) -> dict[str, object] | None:
            if "$ref" in node:
                node = definitions[str(node["$ref"]).split("/")[-1]]
            if node.get("type") != "object" or node.get("required"):
                return None
            values: dict[str, object] = {}
            for name, prop in cast(dict[str, dict[str, object]], node.get("properties", {})).items():
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
            for name, prop in cast(dict[str, dict[str, object]], definition.get("properties", {})).items():
                if name not in required and "default" not in prop:
                    value = object_default(prop)
                    if value is not None:
                        prop["default"] = value
        result.update({"$schema": "https://json-schema.org/draft/2020-12/schema", "$id": identifier})
        return result

    return {"workspace_schema": schema(WorkspaceDocument, example, "urn:umbra:observer:workspace:v1"),
            "library_schema": schema(WorkspaceLibrary, defaults, "urn:umbra:observer:workspace-library:v1"),
            "defaults": defaults}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--frontend", type=Path, help="optional frontend contract snapshot to write/check too")
    args = parser.parse_args(argv)
    content = json.dumps(workspace_contract(), indent=2, sort_keys=True, ensure_ascii=False) + "\n"
    for path in (SCHEMA_PATH, *((args.frontend,) if args.frontend else ())):
        if args.write:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        elif not path.is_file() or path.read_text(encoding="utf-8") != content:
            print(f"Workspace contract drift: {path}. Review the models, then regenerate explicitly.")
            return 1
    print("Workspace contracts " + ("updated." if args.write else "are current."))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
