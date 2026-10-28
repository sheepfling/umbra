#!/usr/bin/env python3
"""Generate a requirement-to-scenario authoring scaffold from the Java TCK catalog."""

from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import TypeAlias, TypedDict, cast

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "compliance" / "catalogs" / "java-tck-scenario-catalog.json"
DEFAULT_OUTPUT = ROOT / "docs" / "testing" / "JAVA-TCK-REQUIREMENT-SCAFFOLD.md"

JsonValue: TypeAlias = (
    str | int | float | bool | None | list["JsonValue"] | dict[str, "JsonValue"]
)
JsonObject: TypeAlias = dict[str, JsonValue]


class RequirementMapping(TypedDict, total=False):
    requirement_id: str
    standard_section: str


class CatalogExclusion(TypedDict, total=False):
    id: str
    status: str
    scope: str
    reason: str


class _JavaTckScenarioOptional(TypedDict, total=False):
    parity: str
    adapter_inputs: list[str]
    mapping_classification: str
    requirement_mappings: list[RequirementMapping]
    exclusion: str


class JavaTckScenario(_JavaTckScenarioOptional):
    id: str
    category: str
    title: str
    java_test: str
    default_status: str
    api_methods: list[str]
    requirement_ids: list[str]


class _JavaTckCatalogOptional(TypedDict, total=False):
    exclusions: list[CatalogExclusion]


class JavaTckCatalog(_JavaTckCatalogOptional):
    kind: str
    scenarios: list[JavaTckScenario]


def _json_object(value: JsonValue, *, context: str) -> JsonObject:
    if not isinstance(value, dict):
        raise TypeError(f"{context} must be an object")
    return value


def _required_text(value: JsonValue | None, *, field: str, context: str) -> str:
    if not isinstance(value, str) or not value:
        raise TypeError(f"{context} must have a non-empty string {field}")
    return value


def _string_list(value: JsonValue | None, *, field: str, context: str) -> list[str]:
    if not isinstance(value, list):
        raise TypeError(f"{context} must have a string list {field}")
    items: list[str] = []
    for item in value:
        if not isinstance(item, str):
            raise TypeError(f"{context} must have a string list {field}")
        items.append(item)
    return items


def _read_mapping(value: JsonValue, *, context: str) -> RequirementMapping:
    source = _json_object(value, context=context)
    mapping: RequirementMapping = {}
    if "requirement_id" in source:
        mapping["requirement_id"] = _required_text(
            source["requirement_id"], field="requirement_id", context=context
        )
    if "standard_section" in source:
        mapping["standard_section"] = _required_text(
            source["standard_section"], field="standard_section", context=context
        )
    return mapping


def _read_exclusion(value: JsonValue, *, context: str) -> CatalogExclusion:
    source = _json_object(value, context=context)
    exclusion: CatalogExclusion = {}
    for field in ("id", "status", "scope", "reason"):
        if field not in source:
            continue
        text = _required_text(source[field], field=field, context=context)
        if field == "id":
            exclusion["id"] = text
        elif field == "status":
            exclusion["status"] = text
        elif field == "scope":
            exclusion["scope"] = text
        else:
            exclusion["reason"] = text
    return exclusion


def _read_scenario(value: JsonValue) -> JavaTckScenario:
    source = _json_object(value, context="Java TCK scenario")
    raw_id = source.get("id")
    context = raw_id if isinstance(raw_id, str) and raw_id else "<unknown>"
    scenario: JavaTckScenario = {
        "id": _required_text(raw_id, field="id", context=context),
        "category": _required_text(
            source.get("category"), field="category", context=context
        ),
        "title": _required_text(source.get("title"), field="title", context=context),
        "java_test": _required_text(
            source.get("java_test"), field="java_test", context=context
        ),
        "default_status": _required_text(
            source.get("default_status"), field="default_status", context=context
        ),
        "api_methods": _string_list(
            source.get("api_methods"), field="api_methods", context=context
        ),
        "requirement_ids": _string_list(
            source.get("requirement_ids"), field="requirement_ids", context=context
        ),
    }
    if "parity" in source:
        scenario["parity"] = _required_text(
            source["parity"], field="parity", context=context
        )
    if "mapping_classification" in source:
        scenario["mapping_classification"] = _required_text(
            source["mapping_classification"],
            field="mapping_classification",
            context=context,
        )
    if "exclusion" in source:
        scenario["exclusion"] = _required_text(
            source["exclusion"], field="exclusion", context=context
        )

    if "adapter_inputs" in source:
        scenario["adapter_inputs"] = _string_list(
            source["adapter_inputs"], field="adapter_inputs", context=context
        )
    if "requirement_mappings" in source:
        raw_mappings = source["requirement_mappings"]
        if not isinstance(raw_mappings, list):
            raise TypeError(f"{context} must have a mapping list requirement_mappings")
        scenario["requirement_mappings"] = [
            _read_mapping(mapping, context=f"{context} requirement mapping {index}")
            for index, mapping in enumerate(raw_mappings)
        ]

    if not scenario["api_methods"]:
        raise ValueError(f"{context} has no standard API method sketch")
    if not scenario["requirement_ids"] and scenario.get("mapping_classification") != (
        "api-contract-only-not-normative-conformance"
    ):
        raise ValueError(
            f"{context} needs requirement IDs or an explicit classification"
        )
    return scenario


