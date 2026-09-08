#!/usr/bin/env python3
"""Merge independent Catch2 JUnit reports into one valid testsuites document."""

from __future__ import annotations

import argparse
import copy
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


_COUNT_ATTRIBUTES = ("tests", "failures", "errors", "skipped")


def _number(value: str | None) -> int:
    try:
        return int(value or "0")
    except ValueError:
        return 0


def _time(value: str | None) -> float:
    try:
        return float(value or "0")
    except ValueError:
        return 0.0


def _suites(root: ET.Element) -> list[ET.Element]:
    if root.tag == "testsuite":
        return [root]
    if root.tag != "testsuites":
        raise ValueError(f"unsupported JUnit root element: {root.tag}")
    return [suite for suite in root.findall("testsuite")]


def merge(inputs: list[Path], output: Path) -> None:
    merged = ET.Element("testsuites")
    totals = {attribute: 0 for attribute in _COUNT_ATTRIBUTES}
    total_time = 0.0
    suite_count = 0

    for input_path in inputs:
        if not input_path.is_file():
            raise FileNotFoundError(input_path)
        root = ET.parse(input_path).getroot()
        suites = _suites(root)
        for suite in suites:
            merged.append(copy.deepcopy(suite))
            suite_count += 1
            for attribute in _COUNT_ATTRIBUTES:
                totals[attribute] += _number(suite.get(attribute))
            total_time += _time(suite.get("time"))

    if suite_count == 0:
        raise ValueError("JUnit inputs contain no testsuite elements")

    for attribute, value in totals.items():
        merged.set(attribute, str(value))
    merged.set("time", f"{total_time:.6f}")

    output.parent.mkdir(parents=True, exist_ok=True)
    tree = ET.ElementTree(merged)
    if hasattr(ET, "indent"):
        ET.indent(tree, space="  ")
    tree.write(output, encoding="utf-8", xml_declaration=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("inputs", nargs="+", type=Path)
    arguments = parser.parse_args(argv)
    try:
        merge(arguments.inputs, arguments.output)
    except (OSError, ET.ParseError, ValueError) as error:
        print(f"merge-junit-reports: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
