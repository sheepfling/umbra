"""Verify the checked-in Python RTI packaging and edition report.

This is intentionally a structural gate.  It validates that the two Python
namespaces, package entry points, JNI ServiceLoader boundary, required test
artifacts, and matrix-to-test anchors agree with the catalog. Behavioral
statuses in the catalog are reported verbatim; ``deferred`` and ``bounded``
are explicit scope decisions, not verifier failures. Provider execution
remains the responsibility of the route-specific test commands.
"""

from __future__ import annotations

import argparse
import ast
import json
import sys
from pathlib import Path
from typing import Any

import tomllib

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = (
    ROOT / "compliance" / "catalogs" / "python-surface-completeness-catalog.json"
)
_ALLOWED_STATUSES = {
    "covered",
    "matrix-covered",
    "surface-bound",
    "bounded",
    "deferred",
    "not-claimed",
}


def _load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise TypeError(f"{path} must contain a JSON object")
    return value


def _literal_constants(path: Path) -> dict[str, str]:
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    constants: dict[str, str] = {}
    for node in tree.body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        if (
            isinstance(target, ast.Name)
            and isinstance(node.value, ast.Constant)
            and isinstance(node.value.value, str)
        ):
            constants[target.id] = node.value.value
    return constants


def _pyproject(path: Path) -> dict[str, Any]:
    with path.open("rb") as stream:
        value = tomllib.load(stream)
    if not isinstance(value, dict):
        raise TypeError(f"{path} did not contain a TOML table")
    return value


def _entry_points(metadata: dict[str, Any]) -> dict[str, dict[str, str]]:
    project = metadata.get("project", {})
    if not isinstance(project, dict):
        return {}
    groups = project.get("entry-points", {})
    if not isinstance(groups, dict):
        return {}
    return {
        str(group): {
            str(name): str(value)
            for name, value in values.items()
            if isinstance(values, dict)
        }
        for group, values in groups.items()
        if isinstance(values, dict)
    }


def _service_loader_values(path: Path) -> tuple[str, ...]:
    if not path.is_file():
        return ()
    return tuple(
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    )


def _route_findings(route: object) -> list[str]:
    """Validate one ``path#test_method`` evidence link.

    Evidence may be linked to a concrete test method rather than only to a
    test file.  That keeps the catalog transplantable: a renamed or removed
    test cannot silently leave a green-looking report.
    """

    if not isinstance(route, str) or not route:
        return ["route must be a non-empty string"]
    path_text, separator, method_name = route.partition("#")
    path = ROOT / path_text
    findings: list[str] = []
    if not path.is_file():
        findings.append(f"route file is missing: {path_text}")
        return findings
    if not separator:
        return findings
    if not method_name:
        findings.append(f"route method is empty: {route}")
        return findings
    try:
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    except (OSError, SyntaxError, UnicodeDecodeError) as error:
        findings.append(f"route file cannot be parsed: {path_text}: {error}")
        return findings
    if not any(
        isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
        and node.name == method_name
        for node in ast.walk(tree)
    ):
        findings.append(f"route method is missing: {route}")
    return findings


def _derived_matrix_counts() -> tuple[dict[str, object] | None, str | None]:
    """Derive checked-in matrix counts from the provider-neutral helpers."""

    source_roots = (
        ROOT / "packages" / "umbra-rti-api" / "src",
        ROOT / "packages" / "umbra-rti-test-support" / "src",
    )
    inserted: list[str] = []
    for source_root in reversed(source_roots):
        value = str(source_root)
        if value not in sys.path:
            sys.path.insert(0, value)
            inserted.append(value)
    try:
        from umbra_rti_test_support import (
            CALLBACK_OVERLOAD_COUNTS,
            iter_callback_provenance_matrix,
            iter_data_element_value_matrix,
            iter_extendable_variant_wire_matrix,
            iter_logical_time_arithmetic_matrix,
            iter_logical_time_wire_matrix,
            iter_save_restore_matrix,
            iter_surface_matrix,
            iter_vendor_data_element_encode_matrix,
            iter_vendor_data_element_wire_matrix,
            iter_vendor_time_arithmetic_matrix,
        )

        return (
            {
                "basic-data-element-values": {
                    edition: len(iter_data_element_value_matrix(edition))
                    for edition in ("2010", "2025")
                },
                "callback-ordering-member-state-space": len(iter_surface_matrix()),
                "callback-provenance-payload-parity": len(
                    iter_callback_provenance_matrix()
                ),
                "callback-delivery-provenance-normalization": len(
                    iter_callback_provenance_matrix()
                )
                * 2,
                "callback-overload-carriers": {
                    edition: CALLBACK_OVERLOAD_COUNTS[edition] * 2
                    for edition in ("2010", "2025")
                },
                "save-restore-outcomes": len(iter_save_restore_matrix()),
                "logical-time-wire-and-arithmetic": {
                    "wire_vector_count": len(iter_logical_time_wire_matrix()),
                    "arithmetic_vector_count": len(
                        iter_logical_time_arithmetic_matrix()
                    ),
                },
                "provider-extension-wire": len(iter_extendable_variant_wire_matrix()),
                "vendor-time-arithmetic-surface": len(
                    iter_vendor_time_arithmetic_matrix()
                ),
                "vendor-data-element-wire-surface": len(
                    iter_vendor_data_element_wire_matrix()
                ),
                "vendor-data-element-encode-surface": len(
                    iter_vendor_data_element_encode_matrix()
                ),
            },
            None,
        )
    except (ImportError, OSError, TypeError, ValueError) as error:
        return None, str(error)
    finally:
        for value in inserted:
            try:
                sys.path.remove(value)
            except ValueError:
                pass


