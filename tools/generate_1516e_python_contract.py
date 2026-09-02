"""Generate the provider-neutral Python 1516e (2010) method surface.

The IEEE 1516.1-2010 Java API archive is the source of truth for the Python
method names, overload counts, and parameter-type tuples. Python cannot
express Java overloads as separate methods, so this generator emits one
dispatch point per method name and records the exact overload metadata in
``__overload_counts__``/``*_PARAMETER_TYPES``. Providers use that metadata to
select and convert the Java/C++ overload at their boundary.

The generated module intentionally contains no RTI behavior.  It is a small,
auditable contract artifact that can be regenerated from a locally staged IEEE
archive without copying the archive into the repository.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re


_DECLARATION = re.compile(
    r"(?m)^\s*(?:public\s+)?(?:static\s+)?(?:default\s+)?"
    r"[A-Za-z_$][\w$]*(?:\s*<[^;{}()]*>)?(?:\s*\[\])*\s+"
    r"([A-Za-z_$][\w$]*)\s*\("
)

_DECLARATION_WITH_PARAMETERS = re.compile(
    r"(?ms)^\s*(?:public\s+)?(?:static\s+)?(?:default\s+)?"
    r"(?:<[A-Za-z_$][^;{}()]*>\s+)?"
    r"(?P<return_type>[A-Za-z_$][\w$]*(?:\s*<[^;{}()]*>)?(?:\s*\[\])*)\s+"
    r"(?P<name>[A-Za-z_$][\w$]*)\s*\((?P<parameters>.*?)\)\s*"
    r"(?=(?:throws\b|;))"
)


def _strip_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", source)


def _top_level_interface_source(source: str) -> str:
    """Keep declarations on the interface itself, excluding nested callback records."""

    source = _strip_comments(source)
    opening = source.find("{")
    if opening < 0:
        return source
    depth = 1
    result: list[str] = []
    for character in source[opening + 1 :]:
        if character == "{":
            depth += 1
            continue
        if character == "}":
            depth -= 1
            if depth == 0:
                break
            continue
        if depth == 1:
            result.append(character)
        else:
            result.append(" ")
    return "".join(result)


def method_names(source: str) -> tuple[str, ...]:
    """Return unique Java method names in declaration order."""

    names: list[str] = []
    for match in _DECLARATION.finditer(_top_level_interface_source(source)):
        name = match.group(1)
        if name not in names:
            names.append(name)
    return tuple(names)


def overload_counts(source: str) -> dict[str, int]:
    """Return Java declaration counts keyed by method name."""

    return dict(
        Counter(match.group(1) for match in _DECLARATION.finditer(_top_level_interface_source(source)))
    )


def _split_parameters(parameters: str) -> tuple[str, ...]:
    """Return normalized Java parameter types, preserving generic nesting."""

    if not parameters.strip():
        return ()
    parts: list[str] = []
    start = 0
    generic_depth = 0
    for index, character in enumerate(parameters):
        if character == "<":
            generic_depth += 1
        elif character == ">":
            generic_depth = max(0, generic_depth - 1)
        elif character == "," and generic_depth == 0:
            parts.append(parameters[start:index])
            start = index + 1
    parts.append(parameters[start:])

    types: list[str] = []
    for part in parts:
        normalized = " ".join(part.strip().split())
        normalized = re.sub(r"^final\s+", "", normalized)
        words = normalized.split(" ")
        if len(words) < 2:
            raise ValueError(f"Cannot split Java parameter declaration: {part!r}")
        parameter_name = words[-1]
        parameter_type = " ".join(words[:-1])
        if parameter_name.endswith("[]"):
            parameter_type += "[]"
        if parameter_type.endswith("..."):
            parameter_type = parameter_type[:-3] + "[]"
        types.append(parameter_type)
    return tuple(types)


def parameter_types(source: str) -> dict[str, tuple[tuple[str, ...], ...]]:
    """Return Java parameter-type tuples keyed by method name and overload."""

    declarations = _top_level_interface_source(source)
    result: dict[str, list[tuple[str, ...]]] = {}
    for match in _DECLARATION_WITH_PARAMETERS.finditer(declarations):
        name = match.group("name")
        result.setdefault(name, []).append(_split_parameters(match.group("parameters")))
    return {name: tuple(overloads) for name, overloads in result.items()}


def return_types(source: str) -> dict[str, tuple[str, ...]]:
    """Return Java return types keyed by method name and overload."""

    declarations = _top_level_interface_source(source)
    result: dict[str, list[str]] = {}
    for match in _DECLARATION_WITH_PARAMETERS.finditer(declarations):
        result.setdefault(match.group("name"), []).append(
            " ".join(match.group("return_type").split())
        )
    return {name: tuple(overloads) for name, overloads in result.items()}


def _quoted(value: str) -> str:
    return repr(value)


def _method_block(name: str, *, abstract: bool) -> str:
    decorator = "    @abstractmethod\n" if abstract else ""
    body = (
        "        raise NotImplementedError(\n"
        "            f\"IEEE 1516e provider has not implemented "
        "{type(self).__name__}.{self_name}\"\n"
        "        )"
        if abstract
        else "        return None"
    )
    return (
        f"{decorator}    def {name}(self, *args: object, **kwargs: object) -> object:\n"
        f"        self_name = {name!r}\n{body}"
    )


def render(java_root: Path) -> str:
    rti_source = (java_root / "RTIambassador.java").read_text(encoding="utf-8")
    federate_source = (java_root / "FederateAmbassador.java").read_text(encoding="utf-8")
    rti_names = method_names(rti_source)
    federate_names = method_names(federate_source)
    rti_counts = overload_counts(rti_source)
    federate_counts = overload_counts(federate_source)
    rti_parameter_types = parameter_types(rti_source)
    federate_parameter_types = parameter_types(federate_source)
    rti_return_types = return_types(rti_source)
    federate_return_types = return_types(federate_source)

    blocks: list[str] = [
        '"""Generated IEEE 1516.1-2010 provider-neutral Python contracts.',
        "",
        "This file is generated from the authoritative Java API source archive.",
        "It contains names and overload metadata only; it is not an RTI.",
        '"""',
        "",
        "from __future__ import annotations",
        "",
        "from abc import ABC, abstractmethod",
        "from dataclasses import dataclass",
        "from typing import ClassVar",
        "",
        'STANDARD_EDITION = "IEEE 1516.1-2010"',
        'JAVA_PACKAGE = "hla.rti1516e"',
        'CPP_NAMESPACE = "rti1516e"',
        "",
        "@dataclass(frozen=True, slots=True)",
        "class MethodSpec:",
        "    name: str",
        "    overload_count: int",
        "",
        f"RTIAMBASSADOR_METHODS = {rti_names!r}",
        f"FEDERATE_AMBASSADOR_METHODS = {federate_names!r}",
        f"RTIAMBASSADOR_OVERLOAD_COUNTS = {rti_counts!r}",
        f"FEDERATE_AMBASSADOR_OVERLOAD_COUNTS = {federate_counts!r}",
        f"RTIAMBASSADOR_PARAMETER_TYPES = {rti_parameter_types!r}",
        f"FEDERATE_AMBASSADOR_PARAMETER_TYPES = {federate_parameter_types!r}",
        f"RTIAMBASSADOR_RETURN_TYPES = {rti_return_types!r}",
        f"FEDERATE_AMBASSADOR_RETURN_TYPES = {federate_return_types!r}",
        "",
        "class FederateAmbassador(ABC):",
        '    \"\"\"Receive callbacks from a 2010 provider.\"\"\"',
        '    __java_package__: ClassVar[str] = JAVA_PACKAGE',
        '    __cpp_namespace__: ClassVar[str] = CPP_NAMESPACE',
        '    __overload_counts__: ClassVar[dict[str, int]] = FEDERATE_AMBASSADOR_OVERLOAD_COUNTS',
        "",
    ]
    blocks.extend(_method_block(name, abstract=False) for name in federate_names)
    blocks.extend(
        [
            "",
            "class NullFederateAmbassador(FederateAmbassador):",
            '    \"\"\"Convenience callback sink matching the standard Java helper.\"\"\"',
            "    pass",
            "",
            "class RTIambassador(ABC):",
            '    \"\"\"Provider-facing IEEE 1516.1-2010 RTI service contract.\"\"\"',
            '    __java_package__: ClassVar[str] = JAVA_PACKAGE',
            '    __cpp_namespace__: ClassVar[str] = CPP_NAMESPACE',
            '    __overload_counts__: ClassVar[dict[str, int]] = RTIAMBASSADOR_OVERLOAD_COUNTS',
            "",
        ]
    )
    blocks.extend(_method_block(name, abstract=True) for name in rti_names)
    blocks.extend(
        [
            "",
            "__all__ = [",
            '    "STANDARD_EDITION",',
            '    "JAVA_PACKAGE",',
            '    "CPP_NAMESPACE",',
            '    "MethodSpec",',
            '    "RTIAMBASSADOR_METHODS",',
            '    "FEDERATE_AMBASSADOR_METHODS",',
            '    "RTIAMBASSADOR_OVERLOAD_COUNTS",',
            '    "FEDERATE_AMBASSADOR_OVERLOAD_COUNTS",',
            '    "RTIAMBASSADOR_PARAMETER_TYPES",',
            '    "FEDERATE_AMBASSADOR_PARAMETER_TYPES",',
            '    "RTIAMBASSADOR_RETURN_TYPES",',
            '    "FEDERATE_AMBASSADOR_RETURN_TYPES",',
            '    "FederateAmbassador",',
            '    "NullFederateAmbassador",',
            '    "RTIambassador",',
            "]",
            "",
        ]
    )
    return "\n".join(blocks)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--java-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    required = (args.java_root / "RTIambassador.java", args.java_root / "FederateAmbassador.java")
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        parser.error("missing Java API sources: " + ", ".join(missing))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render(args.java_root), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
