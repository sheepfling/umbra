"""Compare the 2010 Java encoding interface names with Python contracts."""

from __future__ import annotations

import argparse
import importlib
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
_METHOD = re.compile(
    r"(?m)^\s*(?:public\s+)?(?:default\s+)?(?:static\s+)?"
    r"(?:<[^;{}()]*>\s+)?[A-Za-z_$][\w$]*(?:\s*<[^;{}()]*>)?(?:\[\])?\s+"
    r"(?P<name>[A-Za-z_$][\w$]*)\s*\("
)


def java_methods(path: Path) -> set[str]:
    source = re.sub(r"/\*.*?\*/", "", path.read_text(encoding="utf-8"), flags=re.DOTALL)
    source = re.sub(r"//[^\n]*", "", source)
    return {match.group("name") for match in _METHOD.finditer(source)}


def verify(java_root: Path) -> list[str]:
    sys.path.insert(0, str(ROOT / "packages" / "umbra-rti-api" / "src"))
    module = importlib.import_module("hla.rti1516e.encoding")
    findings: list[str] = []
    aliases = {"iterator": "__iter__"}
    for source in sorted(java_root.glob("*.java")):
        if source.stem in {"DecoderException", "EncoderException"}:
            continue
        python_type = getattr(module, source.stem, None)
        if python_type is None:
            findings.append(f"missing Python encoding class {source.stem}")
            continue
        available = set(dir(python_type))
        for method in java_methods(source):
            if method == source.stem:
                continue
            if method in {"toString", "equals", "hashCode"}:
                continue
            required = aliases.get(method, method)
            if required not in available:
                findings.append(f"{source.stem}: missing {method} (Python {required})")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--java-root", type=Path, required=True)
    args = parser.parse_args()
    findings = verify(args.java_root)
    if findings:
        print("IEEE 1516e encoding contract verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e encoding contract verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
