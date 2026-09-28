#!/usr/bin/env python3
"""Survey periodic HLAfederate MIM attributes referenced by the portable C++ TCK.

This is a lexical coverage aid, not evidence that a matching scenario asserts
the MIM semantics. Review each listed scenario before treating an attribute as
covered; attributes absent from the source are candidate gaps, not test plans.
The script uses only the Python standard library and scans both shared TCK
scenario source and portable dispatch source.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MIM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)
DEFAULT_SOURCES = [
    ROOT / "packages" / "hla-rti-cpp-tck" / "src" / "main.cpp",
    ROOT / "packages" / "hla-rti-cpp-tck" / "src" / "portable_tck.cpp",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mim", type=Path, default=DEFAULT_MIM)
    parser.add_argument(
        "--source",
        type=Path,
        action="append",
        help="C++ TCK source to scan; repeat to replace the default source set",
    )
    parser.add_argument(
        "--details",
        action="store_true",
        help="Include MIM semantics and every lexical source location",
    )
    return parser.parse_args()


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def direct_child_text(element: ET.Element, name: str) -> str:
    for child in element:
        if local_name(child.tag) == name:
            return (child.text or "").strip()
    return ""


def periodic_federate_attributes(mim_path: Path) -> list[dict[str, str]]:
    root = ET.parse(mim_path).getroot()
    federate_classes = [
        element
        for element in root.iter()
        if local_name(element.tag) == "objectClass"
        and direct_child_text(element, "name") == "HLAfederate"
    ]
    if len(federate_classes) != 1:
        raise ValueError(
            "Expected exactly one standard MIM HLAfederate object class; "
            f"found {len(federate_classes)}"
        )

    attributes = []
    for element in federate_classes[0]:
        if local_name(element.tag) != "attribute":
            continue
        if direct_child_text(element, "updateType").casefold() != "periodic":
            continue
        name = direct_child_text(element, "name")
        if name:
            attributes.append(
                {
                    "name": name,
                    "data_type": direct_child_text(element, "dataType"),
                    "update_condition": direct_child_text(element, "updateCondition"),
                    "semantics": " ".join(direct_child_text(element, "semantics").split()),
                }
            )
    return attributes


def scan_source(
    source_path: Path,
    attributes: set[str],
) -> dict[str, list[dict[str, object]]]:
    uses: dict[str, list[dict[str, object]]] = {name: [] for name in attributes}
    try:
        source_label = source_path.relative_to(ROOT).as_posix()
    except ValueError:
        source_label = source_path.as_posix()
    for line_number, line in enumerate(
        source_path.read_text(encoding="utf-8").splitlines(), 1
    ):
        for name in attributes:
            if re.search(rf"\b{re.escape(name)}\b", line):
                uses[name].append(
                    {
                        "source": source_label,
                        "line": line_number,
                    }
                )
    return uses


def main() -> int:
    args = parse_args()
    sources = args.source if args.source else DEFAULT_SOURCES
    attributes = periodic_federate_attributes(args.mim)

    combined_uses: dict[str, list[dict[str, object]]] = {
        attribute["name"]: [] for attribute in attributes
    }
    for source in sources:
        source_path = source if source.is_absolute() else ROOT / source
        source_uses = scan_source(source_path, set(combined_uses))
        for name, uses in source_uses.items():
            combined_uses[name].extend(uses)

    rows = []
    for attribute in attributes:
        name = attribute["name"]
        rows.append(
            {
                **attribute,
                "source_use_count": len(combined_uses[name]),
                "source_uses": combined_uses[name],
            }
        )
    unreferenced = [row["name"] for row in rows if not row["source_uses"]]
    report = {
        "mim": args.mim.as_posix(),
        "sources": [source.as_posix() for source in sources],
        "periodic_hla_federate_attribute_count": len(rows),
        "lexically_referenced_count": len(rows) - len(unreferenced),
        "unreferenced_attributes": unreferenced,
        "lexically_referenced_attributes": [
            {"name": row["name"], "source_use_count": row["source_use_count"]}
            for row in rows
            if row["source_uses"]
        ],
        "interpretation": (
            "Lexical source references are survey hints only; inspect the "
            "relevant scenario assertions before treating an attribute as covered."
        ),
    }
    if args.details:
        report["attributes"] = rows
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