def _validate_matrix_evidence(catalog: dict[str, Any]) -> list[str]:
    """Validate the machine-readable matrix-to-test linkage in the catalog."""

    evidence = catalog.get("matrix_evidence")
    if not isinstance(evidence, list) or not evidence:
        return ["catalog must contain matrix_evidence"]
    findings: list[str] = []
    derived, derivation_error = _derived_matrix_counts()
    if derived is None:
        findings.append(
            f"cannot derive provider-neutral matrix counts: {derivation_error}"
        )
    identifiers: set[str] = set()
    for item in evidence:
        if not isinstance(item, dict):
            findings.append("matrix evidence entry is not an object")
            continue
        identifier = item.get("id")
        if not isinstance(identifier, str) or not identifier:
            findings.append("matrix evidence entries need string ids")
        elif identifier in identifiers:
            findings.append(f"duplicate matrix evidence id {identifier!r}")
        else:
            identifiers.add(identifier)
        support = item.get("support")
        if not isinstance(support, str) or not support:
            findings.append(f"matrix {identifier!r} needs a support path")
        elif not (ROOT / support).is_file():
            findings.append(f"matrix {identifier!r} support path is missing: {support}")
        requirement_refs = item.get("requirement_refs", [])
        if not isinstance(requirement_refs, list):
            findings.append(f"matrix {identifier!r} requirement_refs must be an array")
        else:
            for requirement_ref in requirement_refs:
                if not isinstance(requirement_ref, str) or not requirement_ref:
                    findings.append(
                        f"matrix {identifier!r} requirement references must be paths"
                    )
                elif not (ROOT / requirement_ref).is_file():
                    findings.append(
                        f"matrix {identifier!r} requirement reference is missing: "
                        f"{requirement_ref}"
                    )
        routes = item.get("routes")
        editions = item.get("editions")
        if routes is None and editions is None:
            findings.append(f"matrix {identifier!r} needs at least one route")
        elif routes is not None and (not isinstance(routes, list) or not routes):
            findings.append(f"matrix {identifier!r} routes must be a non-empty array")
        elif isinstance(routes, list):
            for route in routes:
                findings.extend(
                    f"{identifier}: {finding}" for finding in _route_findings(route)
                )
        for field in ("vector_count", "wire_vector_count", "arithmetic_vector_count"):
            if field in item and (
                isinstance(item[field], bool)
                or not isinstance(item[field], int)
                or item[field] < 1
            ):
                findings.append(
                    f"matrix {identifier!r} {field} must be a positive integer"
                )
        expected = derived.get(identifier) if derived is not None else None
        if isinstance(expected, int) and item.get("vector_count") != expected:
            findings.append(
                f"matrix {identifier!r} vector_count is {item.get('vector_count')!r}, "
                f"derived {expected}"
            )
        if isinstance(expected, dict):
            for field in ("wire_vector_count", "arithmetic_vector_count"):
                if field in expected and item.get(field) != expected[field]:
                    findings.append(
                        f"matrix {identifier!r} {field} is {item.get(field)!r}, "
                        f"derived {expected[field]}"
                    )
        if editions is not None:
            if not isinstance(editions, dict) or not editions:
                findings.append(
                    f"matrix {identifier!r} editions must be a non-empty object"
                )
            else:
                for edition, details in editions.items():
                    if edition not in {"2010", "2025"}:
                        findings.append(
                            f"matrix {identifier!r} has unknown edition {edition!r}"
                        )
                        continue
                    if not isinstance(details, dict):
                        findings.append(
                            f"matrix {identifier!r}/{edition} must be an object"
                        )
                        continue
                    count = details.get("vector_count")
                    if (
                        isinstance(count, bool)
                        or not isinstance(count, int)
                        or count < 1
                    ):
                        findings.append(
                            f"matrix {identifier!r}/{edition} vector_count must be a positive integer"
                        )
                    if (
                        isinstance(expected, dict)
                        and edition in expected
                        and count != expected[edition]
                    ):
                        findings.append(
                            f"matrix {identifier!r}/{edition} vector_count is {count!r}, "
                            f"derived {expected[edition]}"
                        )
                    edition_routes = details.get("routes")
                    if not isinstance(edition_routes, list) or not edition_routes:
                        findings.append(
                            f"matrix {identifier!r}/{edition} needs at least one route"
                        )
                    else:
                        for route in edition_routes:
                            findings.extend(
                                f"{identifier}/{edition}: {finding}"
                                for finding in _route_findings(route)
                            )
    return findings