def read_catalog(path: Path) -> JavaTckCatalog:
    payload: object = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise TypeError(f"Java TCK catalog must be an object: {path}")
    catalog_data = cast(JsonObject, payload)
    scenarios_data = catalog_data.get("scenarios")
    if catalog_data.get("kind") != "portable-java-tck-catalog" or not isinstance(
        scenarios_data, list
    ):
        raise ValueError(f"not a portable Java TCK scenario catalog: {path}")

    scenarios = [_read_scenario(scenario) for scenario in scenarios_data]
    ids = [scenario["id"] for scenario in scenarios]
    if len(ids) != len(set(ids)):
        raise ValueError("scenario IDs must be unique non-empty strings")

    catalog: JavaTckCatalog = {
        "kind": "portable-java-tck-catalog",
        "scenarios": scenarios,
    }
    if "exclusions" in catalog_data:
        exclusions_data = catalog_data["exclusions"]
        if not isinstance(exclusions_data, list):
            raise TypeError("Java TCK catalog exclusions must be a list")
        catalog["exclusions"] = [
            _read_exclusion(exclusion, context=f"catalog exclusion {index}")
            for index, exclusion in enumerate(exclusions_data)
        ]
    return catalog


def cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ")


def scenario_anchor(scenario_id: str) -> str:
    return f"scenario-{scenario_id}"


