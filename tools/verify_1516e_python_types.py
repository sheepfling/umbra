"""Verify Python exports for the official IEEE 1516e Java type surface."""

from __future__ import annotations

import argparse
import importlib
import json
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PYTHON_ROOT = ROOT / "packages" / "umbra-rti-api" / "src"


def _java_names(java_root: Path, relative: str) -> list[str]:
    path = java_root / relative
    return sorted(item.stem for item in path.glob("*.java"))


def _module_inventory(java_root: Path, module_name: str, relative: str) -> dict[str, Any]:
    module = importlib.import_module(module_name)
    expected = _java_names(java_root, relative)
    exported = set(getattr(module, "__all__", ()))
    return {
        "java_type_count": len(expected),
        "python_export_count": len(exported),
        "missing_attributes": [name for name in expected if not hasattr(module, name)],
        "not_exported": [name for name in expected if name not in exported],
        "python_helpers": sorted(exported - set(expected)),
    }


def verify(java_root: Path, python_root: Path) -> dict[str, Any]:
    sys.path.insert(0, str(python_root))
    modules = {
        "root": ("hla.rti1516e", Path("hla") / "rti1516e"),
        "encoding": ("hla.rti1516e.encoding", Path("hla") / "rti1516e" / "encoding"),
        "exceptions": ("hla.rti1516e.exceptions", Path("hla") / "rti1516e" / "exceptions"),
        "time": ("hla.rti1516e.time", Path("hla") / "rti1516e" / "time"),
    }
    inventory = {
        label: _module_inventory(java_root, module_name, relative)
        for label, (module_name, relative) in modules.items()
    }
    root = importlib.import_module("hla.rti1516e")
    nested_aliases = {
        "FederateAmbassador.SupplementalReflectInfo": getattr(
            root.FederateAmbassador, "SupplementalReflectInfo", None
        )
        is root.SupplementalReflectInfo,
        "FederateAmbassador.SupplementalReceiveInfo": getattr(
            root.FederateAmbassador, "SupplementalReceiveInfo", None
        )
        is root.SupplementalReceiveInfo,
        "FederateAmbassador.SupplementalRemoveInfo": getattr(
            root.FederateAmbassador, "SupplementalRemoveInfo", None
        )
        is root.SupplementalRemoveInfo,
    }
    result: dict[str, Any] = {
        "standard": "IEEE 1516.1-2010",
        "java_package": "hla.rti1516e",
        "modules": inventory,
        "nested_callback_aliases": nested_aliases,
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--java-root", type=Path, required=True, help="root containing hla/rti1516e/*.java")
    parser.add_argument("--python-root", type=Path, default=DEFAULT_PYTHON_ROOT)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        inventory = verify(args.java_root, args.python_root)
        findings: list[str] = []
        for module_name, data in inventory["modules"].items():
            for key in ("missing_attributes", "not_exported"):
                if data[key]:
                    findings.append(f"{module_name}.{key}: {data[key]}")
        for alias, present in inventory["nested_callback_aliases"].items():
            if not present:
                findings.append(f"missing Java-shaped nested callback alias: {alias}")
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(inventory, indent=2) + "\n", encoding="utf-8")
        if findings:
            print("IEEE 1516e Python type-surface verification: FAIL", file=sys.stderr)
            for finding in findings:
                print(f"  {finding}", file=sys.stderr)
            return 1
        print("IEEE 1516e Python type-surface verification: PASS")
        for module_name, data in inventory["modules"].items():
            print(
                f"  {module_name}: Java={data['java_type_count']}, "
                f"Python exports={data['python_export_count']}"
            )
        return 0
    except (OSError, ImportError, AttributeError, ValueError, json.JSONDecodeError) as error:
        print(f"IEEE 1516e Python type-surface verification: FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