def _validate_roundtrip_catalog(edition: object, value: object) -> list[str]:
    """Validate the detailed JNI carrier catalog linked by one edition.

    The umbrella report intentionally stays compact.  Each edition therefore
    links a carrier-level catalog that can be copied with a provider-specific
    test tranche.  Keep this check structural: it verifies the link, identity,
    and the shared value-matrix count without trying to claim provider
    semantics from a JSON inventory.
    """

    findings: list[str] = []
    if not isinstance(edition, str) or not edition:
        return ["roundtrip catalog has no valid edition id"]
    if not isinstance(value, str) or not value:
        return [f"edition {edition!r} needs a roundtrip_catalog path"]
    path = ROOT / value
    if not path.is_file():
        return [f"edition {edition!r} roundtrip catalog is missing: {value}"]
    try:
        detail = _load_json(path)
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as error:
        return [f"edition {edition!r} roundtrip catalog cannot be loaded: {error}"]

    expected_standard = {
        "2010": "IEEE 1516.1-2010",
        "2025": "IEEE 1516.1-2025",
    }.get(edition)
    if expected_standard is None:
        findings.append(f"roundtrip catalog has unknown edition {edition!r}")
    elif detail.get("standard") != expected_standard:
        findings.append(
            f"edition {edition!r} roundtrip catalog standard is "
            f"{detail.get('standard')!r}, expected {expected_standard!r}"
        )
    expected_kind = {
        "2010": "ieee1516e-2010-jni-type-roundtrip-catalog",
        "2025": "ieee1516-2025-jni-type-roundtrip-catalog",
    }.get(edition)
    if expected_kind is not None and detail.get("kind") != expected_kind:
        findings.append(
            f"edition {edition!r} roundtrip catalog kind is "
            f"{detail.get('kind')!r}, expected {expected_kind!r}"
        )
    for field in ("route", "source_boundary", "python_test"):
        if not isinstance(detail.get(field), str) or not detail[field]:
            findings.append(
                f"edition {edition!r} roundtrip catalog needs a non-empty {field}"
            )
    python_test = detail.get("python_test")
    if isinstance(python_test, str) and python_test:
        test_path = ROOT / python_test
        if not test_path.is_file():
            findings.append(
                f"edition {edition!r} roundtrip catalog Python test is missing: "
                f"{python_test}"
            )

    basic = detail.get("basic_value_matrix")
    if not isinstance(basic, dict):
        findings.append(f"edition {edition!r} roundtrip catalog needs basic_value_matrix")
    else:
        expected_count = {"2010": 79, "2025": 103}.get(edition)
        if expected_count is not None and basic.get("vector_count") != expected_count:
            findings.append(
                f"edition {edition!r} roundtrip basic vector_count is "
                f"{basic.get('vector_count')!r}, expected {expected_count}"
            )
        data_elements = basic.get("data_elements")
        if not isinstance(data_elements, list) or not data_elements:
            findings.append(
                f"edition {edition!r} roundtrip catalog data_elements must be non-empty"
            )
        elif len(set(data_elements)) != len(data_elements):
            findings.append(
                f"edition {edition!r} roundtrip catalog data_elements contain duplicates"
            )

    # The original 2010 catalog predates the more descriptive 2025 field
    # names.  Accept its established aliases while requiring the same two
    # categories of inventory from both editions.
    inventory_groups = (
        ("handle_and_collection_carrier_matrix", "python_carrier_facade_matrix"),
        ("record_enum_and_exception_carriers", "carrier_inventories"),
    )
    for *aliases, in inventory_groups:
        if not any(isinstance(detail.get(alias), dict) for alias in aliases):
            findings.append(
                f"edition {edition!r} roundtrip catalog needs one of: "
                + ", ".join(aliases)
            )
    return findings


