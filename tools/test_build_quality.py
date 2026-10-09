"""Tests for persistent build-quality snapshot history."""

from __future__ import annotations

import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

from tools.build_quality import (
    _append_history,
    _read_history,
    _show_comparison,
    _show_details,
    _show_history,
)


class BuildQualityHistoryTests(unittest.TestCase):
    def test_append_and_read_json_lines_preserves_order(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            history_path = Path(temporary_directory) / "metrics.jsonl"
            first = {"schema": 1, "id": "first", "label": "mingw"}
            second = {"schema": 1, "id": "second", "label": "mingw"}

            _append_history(history_path, first)
            _append_history(history_path, second)

            self.assertEqual(_read_history(history_path), [first, second])

    def test_reader_rejects_corrupt_records_with_line_number(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            history_path = Path(temporary_directory) / "metrics.jsonl"
            history_path.write_text('{"schema":1}\nnot-json\n', encoding="utf-8")

            with self.assertRaisesRegex(ValueError, r":2:"):
                _read_history(history_path)

    def test_snapshot_details_include_build_outcome_and_note(self) -> None:
        snapshot = {
            "id": "capture-1",
            "label": "mingw-catch2",
            "branch": "feature/initial-work",
            "commit": "1234567890",
            "working_tree_dirty": True,
            "build_status": "failed",
            "note": "ambassador lookup declarations do not match definitions",
            "environment": None,
            "build": {
                "translation_units": 0,
                "compiler_time_ms": 0,
                "unmeasured_compile_commands": 0,
                "first_party_include_edges": 0,
                "bundled_include_edges": 0,
                "external_include_edges": 0,
                "records": [],
                "header_fanout": [],
                "target_totals": [],
                "link_actions": [],
            },
            "quality": {
                "source_size_soft_findings": 0,
                "source_size_blocking_errors": 0,
                "source_size_findings": [],
                "source_size_errors": [],
                "mapping_check": {
                    "ok": False,
                    "error_count": 20,
                    "warning_count": 0,
                    "tests_without_source_location": 2,
                    "unlocated_test_ids": ["unmapped-source-case"],
                    "error_samples": ["roadmap/test plan mapping error"],
                    "warning_samples": [],
                },
                "traceability": {
                    "catch2_mapped": 0,
                    "catch2_cases": 0,
                    "catch2_unclassified": 0,
                    "catch2_unmapped": 0,
                    "source_health": "ok",
                    "index_matches_live": True,
                },
            },
        }
        output = StringIO()
        with redirect_stdout(output):
            _show_details(snapshot, top=0)

        self.assertIn("status=failed", output.getvalue())
        self.assertIn("Note: ambassador lookup declarations", output.getvalue())
        self.assertIn("Roadmap/test mapping gate: FAILED, 20 errors", output.getvalue())
        self.assertIn("mapping error: roadmap/test plan mapping error", output.getvalue())
        self.assertIn("missing source location: unmapped-source-case", output.getvalue())

    def test_history_can_filter_failed_builds(self) -> None:
        entries = [
            {
                "recorded_at_utc": "2026-10-08T00:00:00+00:00",
                "label": "mingw",
                "build_status": "passed",
                "commit": "11111111",
                "build": {
                    "translation_units": 1,
                    "compiler_time_ms": 1000,
                    "first_party_include_edges": 1,
                },
                "quality": {
                    "source_size_soft_findings": 0,
                    "source_size_blocking_errors": 0,
                    "traceability": {
                        "catch2_unclassified": 0,
                        "index_matches_live": True,
                    },
                },
            },
            {
                "recorded_at_utc": "2026-10-09T00:00:00+00:00",
                "label": "mingw",
                "build_status": "failed",
                "commit": "22222222",
                "build": {
                    "translation_units": 1,
                    "compiler_time_ms": 1000,
                    "first_party_include_edges": 1,
                },
                "quality": {
                    "source_size_soft_findings": 0,
                    "source_size_blocking_errors": 0,
                    "traceability": {
                        "catch2_unclassified": 0,
                        "index_matches_live": True,
                    },
                },
            },
        ]
        output = StringIO()
        with redirect_stdout(output):
            _show_history(entries, limit=10, label=None, status="failed")

        self.assertIn("22222222", output.getvalue())
        self.assertNotIn("11111111", output.getvalue())
        self.assertIn("n/a", output.getvalue())

    def test_comparison_reports_compile_and_fanout_regressions(self) -> None:
        previous = {
            "build": {
                "compiler_time_ms": 1000,
                "first_party_include_edges": 2,
                "records": [{"source": "x.cpp", "elapsed_ms": 100}],
                "header_fanout": [{"path": "common.hpp", "translation_units": 2}],
            },
            "quality": {
                "source_size_soft_findings": 1,
                "mapping_check": {"error_count": 3},
                "traceability": {
                    "catch2_unclassified": 0,
                    "catch2_unmapped": 1,
                },
            },
        }
        current = {
            "build": {
                "compiler_time_ms": 1250,
                "first_party_include_edges": 3,
                "records": [{"source": "x.cpp", "elapsed_ms": 250}],
                "header_fanout": [{"path": "common.hpp", "translation_units": 3}],
            },
            "quality": {
                "source_size_soft_findings": 2,
                "mapping_check": {"error_count": 1},
                "traceability": {
                    "catch2_unclassified": 1,
                    "catch2_unmapped": 2,
                },
            },
        }
        output = StringIO()
        with redirect_stdout(output):
            _show_comparison(current, previous)

        self.assertIn("compiler CPU +250 ms", output.getvalue())
        self.assertIn("+0.15s  x.cpp", output.getvalue())
        self.assertIn("+1 translation units  common.hpp", output.getvalue())
        self.assertIn("unclassified tests +1", output.getvalue())
        self.assertIn("mapping-check errors -2", output.getvalue())


if __name__ == "__main__":
    unittest.main()
