"""Tests for section-scoped portable C++ TCK execution evidence queries."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.query_cpp_tck_evidence import build_report, build_section_summary


class QueryCppTckEvidenceTests(unittest.TestCase):
    def test_reports_declared_callback_model_completeness_and_exact_section(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            catalog_path = root / "catalog.json"
            evidence_path = root / "results.json"
            catalog_path.write_text(
                json.dumps(
                    {
                        "scenarios": [
                            {
                                "id": "cpp-tck.complete",
                                "title": "Complete scenario",
                                "promotion": "promoted",
                                "callback_models": None,
                                "requirement_mappings": [
                                    {"requirement_id": "req-a", "standard_section": "standard:clause-8.6"}
                                ],
                            },
                            {
                                "id": "cpp-tck.partial",
                                "title": "Partial scenario",
                                "promotion": "promoted",
                                "callback_models": ["evoked", "immediate"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-b", "standard_section": "standard:clause-8.6"}
                                ],
                            },
                            {
                                "id": "cpp-tck.other-section",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-c", "standard_section": "standard:clause-8.7"}
                                ],
                            },
                        ]
                    }
                ),
                encoding="utf-8",
            )
            evidence_path.write_text(
                json.dumps(
                    {
                        "kind": "hla-rti-cpp-tck-evidence",
                        "results": [
                            {"id": "cpp-tck.complete", "callback_model": "evoked", "status": "passed"},
                            {"id": "cpp-tck.complete", "callback_model": "immediate", "status": "passed"},
                            {"id": "cpp-tck.partial", "callback_model": "evoked", "status": "passed"},
                        ],
                    }
                ),
                encoding="utf-8",
            )

            report = build_report(catalog_path, [evidence_path], section="standard:clause-8.6")

        self.assertEqual(report["direct_requirement_ids"], ["req-a", "req-b"])
        self.assertEqual(report["requirements_with_complete_scenario_witness"], ["req-a"])
        self.assertEqual(report["scenario_state_counts"]["complete"], 1)
        self.assertEqual(report["scenario_state_counts"]["partial"], 1)
        self.assertEqual(report["scenarios"][1]["missing_models"], ["immediate"])

    def test_ignores_non_evidence_json_and_honors_requirement_filter(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            catalog_path = root / "catalog.json"
            results_directory = root / "results"
            results_directory.mkdir()
            (results_directory / "not-evidence.json").write_text('{"results": []}', encoding="utf-8")
            catalog_path.write_text(
                json.dumps(
                    {
                        "scenarios": [
                            {
                                "id": "cpp-tck.for-requirement",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-a", "standard_section": "standard:clause-1"},
                                    {"requirement_id": "req-b", "standard_section": "standard:clause-2"},
                                ],
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            (results_directory / "run.json").write_text(
                json.dumps(
                    {
                        "kind": "hla-rti-cpp-tck-evidence",
                        "results": [
                            {"id": "cpp-tck.for-requirement", "callback_model": "evoked", "status": "passed"}
                        ],
                    }
                ),
                encoding="utf-8",
            )

            report = build_report(catalog_path, [results_directory], requirement="req-b")

        self.assertEqual(report["evidence_file_count"], 1)
        self.assertEqual(report["direct_requirement_ids"], ["req-b"])
        self.assertEqual(report["scenario_state_counts"]["complete"], 1)

    def test_requires_one_exact_selector(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            catalog_path = root / "catalog.json"
            catalog_path.write_text('{"scenarios": []}', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "exactly one"):
                build_report(catalog_path, [], section="s", requirement="r")

    def test_exact_scenario_query_supports_string_callback_model_without_mapping(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            catalog_path = root / "catalog.json"
            evidence_path = root / "results.json"
            catalog_path.write_text(
                json.dumps(
                    {
                        "scenarios": [
                            {
                                "id": "cpp-tck.internal-scenario",
                                "promotion": "promoted",
                                "callback_models": "evoked",
                                "requirement_mappings": [],
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            evidence_path.write_text(
                json.dumps(
                    {
                        "kind": "hla-rti-cpp-tck-evidence",
                        "results": [
                            {
                                "id": "cpp-tck.internal-scenario",
                                "callback_model": "evoked",
                                "status": "passed",
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            report = build_report(catalog_path, [evidence_path], scenario="cpp-tck.internal-scenario")

        self.assertEqual(report["scenario_state_counts"]["complete"], 1)
        self.assertEqual(report["direct_requirement_ids"], [])

    def test_ranks_incomplete_cpp_sections_and_excludes_java_and_candidate_rows(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            catalog_path = root / "catalog.json"
            evidence_path = root / "results.json"
            catalog_path.write_text(
                json.dumps(
                    {
                        "scenarios": [
                            {
                                "id": "cpp-tck.complete-clause-one",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-one", "standard_section": "hla-1516.1-2025:clause-1"}
                                ],
                            },
                            {
                                "id": "cpp-tck.partial-clause-one",
                                "promotion": "promoted",
                                "callback_models": ["evoked", "immediate"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-two", "standard_section": "hla-1516.1-2025:clause-1"}
                                ],
                            },
                            {
                                "id": "cpp-tck.missing-clause-two-a",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-three", "standard_section": "hla-1516.1-2025:clause-2"}
                                ],
                            },
                            {
                                "id": "cpp-tck.missing-clause-two-b",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-three", "standard_section": "hla-1516.1-2025:clause-2"}
                                ],
                            },
                            {
                                "id": "cpp-tck.complete-clause-three",
                                "promotion": "promoted",
                                "callback_models": ["immediate"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-four", "standard_section": "hla-1516.1-2025:clause-3"}
                                ],
                            },
                            {
                                "id": "cpp-tck.blocked-by-java-peer",
                                "promotion": "promoted",
                                "callback_models": ["evoked", "immediate"],
                                "execution_blocker": "requires an adapter-managed Java federate peer",
                                "requirement_mappings": [
                                    {"requirement_id": "req-java-peer", "standard_section": "hla-1516.1-2025:clause-4"}
                                ],
                            },
                            {
                                "id": "cpp-tck.noncanonical-2025-section",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-bad-2025", "standard_section": "hla-1516-2025:clause-4"}
                                ],
                            },
                            {
                                "id": "cpp-tck.legacy-section",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-old", "standard_section": "hla-1516e-2010:clause-4"}
                                ],
                            },
                            {
                                "id": "java-tck.missing",
                                "promotion": "promoted",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-java", "standard_section": "hla-1516.1-2025:clause-4"}
                                ],
                            },
                            {
                                "id": "cpp-tck.candidate",
                                "promotion": "candidate",
                                "callback_models": ["evoked"],
                                "requirement_mappings": [
                                    {"requirement_id": "req-candidate", "standard_section": "standard:clause-5"}
                                ],
                            },
                        ]
                    }
                ),
                encoding="utf-8",
            )
            evidence_path.write_text(
                json.dumps(
                    {
                        "kind": "hla-rti-cpp-tck-evidence",
                        "results": [
                            {"id": "cpp-tck.complete-clause-one", "callback_model": "evoked", "status": "passed"},
                            {"id": "cpp-tck.partial-clause-one", "callback_model": "evoked", "status": "passed"},
                            {"id": "cpp-tck.complete-clause-three", "callback_model": "immediate", "status": "passed"},
                        ],
                    }
                ),
                encoding="utf-8",
            )

            report = build_section_summary(catalog_path, [evidence_path])
            blocked_report = build_report(
                catalog_path,
                [evidence_path],
                scenario="cpp-tck.blocked-by-java-peer",
            )

        self.assertEqual(report["mapped_section_count"], 3)
        self.assertEqual(report["incomplete_section_count"], 2)
        self.assertEqual(report["blocked_section_count"], 1)
        self.assertEqual(
            report["blocked_scenarios"],
            [
                {
                    "id": "cpp-tck.blocked-by-java-peer",
                    "title": "",
                    "execution_blocker": "requires an adapter-managed Java federate peer",
                    "missing_models": ["evoked", "immediate"],
                    "standard_sections": ["hla-1516.1-2025:clause-4"],
                    "requirement_ids": ["req-java-peer"],
                }
            ],
        )
        self.assertEqual(
            report["excluded_noncanonical_section_mappings"],
            [
                {
                    "standard_section": "hla-1516-2025:clause-4",
                    "scenario_ids": ["cpp-tck.noncanonical-2025-section"],
                },
                {
                    "standard_section": "hla-1516e-2010:clause-4",
                    "scenario_ids": ["cpp-tck.legacy-section"],
                },
            ],
        )
        self.assertEqual(
            [item["standard_section"] for item in report["sections"]],
            ["hla-1516.1-2025:clause-1", "hla-1516.1-2025:clause-2"],
        )
        clause_one, clause_two = report["sections"]
        self.assertEqual(clause_one["scenario_state_counts"]["partial"], 1)
        self.assertEqual(clause_one["requirements_with_complete_scenario_witness"], ["req-one"])
        self.assertEqual(clause_one["unwitnessed_requirement_ids"], ["req-two"])
        self.assertEqual(clause_two["incomplete_scenario_count"], 2)
        self.assertEqual(clause_two["unwitnessed_requirement_ids"], ["req-three"])
        self.assertEqual(report["warnings"], [])
        self.assertEqual(
            blocked_report["scenarios"][0]["execution_blocker"],
            "requires an adapter-managed Java federate peer",
        )


if __name__ == "__main__":
    unittest.main()