def _validate_catalog_shape(catalog: dict[str, Any]) -> list[str]:
    findings: list[str] = []
    if catalog.get("schema_version") != 1:
        findings.append("catalog schema_version must be 1")
    if catalog.get("kind") != "python-rti-surface-completeness-catalog":
        findings.append("catalog kind is not python-rti-surface-completeness-catalog")
    editions = catalog.get("editions")
    if not isinstance(editions, list) or not editions:
        findings.append("catalog must contain a non-empty editions array")
    else:
        identifiers: set[str] = set()
        identity_values: dict[str, set[str]] = {
            field: set()
            for field in (
                "python_namespace",
                "java_package",
                "cpp_namespace",
                "factory_entry_point_group",
            )
        }
        for edition in editions:
            if not isinstance(edition, dict):
                findings.append("catalog contains a non-object edition")
                continue
            identifier = edition.get("id")
            if not isinstance(identifier, str) or not identifier:
                findings.append("every edition needs a string id")
            elif identifier in identifiers:
                findings.append(f"duplicate edition id {identifier!r}")
            else:
                identifiers.add(identifier)
            adapters = edition.get("adapters")
            if not isinstance(adapters, list) or not adapters:
                findings.append(f"edition {identifier!r} has no adapters")
            elif len(
                {adapter.get("id") for adapter in adapters if isinstance(adapter, dict)}
            ) != len(adapters):
                findings.append(
                    f"edition {identifier!r} has duplicate or invalid adapter ids"
                )
            for field in (
                "standard",
                "python_namespace",
                "java_package",
                "cpp_namespace",
                "factory_entry_point_group",
            ):
                if not isinstance(edition.get(field), str) or not edition[field]:
                    findings.append(f"edition {identifier!r} needs a non-empty {field}")
            for field, values in identity_values.items():
                value = edition.get(field)
                if not isinstance(value, str) or not value:
                    continue
                if value in values:
                    findings.append(
                        f"edition identity field {field!r} is duplicated: {value!r}"
                    )
                values.add(value)
            findings.extend(
                _validate_roundtrip_catalog(
                    identifier,
                    edition.get("roundtrip_catalog"),
                )
            )
    dimensions = catalog.get("coverage_dimensions")
    if not isinstance(dimensions, list) or not dimensions:
        findings.append("catalog must contain coverage_dimensions")
    else:
        dimension_ids: set[str] = set()
        for dimension in dimensions:
            if not isinstance(dimension, dict):
                findings.append("coverage dimension is not an object")
                continue
            identifier = dimension.get("id")
            status = dimension.get("status")
            if not isinstance(identifier, str) or not identifier:
                findings.append("coverage dimensions need string ids")
            elif identifier in dimension_ids:
                findings.append(f"duplicate coverage dimension id {identifier!r}")
            else:
                dimension_ids.add(identifier)
            if status not in _ALLOWED_STATUSES:
                findings.append(
                    f"coverage dimension {identifier!r} has unknown status {status!r}"
                )
            evidence = dimension.get("evidence")
            if not isinstance(evidence, list) or not evidence:
                findings.append(f"coverage dimension {identifier!r} has no evidence")
            elif any(not isinstance(path, str) or not path for path in evidence):
                findings.append(
                    f"coverage dimension {identifier!r} evidence must contain paths"
                )
            requirement_refs = dimension.get("requirement_refs", [])
            if not isinstance(requirement_refs, list):
                findings.append(
                    f"coverage dimension {identifier!r} requirement_refs must be an array"
                )
    findings.extend(_validate_matrix_evidence(catalog))
    return findings


