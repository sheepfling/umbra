"""Generate the Python names for the IEEE 1516e exception hierarchy."""

from __future__ import annotations

import argparse
from pathlib import Path
import re


_CLASS = re.compile(r"public\s+(?:final\s+)?class\s+(\w+)\s+extends\s+(\w+)")


def render(source_root: Path) -> str:
    classes: list[str] = []
    for source in sorted(source_root.glob("*.java")):
        match = _CLASS.search(source.read_text(encoding="utf-8"))
        if match:
            classes.append(match.group(1))
    classes = sorted(set(classes), key=lambda value: (value != "RTIexception", value))
    lines = [
        '"""IEEE 1516.1-2010 RTI exception names.',
        "",
        "Generated from the standard Java exception source inventory.  The",
        "classes preserve names and catchability; providers supply messages.",
        '"""',
        "",
        "from __future__ import annotations",
        "",
        "class RTIexception(Exception):",
        '    \"\"\"Base class for every exception in the 2010 RTI API.\"\"\"',
        "",
        "    def __init__(self, message: str = \"\", cause: BaseException | None = None) -> None:",
        "        super().__init__(message)",
        "        self.cause = cause",
        "        if cause is not None:",
        "            self.__cause__ = cause",
        "",
    ]
    for name in classes:
        if name == "RTIexception":
            continue
        lines.extend(
            [
                f"class {name}(RTIexception):",
                f'    \"\"\"IEEE 1516e exception ``{name}``.\"\"\"',
                "",
            ]
        )
    lines.extend(
        [
            "_EXCEPTION_TYPES = {",
            *[f'    "{name}": {name},' for name in classes],
            "}",
            "_STANDARD_EXCEPTION_NAMES = frozenset(_EXCEPTION_TYPES)",
            "",
            "def exceptionForName(name: str, message: str = \"\", cause: BaseException | None = None) -> RTIexception:",
            "    \"\"\"Construct a standard exception by its Java simple class name.\"\"\"",
            "    exception_type = _EXCEPTION_TYPES.get(name.rsplit(\".\", 1)[-1].rsplit(\"$\", 1)[-1])",
            "    if exception_type is None:",
            "        return RTIinternalError(message or f\"Unknown IEEE 1516e exception: {name}\", cause)",
            "    return exception_type(message, cause)",
            "",
            "__all__ = [",
            *[f'    "{name}",' for name in classes],
            '    "exceptionForName",',
            "]",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not (args.source_root / "RTIexception.java").is_file():
        parser.error("source root does not contain RTIexception.java")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render(args.source_root), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
