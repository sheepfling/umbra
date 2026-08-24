"""Generate the reviewable abstract-member inventory for the IEEE C++ binding.

This is intentionally a narrow, dependency-free parser for the unmodified
IEEE headers that Umbra vendors. It is not a C++ parser and must not be used to
generate implementation code. Its job is to make every pure virtual member
visible to review and to fail when the pinned header baseline changes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
HEADER_ROOT = REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "include"
DEFAULT_OUTPUT = REPOSITORY_ROOT / "compliance" / "standards" / "binding-inventory.json"
HEADER_DIGESTS = REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "header-digests.json"

_CLASSES = (
    ("rti1516_2025::RTIambassador", "RTI/RTIambassador.h", 181),
    ("rti1516_2025::FederateAmbassador", "RTI/FederateAmbassador.h", 63),
)
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//[^\n]*")
# A pure-virtual declaration cannot cross a semicolon. This excludes the
# non-pure virtual destructors that precede the service declarations.
_PURE_VIRTUAL = re.compile(r"\bvirtual\b(?P<declaration>(?:(?!;).)*?)=\s*0\s*;", re.DOTALL)


def _compact(value: str) -> str:
    return " ".join(value.split())


def _member_name(signature: str) -> str:
    before_parameters = signature.split("(", maxsplit=1)[0].rstrip()
    match = re.search(r"([~A-Za-z_]\w*)$", before_parameters)
    if match is None:
        raise ValueError(f"could not identify member name in {signature!r}")
    return match.group(1)


def _pure_virtual_members(header_path: Path) -> list[dict[str, object]]:
    source = header_path.read_text(encoding="utf-8")
    source = _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", source))
    members: list[dict[str, object]] = []
    for ordinal, match in enumerate(_PURE_VIRTUAL.finditer(source), start=1):
        signature = _compact(match.group("declaration"))
        members.append(
            {
                "ordinal": ordinal,
                "name": _member_name(signature),
                "signature": signature,
            }
        )
    return members


def _digest_by_path() -> dict[str, str]:
    try:
        manifest = json.loads(HEADER_DIGESTS.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read header digest manifest: {error}") from error
    files = manifest.get("files")
    if manifest.get("schema_version") != 1 or not isinstance(files, list):
        raise ValueError("header digest manifest has an unsupported shape")
    digests: dict[str, str] = {}
    for entry in files:
        if not isinstance(entry, dict):
            raise ValueError("header digest manifest has a non-object entry")
        path = entry.get("path")
        digest = entry.get("sha256")
        if not isinstance(path, str) or not isinstance(digest, str):
            raise ValueError("header digest manifest has an invalid entry")
        digests[path] = digest
    return digests


def build_inventory() -> dict[str, Any]:
    digests = _digest_by_path()
    classes: list[dict[str, object]] = []
    for class_name, relative_header, expected_count in _CLASSES:
        path = HEADER_ROOT / relative_header
        members = _pure_virtual_members(path)
        if len(members) != expected_count:
            raise ValueError(
                f"{relative_header}: expected {expected_count} pure virtual members, "
                f"found {len(members)}"
            )
        actual_digest = hashlib.sha256(path.read_bytes()).hexdigest()
        expected_digest = digests.get(relative_header)
        if actual_digest != expected_digest:
            raise ValueError(f"{relative_header}: digest does not match the committed baseline")
        classes.append(
            {
                "class": class_name,
                "header": relative_header,
                "sha256": actual_digest,
                "pure_virtual_member_count": len(members),
                "members": members,
            }
        )
    return {
        "schema_version": 1,
        "kind": "ieee1516.1-2025-binding-inventory",
        "notes": (
            "Review inventory only. It does not assert an implementation, "
            "test result, or conformance status."
        ),
        "classes": classes,
        "total_pure_virtual_member_count": sum(
            entry["pure_virtual_member_count"] for entry in classes
        ),
    }


def _serialized(value: dict[str, Any]) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="Fail if the committed inventory drifts")
    args = parser.parse_args()

    try:
        expected = _serialized(build_inventory())
        output = args.output.resolve()
        if args.check:
            actual = output.read_text(encoding="utf-8")
            if actual != expected:
                print(
                    f"binding inventory drifted: regenerate with {Path(__file__).name}",
                    file=sys.stderr,
                )
                return 1
            print("IEEE binding inventory: PASS (244 pure virtual members)")
            return 0
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(expected, encoding="utf-8")
        print(f"wrote IEEE binding inventory to {output}")
        return 0
    except (OSError, ValueError) as error:
        print(f"binding inventory: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
