"""Verify Python exports for the official IEEE 1516e Java type surface."""

from __future__ import annotations

import argparse
import importlib
import json
import sys
from collections.abc import Iterable
from pathlib import Path
from typing import Literal, TypeAlias, TypedDict, cast

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PYTHON_ROOT = ROOT / "packages" / "umbra-rti-api" / "src"

ModuleName: TypeAlias = Literal["root", "encoding", "exceptions", "time"]
NestedCallbackAliasName: TypeAlias = Literal[
    "FederateAmbassador.SupplementalReflectInfo",
    "FederateAmbassador.SupplementalReceiveInfo",
    "FederateAmbassador.SupplementalRemoveInfo",
]
ModuleDifferenceField: TypeAlias = Literal["missing_attributes", "not_exported"]


class ModuleInventory(TypedDict):
    java_type_count: int
    python_export_count: int
    missing_attributes: list[str]
    not_exported: list[str]
    python_helpers: list[str]


class PythonTypeSurfaceReport(TypedDict):
    standard: Literal["IEEE 1516.1-2010"]
    java_package: Literal["hla.rti1516e"]
    modules: dict[ModuleName, ModuleInventory]
    nested_callback_aliases: dict[NestedCallbackAliasName, bool]


def _java_names(java_root: Path, relative: Path) -> list[str]:
    path = java_root / relative
    return sorted(item.stem for item in path.glob("*.java"))


def _module_inventory(
    java_root: Path, module_name: str, relative: Path
) -> ModuleInventory:
    module = importlib.import_module(module_name)
    expected = _java_names(java_root, relative)
    exported_names = cast(Iterable[str], getattr(module, "__all__", ()))
    exported = set(exported_names)
    return {
        "java_type_count": len(expected),
        "python_export_count": len(exported),
        "missing_attributes": [name for name in expected if not hasattr(module, name)],
        "not_exported": [name for name in expected if name not in exported],
        "python_helpers": sorted(exported - set(expected)),
    }


def verify(java_root: Path, python_root: Path) -> PythonTypeSurfaceReport:
    sys.path.insert(0, str(python_root))
    module_specs: dict[ModuleName, tuple[str, Path]] = {
        "root": ("hla.rti1516e", Path("hla") / "rti1516e"),
        "encoding": ("hla.rti1516e.encoding", Path("hla") / "rti1516e" / "encoding"),
        "exceptions": (
            "hla.rti1516e.exceptions",
            Path("hla") / "rti1516e" / "exceptions",
        ),
        "time": ("hla.rti1516e.time", Path("hla") / "rti1516e" / "time"),
    }
    inventory: dict[ModuleName, ModuleInventory] = {
        label: _module_inventory(java_root, module_name, relative)
        for label, (module_name, relative) in module_specs.items()
    }
    root = importlib.import_module("hla.rti1516e")
    nested_aliases: dict[NestedCallbackAliasName, bool] = {
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
    result: PythonTypeSurfaceReport = {
        "standard": "IEEE 1516.1-2010",
        "java_package": "hla.rti1516e",
        "modules": inventory,
        "nested_callback_aliases": nested_aliases,
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--java-root",
        type=Path,
        required=True,
        help="root containing hla/rti1516e/*.java",
    )
    parser.add_argument("--python-root", type=Path, default=DEFAULT_PYTHON_ROOT)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        inventory = verify(args.java_root, args.python_root)
        findings: list[str] = []
        for module_name, data in inventory["modules"].items():
            fields: tuple[ModuleDifferenceField, ...] = (
                "missing_attributes",
                "not_exported",
            )
            for key in fields:
                if data[key]:
                    findings.append(f"{module_name}.{key}: {data[key]}")
        for alias, present in inventory["nested_callback_aliases"].items():
            if not present:
                findings.append(f"missing Java-shaped nested callback alias: {alias}")
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(
                json.dumps(inventory, indent=2) + "\n", encoding="utf-8"
            )
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
    except (
        OSError,
        ImportError,
        AttributeError,
        ValueError,
        json.JSONDecodeError,
    ) as error:
        print(
            f"IEEE 1516e Python type-surface verification: FAIL: {error}",
            file=sys.stderr,
        )
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
