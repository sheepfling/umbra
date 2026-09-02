"""Generate a cross-language IEEE 1516e surface inventory.

The 2010 C++ and Java APIs are related, but they are not textually identical:
the C++ API exposes handle decoders and a few legacy names while Java exposes
factories and passive-subscription overloads.  This tool keeps that distinction
explicit and verifies that the checked-in Python contract mirrors the exact
Java method groups it claims to implement.

The official API sources are supplied by the caller because they are not
repository assets.  The generated JSON is an evidence artifact, not a source
of implementation behavior.
"""

from __future__ import annotations

import argparse
import ast
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PYTHON_CONTRACT = (
    ROOT / "packages" / "umbra-rti-api" / "src" / "hla" / "rti1516e" / "contracts.py"
)


def _strip_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", source)


def _cpp_method_names(source: str, class_name: str) -> list[str]:
    """Return unique pure/virtual method names in declaration order."""

    source = _strip_comments(source)
    matches = re.findall(
        r"\bvirtual\s+(?:[^;{}()]|\([^)]*\))*?\b([A-Za-z_]\w*)\s*\(",
        source,
        flags=re.DOTALL,
    )
    return list(dict.fromkeys(name for name in matches if name != class_name))


def _top_level_java_body(source: str) -> str:
    """Extract the interface body while excluding nested callback records."""

    opening = source.find("{")
    if opening < 0:
        return ""
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
    return "".join(body)


def _java_method_names(source: str) -> list[str]:
    body = _top_level_java_body(_strip_comments(source))
    matches = re.findall(
        r"(?m)^\s*(?:public\s+)?(?:static\s+)?(?:default\s+)?"
        r"(?:<[A-Za-z_$][^;{}()]*>\s+)?"
        r"[A-Za-z_$][\w$]*(?:\s*<[^;{}()]*>)?(?:\s*\[\])*\s+"
        r"([A-Za-z_$][\w$]*)\s*\([^)]*\)\s*(?=(?:throws\b|;))",
        body,
    )
    return list(dict.fromkeys(matches))


def _python_contract_names(path: Path, assignment: str) -> list[str]:
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    for node in tree.body:
        if not isinstance(node, ast.Assign):
            continue
        if not any(isinstance(target, ast.Name) and target.id == assignment for target in node.targets):
            continue
        value = ast.literal_eval(node.value)
        if not isinstance(value, tuple) or not all(isinstance(item, str) for item in value):
            raise ValueError(f"{assignment} is not a tuple of method names")
        return list(value)
    raise ValueError(f"Python contract is missing {assignment}")


def _surface(cpp_root: Path, java_root: Path, python_contract: Path) -> dict[str, Any]:
    cpp_rti = _cpp_method_names(
        (cpp_root / "RTI" / "RTIambassador.h").read_text(encoding="utf-8"),
        "RTIambassador",
    )
    cpp_federate = _cpp_method_names(
        (cpp_root / "RTI" / "FederateAmbassador.h").read_text(encoding="utf-8"),
        "FederateAmbassador",
    )
    java_rti = _java_method_names(
        (java_root / "hla" / "rti1516e" / "RTIambassador.java").read_text(encoding="utf-8")
    )
    java_federate = _java_method_names(
        (java_root / "hla" / "rti1516e" / "FederateAmbassador.java").read_text(encoding="utf-8")
    )
    python_rti = _python_contract_names(python_contract, "RTIAMBASSADOR_METHODS")
    python_federate = _python_contract_names(python_contract, "FEDERATE_AMBASSADOR_METHODS")

    def diff(left: list[str], right: list[str]) -> list[str]:
        return sorted(set(left) - set(right))

    result: dict[str, Any] = {
        "standard": "IEEE 1516.1-2010",
        "namespaces": {
            "cpp": "rti1516e",
            "java": "hla.rti1516e",
            "python": "hla.rti1516e",
        },
        "cpp": {
            "rti_method_count": len(cpp_rti),
            "federate_method_count": len(cpp_federate),
            "rti_methods": cpp_rti,
            "federate_methods": cpp_federate,
        },
        "java": {
            "rti_method_count": len(java_rti),
            "federate_method_count": len(java_federate),
            "rti_methods": java_rti,
            "federate_methods": java_federate,
        },
        "python": {
            "rti_method_count": len(python_rti),
            "federate_method_count": len(python_federate),
            "rti_methods": python_rti,
            "federate_methods": python_federate,
        },
        "differences": {
            "cpp_rti_only": diff(cpp_rti, java_rti),
            "java_rti_only": diff(java_rti, cpp_rti),
            "cpp_federate_only": diff(cpp_federate, java_federate),
            "java_federate_only": diff(java_federate, cpp_federate),
            "python_rti_missing_java": diff(java_rti, python_rti),
            "python_rti_extra": diff(python_rti, java_rti),
            "python_federate_missing_java": diff(java_federate, python_federate),
            "python_federate_extra": diff(python_federate, java_federate),
        },
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpp-root", type=Path, required=True, help="root containing RTI/*.h")
    parser.add_argument("--java-root", type=Path, required=True, help="root containing hla/rti1516e/*.java")
    parser.add_argument("--python-contract", type=Path, default=DEFAULT_PYTHON_CONTRACT)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    try:
        inventory = _surface(args.cpp_root, args.java_root, args.python_contract)
        differences = inventory["differences"]
        python_differences = [
            differences["python_rti_missing_java"],
            differences["python_rti_extra"],
            differences["python_federate_missing_java"],
            differences["python_federate_extra"],
        ]
        if any(python_differences):
            raise ValueError("Python contract does not exactly mirror the Java method groups")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(inventory, indent=2) + "\n", encoding="utf-8")
        print("IEEE 1516e cross-language surface inventory: PASS")
        print(
            "  unique methods: "
            f"C++ RTI={inventory['cpp']['rti_method_count']}, "
            f"Java/Python RTI={inventory['java']['rti_method_count']}"
        )
        print(
            "  unique callbacks: "
            f"C++={inventory['cpp']['federate_method_count']}, "
            f"Java/Python={inventory['java']['federate_method_count']}"
        )
        print(
            "  intentional C++/Java name differences: "
            f"RTI C++-only={len(differences['cpp_rti_only'])}, "
            f"Java-only={len(differences['java_rti_only'])}; "
            f"callbacks C++-only={len(differences['cpp_federate_only'])}, "
            f"Java-only={len(differences['java_federate_only'])}"
        )
        print(f"  output: {args.output}")
        return 0
    except (OSError, ValueError, SyntaxError, json.JSONDecodeError) as error:
        print(f"IEEE 1516e cross-language surface inventory: FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
