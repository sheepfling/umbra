from __future__ import annotations

import subprocess
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1]
ROOT = TOOLS.parent
sys.path.insert(0, str(TOOLS))

import query_rti_work


def test_java_api_requirement_is_not_a_cpp_gap_candidate() -> None:
    assert query_rti_work.is_java_api_specific_requirement(
        {
            "title": "Java API utility classes",
            "statement": "Implementations of the Java API shall add these classes.",
        }
    )
    assert not query_rti_work.is_java_api_specific_requirement(
        {
            "title": "Language transparency",
            "statement": "The federation shall be transparent to API language.",
        }
    )
    assert not query_rti_work.is_java_api_specific_requirement(
        {
            "title": "Language API parity",
            "statement": "The Java API and C++ API shall expose equivalent behavior.",
        }
    )


def test_cpp_tck_gap_or_active_handoff_remains_queryable() -> None:
    result = subprocess.run(
        [
            sys.executable,
            str(TOOLS / "query_rti_work.py"),
            "next",
            "--summary",
            "--compact",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    if "direct_tck_gap_query=python tools/query_rti_work.py tck-gaps " in result.stdout:
        assert (
            "--document hla-1516.1-2025 --summary --compact"
            in result.stdout
        )
        assert (
            "the family gaps above are Catch2-plan inventory, not direct C++ TCK coverage"
            in result.stdout
        )
        return

    assert "state: active-handoff" in result.stdout
    assert (
        "plan_id: cpp-tck.resign-cancel-pending-ownership-acquisitions-action"
        in result.stdout
    )
    assert "matrix_requirement_command: python tools/query_rti_work.py matrix" in result.stdout

    gap_query = subprocess.run(
        [
            sys.executable,
            str(TOOLS / "query_rti_work.py"),
            "tck-gaps",
            "--document",
            "hla-1516.1-2025",
            "--summary",
            "--compact",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    assert "Portable C++ TCK direct requirement coverage" in gap_query.stdout
