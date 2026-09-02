"""Verify the IEEE 1516e provider boundaries.

The 2010 provider is deliberately isolated from the 2025 native/JNI route.
Its bounded C++/pybind and JNI artifacts are allowed to mention `rti1516e`,
while the 2025 native/JNI source trees must remain edition-pure.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections.abc import Iterable
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "compliance" / "standards" / "ieee1516e-2010-artifact-manifest.json"


_ENTRY_POINT_SECTION = re.compile(r'^\[project\.entry-points\."([^"]+)"\]\s*$')
_ENTRY_POINT_VALUE = re.compile(r'^([A-Za-z0-9_.-]+)\s*=\s*"([^"]+)"\s*$')


def _entry_points(path: Path) -> dict[str, dict[str, str]]:
    """Read the small entry-point subset without adding a TOML dependency."""

    groups: dict[str, dict[str, str]] = {}
    current: dict[str, str] | None = None
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        section = _ENTRY_POINT_SECTION.match(line)
        if section:
            current = groups.setdefault(section.group(1), {})
            continue
        if line.startswith("["):
            current = None
            continue
        if current is not None:
            value = _ENTRY_POINT_VALUE.match(line)
            if value:
                current[value.group(1)] = value.group(2)
    return groups


def _source_files(root: Path) -> Iterable[Path]:
    if not root.exists():
        return ()
    if root.is_file():
        return (root,)
    return (
        path
        for path in root.rglob("*")
        if path.is_file()
        and path.suffix.lower() in {".c", ".cc", ".cpp", ".h", ".hpp", ".java", ".py"}
    )


def _manifest(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise TypeError(f"{path} must contain a JSON object")
    return value


def verify() -> list[str]:
    findings: list[str] = []
    native_path = ROOT / "packages" / "umbra-rti-native" / "pyproject.toml"
    jpype_path = ROOT / "packages" / "umbra-rti-jpype" / "pyproject.toml"
    jni_2010_path = ROOT / "packages" / "umbra-rti-jni-2010"

    native_groups = _entry_points(native_path)
    if set(native_groups) != {
        "hla.rti1516_2025.factories",
        "hla.rti1516e.factories",
    }:
        findings.append(
            "native package must expose only the edition-specific 2025 and 2010 factory groups "
            f"(found {sorted(native_groups)})"
        )
    if native_groups.get("hla.rti1516_2025.factories", {}).get("Umbra") != (
        "umbra._native.rti1516_2025:UmbraRtiFactory"
    ):
        findings.append("native 2025 Umbra entry point changed unexpectedly")
    if native_groups.get("hla.rti1516e.factories", {}).get("UmbraNative2010") != (
        "umbra._native.rti1516e:Native2010RtiFactory"
    ):
        findings.append("native 2010 Umbra entry point changed unexpectedly")

    jpype_groups = _entry_points(jpype_path)
    providers_2010 = jpype_groups.get("hla.rti1516e.factories", {})
    if providers_2010.get("java-2010") != "umbra._java.rti1516e:Java2010RtiFactory":
        findings.append(
            "JPype package must expose java-2010 through hla.rti1516e.factories"
        )
    if "hla.rti1516_2025.factories" not in jpype_groups:
        findings.append("JPype package lost its 2025 provider entry-point group")
    if jpype_groups.get("hla.rti1516_2025.factories", {}).get("umbra-jni") != (
        "umbra._java.rti1516_2025:UmbraJniRtiFactory"
    ):
        findings.append("JPype package lost its 2025 Umbra JNI entry point")

    forbidden = re.compile(r"(?:rti1516e|hla[./]rti1516e|1516e)", re.IGNORECASE)
    for root, label in (
        (
            ROOT / "packages" / "umbra-rti-native" / "src" / "native_module.cpp",
            "native 2025 source",
        ),
        (
            ROOT
            / "packages"
            / "umbra-rti-native"
            / "src"
            / "umbra"
            / "_native"
            / "rti1516_2025",
            "native 2025 façade",
        ),
        (ROOT / "packages" / "umbra-rti-jni" / "src", "JNI 2025 source"),
    ):
        for path in _source_files(root):
            text = path.read_text(encoding="utf-8", errors="replace")
            if forbidden.search(text):
                findings.append(
                    f"{label} contains an accidental 1516e reference: {path}"
                )

    if not (jni_2010_path / "CMakeLists.txt").is_file():
        findings.append("the separate 2010 JNI package is missing its CMake boundary")
    if not (jni_2010_path / "src" / "native" / "umbra_rti_jni_2010.cpp").is_file():
        findings.append("the separate 2010 JNI package is missing its native bridge")
    service_root = (
        jni_2010_path / "src" / "main" / "resources" / "META-INF" / "services"
    )
    expected_descriptors = {
        "hla.rti1516e.RtiFactory": (
            "org.umbra.jni.rti1516e.NativeRtiFactory",
        ),
        "hla.rti1516e.LogicalTimeFactory": (
            "org.umbra.jni.rti1516e.NativeInteger64TimeFactory",
            "org.umbra.jni.rti1516e.NativeFloat64TimeFactory",
        ),
    }
    for name, expected in expected_descriptors.items():
        descriptor = service_root / name
        actual = tuple(
            line.strip()
            for line in descriptor.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ) if descriptor.is_file() else ()
        if actual != expected:
            findings.append(f"the 2010 JNI ServiceLoader descriptor {name} is missing or changed")

    manifest = _manifest(MANIFEST)
    scope = manifest.get("scope", {})
    if scope.get("primary_goal") != "surface-complete-bindable":
        findings.append("manifest does not pin the surface-complete-bindable goal")
    if scope.get("completion_rule") != (
        "A callable standard method counts as surface-bound when its name, overload shape, "
        "callback carrier, exception mapping, and provider load/link path are present. "
        "Invocation behavior is supplementary evidence; an explicit unsupported result "
        "is not a missing surface symbol."
    ):
        findings.append("manifest does not pin the surface-only completion rule")
    boundary = manifest.get("provider_boundaries", {})
    if boundary.get("python_2010_entry_point_group") != "hla.rti1516e.factories":
        findings.append("manifest does not pin the 2010 Python entry-point group")
    if boundary.get("python_2010_entry_point_alias") != "java-2010":
        findings.append("manifest does not pin the java-2010 provider alias")
    if boundary.get("python_2010_native_entry_point") != (
        "UmbraNative2010 / umbra._native.rti1516e:Native2010RtiFactory"
    ):
        findings.append(
            "manifest does not pin the direct native 2010 Python entry point"
        )
    if boundary.get("python_2010_time_factory_policy") != (
        "LogicalTimeFactoryFactory honors hla.rti1516e.time_factories entries and otherwise discovers "
        "getTimeFactory/getFloat64TimeFactory from the edition-specific RTI providers."
    ):
        findings.append(
            "manifest does not pin the 2010 logical-time factory discovery policy"
        )
    if boundary.get("native_2010_entry_points") != ["umbra::rti_2010"]:
        findings.append("manifest must pin the bounded native 2010 target")
    if boundary.get("jni_2010_java_packages") != ["org.umbra.jni.rti1516e"]:
        findings.append("manifest must pin the bounded JNI 2010 package")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--json", action="store_true", help="emit a machine-readable result"
    )
    args = parser.parse_args()
    try:
        findings = verify()
    except (OSError, TypeError, ValueError) as error:  # pragma: no cover
        findings = [str(error)]
    result = {
        "kind": "ieee1516e-provider-boundary-verification",
        "status": "fail" if findings else "pass",
        "findings": findings,
    }
    if args.json:
        print(json.dumps(result, indent=2))
    elif findings:
        print("IEEE 1516e provider boundary verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
    else:
        print("IEEE 1516e provider boundary verification: PASS")
        print("  Python 2010 provider: java-2010 / hla.rti1516e.factories")
        print("  Native Python 2010 provider: UmbraNative2010 / hla.rti1516e.factories")
        print(
            "  Native 2010 provider: umbra::rti_2010 (bounded encoder/time/federation/declaration/object/interaction/ownership/synchronization target)"
        )
        print("  JNI 2010 package: org.umbra.jni.rti1516e")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