def verify(catalog_path: Path = DEFAULT_CATALOG) -> dict[str, Any]:
    """Return a machine-readable structural report for the Python routes."""

    try:
        catalog = _load_json(catalog_path)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError, ValueError) as error:
        return {
            "kind": "python-rti-surface-completeness-report",
            "catalog": str(catalog_path.resolve()),
            "status": "fail",
            "findings": [f"cannot load catalog: {error}"],
            "editions": [],
            "coverage_dimensions": [],
            "matrix_evidence": [],
        }

    findings = _validate_catalog_shape(catalog)
    edition_reports: list[dict[str, Any]] = []
    for edition in catalog.get("editions", ()):
        if not isinstance(edition, dict):
            continue
        identifier = str(edition.get("id", "<unknown>"))
        edition_findings: list[str] = []
        api_project = edition.get("api_project", {})
        if not isinstance(api_project, dict):
            edition_findings.append("api_project must be an object")
        else:
            api_path = ROOT / str(api_project.get("path", ""))
            try:
                metadata = _pyproject(api_path)
                project = metadata.get("project", {})
                actual_name = project.get("name") if isinstance(project, dict) else None
                if actual_name != api_project.get("name"):
                    edition_findings.append(
                        f"API project name is {actual_name!r}, expected {api_project.get('name')!r}"
                    )
            except (OSError, tomllib.TOMLDecodeError, ValueError) as error:
                edition_findings.append(f"cannot read API project metadata: {error}")

        expected_namespace = str(edition.get("python_namespace", ""))
        namespace_path = (
            ROOT
            / "packages"
            / "umbra-rti-api"
            / "src"
            / Path(*expected_namespace.split("."))
        )
        if not namespace_path.is_dir():
            edition_findings.append(
                f"Python namespace directory is missing: {namespace_path}"
            )
        constants_path = namespace_path / (
            "contracts.py" if identifier == "2010" else "public.py"
        )
        try:
            constants = _literal_constants(constants_path)
        except (OSError, SyntaxError, UnicodeDecodeError) as error:
            constants = {}
            edition_findings.append(f"cannot inspect namespace constants: {error}")
        for constant, expected in (
            ("STANDARD_EDITION", edition.get("standard")),
            ("JAVA_PACKAGE", edition.get("java_package")),
            ("CPP_NAMESPACE", edition.get("cpp_namespace")),
        ):
            if constants.get(constant) != expected:
                edition_findings.append(
                    f"{expected_namespace}.{constant} is {constants.get(constant)!r}, expected {expected!r}"
                )

        adapter_reports: list[dict[str, Any]] = []
        group = str(edition.get("factory_entry_point_group", ""))
        for adapter in edition.get("adapters", ()):
            if not isinstance(adapter, dict):
                edition_findings.append("adapter entry is not an object")
                continue
            adapter_id = str(adapter.get("id", "<unknown>"))
            adapter_findings: list[str] = []
            project_path = ROOT / str(adapter.get("project", ""))
            expected_alias = adapter.get("entry_point_alias")
            expected_value = adapter.get("entry_point")
            if project_path.is_file() and project_path.name == "pyproject.toml":
                try:
                    points = _entry_points(_pyproject(project_path)).get(group, {})
                except (OSError, tomllib.TOMLDecodeError, ValueError) as error:
                    points = {}
                    adapter_findings.append(f"cannot read adapter metadata: {error}")
                if points.get(str(expected_alias)) != expected_value:
                    adapter_findings.append(
                        f"entry point {group}/{expected_alias} is {points.get(str(expected_alias))!r}, expected {expected_value!r}"
                    )
            elif project_path.is_dir():
                if adapter_id == "jni" and identifier == "2010":
                    descriptor = (
                        project_path
                        / "src"
                        / "main"
                        / "resources"
                        / "META-INF"
                        / "services"
                        / "hla.rti1516e.RtiFactory"
                    )
                    if str(expected_value) not in _service_loader_values(descriptor):
                        adapter_findings.append(
                            f"2010 JNI ServiceLoader descriptor does not expose {expected_value!r}"
                        )
                else:
                    adapter_findings.append(
                        f"unsupported directory adapter layout for {adapter_id!r}"
                    )
            else:
                adapter_findings.append(f"adapter project is missing: {project_path}")
            required_tests = adapter.get("required_tests", ())
            if not isinstance(required_tests, list) or not required_tests:
                adapter_findings.append("adapter has no required_tests")
            else:
                for test_path in required_tests:
                    if not (ROOT / str(test_path)).is_file():
                        adapter_findings.append(
                            f"required test is missing: {test_path}"
                        )
            adapter_reports.append(
                {
                    "id": adapter_id,
                    "declared_status": adapter.get("status"),
                    "status": "fail" if adapter_findings else "pass",
                    "findings": adapter_findings,
                }
            )
            edition_findings.extend(
                f"{adapter_id}: {finding}" for finding in adapter_findings
            )
        edition_reports.append(
            {
                "id": identifier,
                "standard": edition.get("standard"),
                "python_namespace": expected_namespace,
                "status": "fail" if edition_findings else "pass",
                "findings": edition_findings,
                "adapters": adapter_reports,
            }
        )

    coverage = catalog.get("coverage_dimensions", [])
    coverage_reports = [
        {
            "id": item.get("id"),
            "status": item.get("status"),
            "evidence": item.get("evidence", []),
            "requirement_refs": item.get("requirement_refs", []),
        }
        for item in coverage
        if isinstance(item, dict)
    ]
    for dimension in coverage:
        if not isinstance(dimension, dict):
            continue
        for evidence_path in dimension.get("evidence", ()):
            for finding in _route_findings(evidence_path):
                # Keep the coverage-dimension diagnostic stable for callers
                # that supply a plain path, while method-qualified evidence
                # shares the same route validation as matrix evidence.
                if finding.startswith("route file is missing:"):
                    finding = finding.replace(
                        "route file is missing:", "evidence path is missing:", 1
                    )
                findings.append(f"{dimension.get('id')}: {finding}")
        for requirement_ref in dimension.get("requirement_refs", ()):
            if (
                not isinstance(requirement_ref, str)
                or not (ROOT / requirement_ref).is_file()
            ):
                findings.append(
                    f"{dimension.get('id')}: requirement reference is missing: {requirement_ref!r}"
                )
    findings.extend(
        f"{edition['id']}: {finding}"
        for edition in edition_reports
        if edition["status"] == "fail"
        for finding in edition["findings"]
    )
    matrix_reports = []
    for item in catalog.get("matrix_evidence", []):
        if not isinstance(item, dict):
            continue
        matrix_reports.append(
            {
                "id": item.get("id"),
                "support": item.get("support"),
                "vector_count": item.get("vector_count"),
                "wire_vector_count": item.get("wire_vector_count"),
                "arithmetic_vector_count": item.get("arithmetic_vector_count"),
                "editions": item.get("editions", {}),
                "routes": item.get("routes", []),
                "requirement_refs": item.get("requirement_refs", []),
            }
        )
    return {
        "kind": "python-rti-surface-completeness-report",
        "catalog": str(catalog_path.resolve()),
        "status": "fail" if findings else "pass",
        "findings": findings,
        "editions": edition_reports,
        "coverage_dimensions": coverage_reports,
        "matrix_evidence": matrix_reports,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument(
        "--json", action="store_true", help="emit machine-readable JSON"
    )
    args = parser.parse_args()
    result = verify(args.catalog)
    if args.json:
        print(json.dumps(result, indent=2))
    elif result["findings"]:
        print("Python RTI surface report verification: FAIL", file=sys.stderr)
        for finding in result["findings"]:
            print(f"  {finding}", file=sys.stderr)
    else:
        print("Python RTI surface report verification: PASS")
        for edition in result["editions"]:
            print(f"  {edition['standard']}: {len(edition['adapters'])} adapter routes")
        for dimension in result["coverage_dimensions"]:
            print(f"  {dimension['id']}: {dimension['status']}")
        for matrix in result["matrix_evidence"]:
            counts = []
            if matrix.get("vector_count") is not None:
                counts.append(f"vectors={matrix['vector_count']}")
            if matrix.get("wire_vector_count") is not None:
                counts.append(f"wire={matrix['wire_vector_count']}")
            if matrix.get("arithmetic_vector_count") is not None:
                counts.append(f"arithmetic={matrix['arithmetic_vector_count']}")
            for edition, details in matrix.get("editions", {}).items():
                if (
                    isinstance(details, dict)
                    and details.get("vector_count") is not None
                ):
                    counts.append(f"{edition}={details['vector_count']}")
            suffix = f" ({', '.join(counts)})" if counts else ""
            print(f"  matrix/{matrix['id']}{suffix}")
    return 1 if result["status"] == "fail" else 0


if __name__ == "__main__":
    raise SystemExit(main())
