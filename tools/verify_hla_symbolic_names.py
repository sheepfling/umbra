"""Reject raw string literals at dynamic HLA handle-lookup boundaries."""

from __future__ import annotations

import argparse
import ast
import re
import sys
from pathlib import Path

LOOKUP_ARGUMENTS = {
    "getObjectClassHandle": 0,
    "getInteractionClassHandle": 0,
    "getAttributeHandle": 1,
    "getParameterHandle": 1,
    "getDimensionHandle": 0,
    "getTransportationTypeHandle": 0,
}
CPP_LOOKUP = re.compile(
    r"\b(?:getObjectClassHandle|getInteractionClassHandle|"
    r"getAttributeHandle|getParameterHandle|getDimensionHandle|"
    r"getTransportationTypeHandle)\s*\("
)
CPP_STRING = re.compile(r"(?:u8|u|U|L)?\"(?:\\.|[^\"\\])*\"")


def _python_files(root: Path) -> list[Path]:
    directories = (
        root / "packages" / "umbra-rti-native" / "tests",
        root / "packages" / "umbra-rti-native" / "src",
        root / "packages" / "umbra-rti-jpype" / "tests",
        root / "packages" / "umbra-rti-jpype" / "src",
        root / "packages" / "umbra-rti-test-support" / "src",
    )
    return sorted(
        {
            path
            for directory in directories
            for path in directory.rglob("*.py")
            if "build" not in path.parts and "site-packages" not in path.parts
        }
    )


def _cpp_files(root: Path) -> list[Path]:
    return sorted(
        path
        for directory in (root / "cpp" / "src", root / "cpp" / "tests")
        for path in directory.rglob("*")
        if path.suffix in {".cpp", ".hpp", ".h"}
        and "data" not in path.parts
        and ".build" not in path.parts
    )


def _python_violations(path: Path) -> list[str]:
    try:
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    except (OSError, SyntaxError, UnicodeDecodeError) as error:
        return [f"{path}: unable to parse Python source: {error}"]

    violations: list[str] = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue
        method = getattr(node.func, "attr", None)
        argument_index = LOOKUP_ARGUMENTS.get(method)
        if argument_index is None or len(node.args) <= argument_index:
            continue
        argument = node.args[argument_index]
        if isinstance(argument, (ast.Constant, ast.JoinedStr)):
            violations.append(
                f"{path}:{argument.lineno}: {method} receives a raw string expression; "
                "use a named HLA fixture/MOM value"
            )
    return violations


def _matching_parenthesis(text: str, opening: int) -> int:
    depth = 0
    index = opening
    quote: str | None = None
    escaped = False
    while index < len(text):
        character = text[index]
        if quote is not None:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == quote:
                quote = None
        elif character in {'"', "'"}:
            quote = character
        elif character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth == 0:
                return index
        index += 1
    return len(text)


def _cpp_violations(path: Path) -> list[str]:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as error:
        return [f"{path}: unable to read C++ source: {error}"]

    violations: list[str] = []
    for match in CPP_LOOKUP.finditer(text):
        closing = _matching_parenthesis(text, match.end() - 1)
        call_text = text[match.start() : closing + 1]
        if not CPP_STRING.search(call_text):
            continue
        line = text.count("\n", 0, match.start()) + 1
        violations.append(
            f"{path}:{line}: HLA handle lookup receives a raw string literal; "
            "use a named HLA fixture/MOM value"
        )
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repository root (defaults to the parent of tools/)",
    )
    args = parser.parse_args()
    root = args.root.resolve()

    violations = [
        violation
        for path in _python_files(root)
        for violation in _python_violations(path)
    ]
    violations.extend(
        violation
        for path in _cpp_files(root)
        for violation in _cpp_violations(path)
    )
    if violations:
        print("HLA symbolic-name verification failed:", file=sys.stderr)
        print("\n".join(violations), file=sys.stderr)
        return 1

    print(
        "HLA symbolic-name verification passed "
        f"({len(_python_files(root))} Python files, "
        f"{len(_cpp_files(root))} C++ files checked)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