def render(catalog: JavaTckCatalog, catalog_path: Path) -> str:
    scenarios = catalog["scenarios"]
    by_requirement: dict[str, list[JavaTckScenario]] = defaultdict(list)
    by_category: dict[str, list[JavaTckScenario]] = defaultdict(list)
    for scenario in scenarios:
        by_category[scenario["category"]].append(scenario)
        for requirement_id in scenario["requirement_ids"]:
            by_requirement[requirement_id].append(scenario)

    statuses = Counter(scenario["default_status"] for scenario in scenarios)
    parity_count = sum(bool(scenario.get("parity")) for scenario in scenarios)
    exact_mapping_scenarios = [
        scenario for scenario in scenarios if scenario.get("requirement_mappings")
    ]
    exact_mapping_count = sum(
        len(scenario["requirement_mappings"]) for scenario in exact_mapping_scenarios
    )
    api_only = [
        scenario
        for scenario in scenarios
        if scenario.get("mapping_classification")
        == "api-contract-only-not-normative-conformance"
    ]
    exclusions = catalog.get("exclusions", [])

    lines = [
        "<!-- Generated by tools/export_java_tck_requirement_scaffold.py; edit the catalog, then regenerate. -->",
        "# Java TCK requirement-to-translation scaffold",
        "",
        f"Generated from `{catalog_path.relative_to(ROOT).as_posix()}`. This is an authoring index, not a conformance report.",
        "",
        "## Scope and evidence rules",
        "",
        "This scaffold indexes every stable `requirement_id` currently present in the portable Java scenario catalog, and every cataloged scenario sketch. The catalog's `api_methods`, `adapter_inputs`, `contract_refs`, `java_test`, and `parity` fields remain the detailed source for each sketch.",
        "",
        "The index is not a claim that these IDs exhaust IEEE 1516.1-2025 normative requirements. An ID-to-scenario link is traceability, not proof of a test or a normative clause mapping. Only mappings explicitly recorded in `requirement_mappings` are shown as exact section links; do not infer clause numbers from titles, C++ IDs, or broad contract references.",
        "",
        "`default_status` is harness configuration, not provider evidence: `run` means enabled by default and `unsupported` means not enabled pending adapter/provider support. Neither means the provider passed. A C++ `parity` value is a source-case pointer, not proof that the Java behavior is equivalent until the assertions and lifecycle are reviewed.",
        "",
        "Portable Java sources remain limited to JDK types and `hla.rti1516_2025` API types. Provider selection, JARs, FOM/MIM, endpoint, and callback configuration belong at the adapter boundary. No implementation-specific assertion belongs in this TCK.",
        "",
        "## Inventory snapshot",
        "",
        "| Catalog measure | Count |",
        "| --- | ---: |",
        f"| Scenario sketches | {len(scenarios)} |",
        f"| Categories | {len(by_category)} |",
        f"| Distinct stable requirement IDs | {len(by_requirement)} |",
        f"| Scenarios with a C++ parity pointer | {parity_count} |",
        f"| Scenarios without a C++ parity pointer | {len(scenarios) - parity_count} |",
        f"| Default `run` scenarios | {statuses.get('run', 0)} |",
        f"| Default `unsupported` scenarios | {statuses.get('unsupported', 0)} |",
        f"| Scenarios with explicit `requirement_mappings` | {len(exact_mapping_scenarios)} |",
        f"| Explicit mapping records | {exact_mapping_count} |",
        "",
        "The separate `python tools/java_tck.py validate` check audits the catalog against the Requirements Lab Java API surface inventory and checks source purity. Run it after catalog/source changes; this scaffold generator does not replace that validation.",
        "",
        "## Translation card to complete for each promoted case",
        "",
        "Use the matching catalog scenario row as the starting card. Complete these before calling a translated case verified:",
        "",
        "1. Record the C++ TCK scenario ID and exact source test symbol; inspect only the relevant source/test range and existing evidence. A pointer alone is not a behavior review.",
        "2. Summarize the observable contract as preconditions, service calls, callback/event sequence, assertions, and cleanup. Keep assertions at the public Java API boundary.",
        "3. Verify the exact IEEE 1516.1-2025 clause/subclause mapping from an authoritative requirement source. Add only a supported, direct mapping; label internal/API-only checks explicitly.",
        "4. List the standard Java API methods and adapter inputs (provider JAR/factory, FOM/MIM, endpoint, callback model, logical-time configuration, and any test data). Do not add vendor flags or implementation classes.",
        "5. Check equivalent callback servicing modes when the behavior is callback-sensitive, then record compile evidence separately from provider runtime results.",
        "6. Keep scenarios unsupported until a provider run artifact demonstrates the behavior. Record provider, adapter profile, result, and failure detail in the normal evidence path.",
        "7. If no direct portable behavior exists, record a reasoned not-applicable/API-contract-only disposition rather than inventing a test.",
        "",
        "### Blank translation card",
        "",
        "```text",
        "Java scenario ID:",
        "Stable requirement ID(s):",
        "Exact IEEE 1516.1-2025 clause/subclause + source:",
        "C++ source test ID / symbol:",
        "Observable setup and service sequence:",
        "Expected callbacks/events and ordering:",
        "Assertions through the standard Java API:",
        "Standard API methods:",
        "Adapter inputs / FOM data:",
        "Callback model(s) exercised:",
        "Cleanup and isolation:",
        "Compile evidence:",
        "Provider runtime evidence:",
        "Disposition / rationale if not applicable:",
        "```",
        "",
        "## Requirement index",
        "",
        "Every ID below is an ID already declared by the Java catalog. Linked scenario sketches are listed with their default harness status. Exact section links, when present, are recorded in the separate mapping inventory below and remain scenario-level unless the requirement ID is explicitly the same.",
        "",
    ]

    for requirement_id in sorted(by_requirement):
        linked = sorted(
            by_requirement[requirement_id], key=lambda scenario: scenario["id"]
        )
        lines.extend([f"### `{requirement_id}`", ""])
        for scenario in linked:
            title = cell(scenario["title"])
            parity = scenario.get("parity")
            parity_note = (
                f"; C++ pointer `{cell(parity)}`"
                if parity
                else "; C++ source pointer not declared"
            )
            lines.append(
                f"- [`{scenario['id']}`](#{scenario_anchor(scenario['id'])}) — {title} "
                f"(`{scenario['category']}`; default `{scenario['default_status']}`{parity_note})."
            )
        lines.extend(
            [
                "",
                "**Exact clause mapping:** verify/add only from an authoritative 1516.1-2025 source; this stable catalog ID is not itself a clause citation.",
                "",
            ]
        )

    lines.extend(
        [
            "## Explicit clause-mapping records",
            "",
        ]
    )
    if exact_mapping_scenarios:
        for scenario in exact_mapping_scenarios:
            lines.append(f"- `{scenario['id']}`:")
            for mapping in scenario["requirement_mappings"]:
                lines.append(
                    f"  - `{cell(mapping.get('requirement_id', ''))}` → "
                    f"`{cell(mapping.get('standard_section', ''))}`"
                )
        lines.append("")
    else:
        lines.extend(["- None are declared in the catalog.", ""])

    if api_only:
        lines.extend(["## Explicit non-normative/API-only dispositions", ""])
        for scenario in api_only:
            lines.append(
                f"- `{scenario['id']}` — {cell(scenario.get('exclusion', 'No rationale recorded.'))}"
            )
        lines.append("")

    if exclusions:
        lines.extend(["## Explicit C++ source-case scope dispositions", ""])
        lines.append(
            "These entries classify C++ source behaviors that do not belong in a portable Java TCK, "
            "or identify the limited standard slice that is covered separately. They are not provider results."
        )
        lines.append("")
        for exclusion in exclusions:
            lines.append(
                f"- `{cell(exclusion.get('id', ''))}` (`{cell(exclusion.get('status', 'unclassified'))}`) — "
                f"**{cell(exclusion.get('scope', 'Unspecified scope'))}:** "
                f"{cell(exclusion.get('reason', 'No rationale recorded.'))}"
            )
        lines.append("")

    lines.extend(
        [
            "## Scenario sketch index",
            "",
            "The API-method list and adapter inputs are intentionally kept in the canonical catalog to avoid a second hand-maintained copy. Use each linked scenario ID to retrieve those fields before translating its C++ source case.",
            "",
        ]
    )
    for category in sorted(by_category):
        lines.extend(
            [
                f"### `{category}`",
                "",
                "| Scenario sketch | Java entry point | API methods | Requirement IDs | C++ parity pointer | Default | Adapter inputs |",
                "| --- | --- | ---: | --- | --- | --- | --- |",
            ]
        )
        for scenario in sorted(by_category[category], key=lambda item: item["id"]):
            anchor = scenario_anchor(scenario["id"])
            sketch = (
                f'<a id="{anchor}"></a>`{scenario["id"]}` — {cell(scenario["title"])}'
            )
            parity = (
                f"`{cell(scenario['parity'])}`"
                if scenario.get("parity")
                else "not declared"
            )
            req_ids = (
                ", ".join(f"`{cell(item)}`" for item in scenario["requirement_ids"])
                or "explicit API-only disposition"
            )
            inputs = (
                "; ".join(cell(item) for item in scenario.get("adapter_inputs", []))
                or "not scenario-declared"
            )
            lines.append(
                f"| {sketch} | `{cell(scenario['java_test'])}` | {len(scenario['api_methods'])} | "
                f"{req_ids} | {parity} | `{cell(scenario['default_status'])}` | {inputs} |"
            )
        lines.append("")

    lines.extend(
        [
            "## Known limits and next handoff",
            "",
            "- The requirement index covers catalog-linked IDs, not an independently enumerated set of every normative paragraph in IEEE 1516.1-2025. Building that full standard clause inventory and reconciling it to these scenarios is a separate traceability task.",
            "- Exact clause mappings are sparse in the current catalog. Close those mapping gaps before claiming normative requirement coverage, even when a scenario already exists or compiles.",
            "- Use the read-only parity audit to locate C++ catalog cases without a Java parity link: `python tools/audit_cpp_java_tck_parity.py --report-cpp-gaps`. Classify each gap for standard portability before translating it.",
            "- This work is Java-catalog/documentation only; C++ sources are not rebuilt by the scaffold generator.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    catalog = read_catalog(args.catalog)
    output = render(catalog, args.catalog.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output, encoding="utf-8")
    distinct_requirements = {
        item
        for scenario in catalog["scenarios"]
        for item in scenario["requirement_ids"]
    }
    print(
        f"Wrote {args.output}: {len(catalog['scenarios'])} scenarios, "
        f"{len(distinct_requirements)} distinct requirement IDs"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
