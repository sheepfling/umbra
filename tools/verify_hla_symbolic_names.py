"""Reject raw string literals at dynamic HLA handle-lookup boundaries."""

from __future__ import annotations

import argparse
import ast
import json
import re
import sys
from collections import Counter
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


def _violation_path(violation: str, root: Path) -> str | None:
    location = violation.split(": ", 1)[0]
    candidate = location.rsplit(":", 1)
    raw_path = candidate[0] if len(candidate) == 2 and candidate[1].isdigit() else location
    path = Path(raw_path)
    try:
        return path.resolve().relative_to(root).as_posix()
    except (OSError, ValueError):
        return None


def _load_baseline(path: Path) -> dict[str, int]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"unable to read symbolic-name baseline {path}: {error}") from error
    if not isinstance(data, dict) or data.get("schema_version") != 1:
        raise ValueError(f"unsupported symbolic-name baseline schema: {path}")
    counts = data.get("known_violation_counts")
    if not isinstance(counts, dict) or any(
        not isinstance(key, str)
        or not isinstance(value, int)
        or isinstance(value, bool)
        or value < 0
        for key, value in counts.items()
    ):
        raise ValueError(f"invalid symbolic-name baseline counts: {path}")
    return counts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repository root (defaults to the parent of tools/)",
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        help="per-file ceilings for existing findings (defaults to the repository baseline)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="reject every raw literal, including findings recorded in the baseline",
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
    baseline_path = args.baseline or root / "docs" / "development" / "hla-symbolic-name-baseline.json"
    try:
        baseline = {} if args.strict else _load_baseline(baseline_path)
    except ValueError as error:
        print("HLA symbolic-name verification failed:", file=sys.stderr)
        print(str(error), file=sys.stderr)
        return 1

    actual = Counter()
    unlocated: list[str] = []
    for violation in violations:
        path = _violation_path(violation, root)
        if path is None:
            unlocated.append(violation)
        else:
            actual[path] += 1
    excess = {
        path: count - baseline.get(path, 0)
        for path, count in actual.items()
        if count > baseline.get(path, 0)
    }
    if excess or unlocated:
        print("HLA symbolic-name verification failed:", file=sys.stderr)
        for path, count in sorted(excess.items()):
            print(
                f"{path}: {count} new raw-string lookup(s) "
                f"(baseline {baseline.get(path, 0)}, current {actual[path]})",
                file=sys.stderr,
            )
        if unlocated:
            print("\n".join(unlocated), file=sys.stderr)
        return 1

    print(
        "HLA symbolic-name verification passed "
        f"(0 new findings; {sum(actual.values())} existing findings within "
        f"per-file ceilings across {len(actual)} files; scanned "
        f"{len(_python_files(root))} Python files and "
        f"{len(_cpp_files(root))} C++ files)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
