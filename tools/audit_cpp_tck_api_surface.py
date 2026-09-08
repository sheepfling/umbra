#!/usr/bin/env python3
"""Audit official IEEE C++ RTIambassador coverage in the portable TCK.

The auditor is deliberately provider-independent. It reads the official API
header, the portable scenario catalog, and the portable source tree, then
reports public RTIambassador methods that are absent from the catalog or the
source. It does not configure, load, or execute an RTI implementation.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


METHOD_DECLARATION = re.compile(
    r"\bvirtual\b[^;\n]*?\b([A-Za-z_]\w*)\s*\("
)
METHOD_REFERENCE = re.compile(r"\.\s*([A-Za-z_]\w*)\s*\(")
FUNCTION_REFERENCE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--header",
        type=Path,
        default=Path("third_party/ieee1516.1-2025/include/RTI/RTIambassador.h"),
    )
    parser.add_argument(
        "--catalog",
        type=Path,
        default=Path("compliance/catalogs/cpp-tck-scenario-catalog.json"),
    )
    parser.add_argument(
        "--federate-header",
        type=Path,
        default=Path("third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h"),
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=Path("packages/hla-rti-cpp-tck/src/main.cpp"),
    )
    return parser.parse_args()


def official_methods(header_text: str) -> set[str]:
    methods = set(METHOD_DECLARATION.findall(header_text))
    methods.discard("RTIambassador")
    methods.discard("FederateAmbassador")
    return methods


def catalog_methods(catalog: dict, prefix: str) -> set[str]:
    methods: set[str] = set()
    for scenario in catalog.get("scenarios", []):
        for api_method in scenario.get("api_methods", []):
            if not api_method.startswith(prefix + "."):
                continue
            method = api_method.split(".", 1)[1]
            methods.add(method.split("(", 1)[0])
    return methods


def source_methods(source_text: str) -> set[str]:
    return set(METHOD_REFERENCE.findall(source_text))


def main() -> int:
    args = parse_args()
    header_text = args.header.read_text(encoding="utf-8")
    federate_header_text = args.federate_header.read_text(encoding="utf-8")
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    source_text = args.source.read_text(encoding="utf-8")

    official = official_methods(header_text)
    official_callbacks = official_methods(federate_header_text)
    catalog_covered = catalog_methods(catalog, "RTIambassador")
    catalog_callbacks = catalog_methods(catalog, "FederateAmbassador")
    source_covered = source_methods(source_text)
    source_functions = set(FUNCTION_REFERENCE.findall(source_text))
    missing_catalog = sorted(official - catalog_covered)
    missing_source = sorted(official - source_covered)
    missing_callback_catalog = sorted(official_callbacks - catalog_callbacks)
    missing_callback_source = sorted(official_callbacks - source_functions)
    report = {
        "header": args.header.as_posix(),
        "catalog": args.catalog.as_posix(),
        "source": args.source.as_posix(),
        "federate_header": args.federate_header.as_posix(),
        "official_rti_ambassador_method_count": len(official),
        "catalog_covered_method_count": len(official & catalog_covered),
        "source_referenced_method_count": len(official & source_covered),
        "missing_from_catalog": missing_catalog,
        "missing_from_source": missing_source,
        "official_federate_ambassador_callback_count": len(official_callbacks),
        "catalog_covered_callback_count": len(official_callbacks & catalog_callbacks),
        "source_referenced_callback_count": len(official_callbacks & source_functions),
        "missing_callbacks_from_catalog": missing_callback_catalog,
        "missing_callbacks_from_source": missing_callback_source,
        "scenario_count": len(catalog.get("scenarios", [])),
        "valid": not missing_catalog
        and not missing_source
        and not missing_callback_catalog
        and not missing_callback_source,
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["valid"] else 1


if __name__ == "__main__":
    sys.exit(main())
