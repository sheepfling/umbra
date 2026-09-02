"""Verify locally staged IEEE 1516e (2010) API archives.

IEEE's download terms permit local/personal use but do not make the API source
archives repository assets. This verifier consumes the archives from a caller-
selected staging directory, checks the committed provenance manifest, and
prints the public namespace/surface facts used to generate the Python route.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
from zipfile import ZipFile

from generate_1516e_surface_inventory import _cpp_method_names, _java_method_names


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "compliance" / "standards" / "ieee1516e-2010-artifact-manifest.json"


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest().upper()


def archive_bytes(path: Path) -> bytes:
    return path.read_bytes()


def inner_archive(outer: bytes, name: str) -> ZipFile:
    import io

    with ZipFile(io.BytesIO(outer)) as archive:
        try:
            return ZipFile(io.BytesIO(archive.read(name)))
        except KeyError as error:
            raise ValueError(f"archive is missing {name}") from error


def text_entry(archive: ZipFile, suffix: str) -> str:
    matches = [name for name in archive.namelist() if name.endswith(suffix)]
    if len(matches) != 1:
        raise ValueError(f"expected one {suffix}, found {matches}")
    return archive.read(matches[0]).decode("utf-8")


def pure_virtual_count(source: str) -> int:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    source = re.sub(r"//[^\n]*", "", source)
    return len(re.findall(r"\bvirtual\b(?:(?!;).)*?=\s*0\s*;", source, flags=re.DOTALL))


def java_declaration_count(source: str) -> int:
    """Count top-level Java interface declarations (including overloads)."""

    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    source = re.sub(r"//[^\n]*", "", source)
    opening = source.find("{")
    if opening < 0:
        return 0
    depth = 1
    body: list[str] = []
    for character in source[opening + 1 :]:
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                break
        if depth == 1:
            body.append(character)
    return len(
        re.findall(
            r"(?m)^\s*(?:public\s+)?(?:static\s+)?(?:default\s+)?"
            r"(?:<[A-Za-z_$][^;{}()]*>\s+)?"
            r"[A-Za-z_$][\w$]*(?:\s*<[^;{}()]*>)?(?:\s*\[\])*\s+"
            r"[A-Za-z_$][\w$]*\s*\([^)]*\)\s*(?=(?:throws\b|;))",
            "".join(body),
        )
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--part1-archive", type=Path, required=True)
    parser.add_argument("--part2-archive", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args()

    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        if manifest.get("kind") != "ieee1516e-2010-artifact-manifest":
            raise ValueError("manifest is not an IEEE 1516e artifact manifest")
        expected_archives = {
            item["part"]: item["sha256"]
            for item in manifest["archives"]
        }
        part1 = archive_bytes(args.part1_archive)
        part2 = archive_bytes(args.part2_archive)
        if sha256_bytes(part1) != expected_archives["1516.1"]:
            raise ValueError("1516.1-2010 archive SHA-256 mismatch")
        if sha256_bytes(part2) != expected_archives["1516.2"]:
            raise ValueError("1516.2-2010 archive SHA-256 mismatch")

        cpp = inner_archive(part1, "1516.1-2010_downloads/IEEE1516-2010_C++_API.zip")
        java = inner_archive(part1, "1516.1-2010_downloads/IEEE1516-2010_Java_API.zip")
        cpp_source = text_entry(cpp, "RTI/RTIambassador.h")
        federate_source = text_entry(cpp, "RTI/FederateAmbassador.h")
        java_rti = text_entry(java, "hla/rti1516e/RTIambassador.java")
        java_federate = text_entry(java, "hla/rti1516e/FederateAmbassador.java")
        if "namespace rti1516e" not in cpp_source:
            raise ValueError("C++ API does not declare namespace rti1516e")
        if "package hla.rti1516e;" not in java_rti:
            raise ValueError("Java API does not declare package hla.rti1516e")
        expected_cpp = manifest["api_archives"]["cpp"]
        if pure_virtual_count(cpp_source) != expected_cpp["rti_ambassador_pure_virtual_count"]:
            raise ValueError("2010 C++ RTIambassador surface count mismatch")
        if pure_virtual_count(federate_source) != expected_cpp["federate_ambassador_pure_virtual_count"]:
            raise ValueError("2010 C++ FederateAmbassador surface count mismatch")
        expected_java = manifest["api_archives"]["java"]
        java_rti_count = java_declaration_count(java_rti)
        java_federate_count = java_declaration_count(java_federate)
        if java_rti_count != expected_java["rti_ambassador_declared_method_count"]:
            raise ValueError("2010 Java RTIambassador surface count mismatch")
        if java_federate_count != expected_java["federate_ambassador_declared_method_count"]:
            raise ValueError("2010 Java FederateAmbassador surface count mismatch")
        expected_surface = manifest.get("surface_inventory")
        if expected_surface:
            cpp_rti_names = _cpp_method_names(cpp_source, "RTIambassador")
            cpp_federate_names = _cpp_method_names(federate_source, "FederateAmbassador")
            java_rti_names = _java_method_names(java_rti)
            java_federate_names = _java_method_names(java_federate)
            if len(cpp_rti_names) != expected_surface["cpp_unique_rti_method_count"]:
                raise ValueError("2010 C++ unique RTI method inventory mismatch")
            if len(java_rti_names) != expected_surface["java_unique_rti_method_count"]:
                raise ValueError("2010 Java unique RTI method inventory mismatch")
            if len(cpp_federate_names) != expected_surface["cpp_unique_federate_callback_count"]:
                raise ValueError("2010 C++ unique callback inventory mismatch")
            if len(java_federate_names) != expected_surface["java_unique_federate_callback_count"]:
                raise ValueError("2010 Java unique callback inventory mismatch")
            if sorted(set(cpp_rti_names) - set(java_rti_names)) != sorted(expected_surface["cpp_rti_only"]):
                raise ValueError("2010 C++-only RTI method inventory mismatch")
            if sorted(set(java_rti_names) - set(cpp_rti_names)) != sorted(expected_surface["java_rti_only"]):
                raise ValueError("2010 Java-only RTI method inventory mismatch")
            if sorted(set(cpp_federate_names) - set(java_federate_names)):
                raise ValueError("2010 C++ callback name differences are not zero")
            if sorted(set(java_federate_names) - set(cpp_federate_names)):
                raise ValueError("2010 Java callback name differences are not zero")
        print("IEEE 1516e (2010) artifact provenance: PASS")
        print("  C++ namespace: rti1516e")
        print("  Java package: hla.rti1516e")
        print(
            "  C++ pure virtuals: "
            f"RTIambassador={pure_virtual_count(cpp_source)}, "
            f"FederateAmbassador={pure_virtual_count(federate_source)}"
        )
        print(
            "  Java declarations: "
            f"RTIambassador={java_rti_count}, FederateAmbassador={java_federate_count}"
        )
        if expected_surface:
            print(
                "  unique method names: "
                f"C++ RTI={len(cpp_rti_names)}, Java RTI={len(java_rti_names)}; "
                f"callbacks={len(java_federate_names)}"
            )
        return 0
    except (OSError, KeyError, ValueError, json.JSONDecodeError) as error:
        print(f"IEEE 1516e artifact verification: FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
