"""Load the portable Java TCK catalog and its ordered scenario shards."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any


def _unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON object key {key!r}")
        result[key] = value
    return result


def _read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=_unique_object)


def load_java_tck_catalog(path: Path) -> dict[str, Any]:
    """Return a catalog with all scenarios joined in manifest order.

    Version 1 catalogs store scenarios inline. Version 2 catalogs list ordered,
    repo-local shard paths and keep shared metadata in the manifest.
    """

    catalog = _read_json(path)
    if not isinstance(catalog, dict) or catalog.get("kind") != "portable-java-tck-catalog":
        raise ValueError(f"not a portable Java TCK catalog: {path}")

    version = catalog.get("schema_version")
    if version == 1:
        scenarios = catalog.get("scenarios")
    elif version == 2:
        if "scenarios" in catalog:
            raise ValueError(f"split Java TCK catalog also contains inline scenarios: {path}")
        scenario_files = catalog.get("scenario_files")
        if not isinstance(scenario_files, list) or not scenario_files:
            raise ValueError(f"split Java TCK catalog has no scenario_files: {path}")

        catalog_dir = path.resolve().parent
        scenarios = []
        seen_files: set[str] = set()
        for sequence, relative_name in enumerate(scenario_files, 1):
            if not isinstance(relative_name, str) or not relative_name:
                raise TypeError(f"invalid Java TCK scenario shard path in {path}")
            if relative_name in seen_files:
                raise ValueError(f"duplicate Java TCK scenario shard path: {relative_name}")
            seen_files.add(relative_name)

            relative_path = Path(relative_name)
            if relative_path.is_absolute():
                raise ValueError(f"Java TCK scenario shard path must be relative: {relative_name}")
            shard_path = (catalog_dir / relative_path).resolve()
            try:
                shard_path.relative_to(catalog_dir)
            except ValueError as error:
                raise ValueError(
                    f"Java TCK scenario shard escapes its catalog directory: {relative_name}"
                ) from error

            shard = _read_json(shard_path)
            if (
                not isinstance(shard, dict)
                or shard.get("schema_version") != 1
                or shard.get("kind") != "portable-java-tck-scenario-shard"
            ):
                raise ValueError(f"unsupported Java TCK scenario shard: {shard_path}")
            if shard.get("sequence") != sequence:
                raise ValueError(f"Java TCK scenario shard sequence mismatch: {shard_path}")
            shard_scenarios = shard.get("scenarios")
            if not isinstance(shard_scenarios, list) or not shard_scenarios:
                raise ValueError(f"Java TCK scenario shard has no scenarios: {shard_path}")
            scenarios.extend(shard_scenarios)
        catalog["scenarios"] = scenarios
    else:
        raise ValueError(f"unsupported Java TCK catalog schema version {version!r}: {path}")

    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError(f"Java TCK catalog has no scenarios: {path}")
    identifiers: list[str] = []
    for scenario in scenarios:
        if not isinstance(scenario, dict):
            raise TypeError(f"Java TCK catalog contains a non-object scenario: {path}")
        identifier = scenario.get("id")
        if not isinstance(identifier, str) or not identifier:
            raise ValueError(f"Java TCK catalog contains a scenario without an id: {path}")
        identifiers.append(identifier)
    if len(identifiers) != len(set(identifiers)):
        raise ValueError(f"Java TCK scenario IDs are not unique: {path}")
    return catalog


def load_java_tck_scenario_ids(path: Path) -> set[str]:
    catalog = load_java_tck_catalog(path)
    return {scenario["id"] for scenario in catalog["scenarios"]}
