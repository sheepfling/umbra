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
from typing import Literal, TypedDict, cast

# TODO this seems super fragile.. we should be using a proper library to read C++ ? lizard ?? This should have been flagged by alphus

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
HEADER_ROOT = REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "include"
DEFAULT_OUTPUT = REPOSITORY_ROOT / "compliance" / "standards" / "binding-inventory.json"
HEADER_DIGESTS = (
    REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "header-digests.json"
)

_CLASSES = (
    ("rti1516_2025::RTIambassador", "RTI/RTIambassador.h", 181),
    ("rti1516_2025::FederateAmbassador", "RTI/FederateAmbassador.h", 63),
)
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//[^\n]*")
# A pure-virtual declaration cannot cross a semicolon. This excludes the
# non-pure virtual destructors that precede the service declarations.
_PURE_VIRTUAL = re.compile(
    r"\bvirtual\b(?P<declaration>(?:(?!;).)*?)=\s*0\s*;", re.DOTALL
)


class _DigestEntryInput(TypedDict, total=False):
    path: object
    sha256: object


class _DigestManifestInput(TypedDict, total=False):
    schema_version: object
    files: object


class BindingMember(TypedDict):
    ordinal: int
    name: str
    signature: str


BindingClass = TypedDict(
    "BindingClass",
    {
        "class": str,
        "header": str,
        "sha256": str,
        "pure_virtual_member_count": int,
        "members": list[BindingMember],
    },
)


class BindingInventory(TypedDict):
    schema_version: Literal[1]
    kind: Literal["ieee1516.1-2025-binding-inventory"]
    notes: str
    classes: list[BindingClass]
    total_pure_virtual_member_count: int


def _compact(value: str) -> str:
    return " ".join(value.split())


def _member_name(signature: str) -> str:
    before_parameters = signature.split("(", maxsplit=1)[0].rstrip()
    match = re.search(r"([~A-Za-z_]\w*)$", before_parameters)
    if match is None:
        raise ValueError(f"could not identify member name in {signature!r}")
    return match.group(1)


def _pure_virtual_members(header_path: Path) -> list[BindingMember]:
    source = header_path.read_text(encoding="utf-8")
    source = _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", source))
    members: list[BindingMember] = []
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
        value: object = json.loads(HEADER_DIGESTS.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read header digest manifest: {error}") from error

    if not isinstance(value, dict):
        raise TypeError("header digest manifest must contain a JSON object")
    manifest = cast(_DigestManifestInput, value)
    files = manifest.get("files")
    if manifest.get("schema_version") != 1:
        raise ValueError("header digest manifest has an unsupported shape")
    if not isinstance(files, list):
        raise TypeError("header digest manifest.files must be an array")
    digests: dict[str, str] = {}
    for index, value in enumerate(files):
        context = f"header digest manifest.files[{index}]"
        if not isinstance(value, dict):
            raise TypeError(f"{context} must be a JSON object")
        entry = cast(_DigestEntryInput, value)
        path = entry.get("path")
        digest = entry.get("sha256")
        if not isinstance(path, str) or not path:
            raise ValueError(f"{context}.path must be a non-empty string")
        if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
            raise ValueError(f"{context}.sha256 must be a lowercase SHA-256 digest")
        if path in digests:
            raise ValueError(f"{context}.path duplicates {path!r}")
        digests[path] = digest
    return digests


def build_inventory() -> BindingInventory:
    digests = _digest_by_path()
    classes: list[BindingClass] = []
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
            raise ValueError(
                f"{relative_header}: digest does not match the committed baseline"
            )
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


def _serialized(value: BindingInventory) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--check", action="store_true", help="Fail if the committed inventory drifts"
    )
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
    except (OSError, TypeError, ValueError) as error:
        print(f"binding inventory: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
