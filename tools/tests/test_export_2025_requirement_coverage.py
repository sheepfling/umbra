"""Tests for the cross-language 2025 requirement coverage export."""

from __future__ import annotations

import unittest

from tools.export_2025_requirement_coverage import (
    aggregate_section_stats_by_document,
    java_requirement_ids_for_scenario,
    target_standard_requirements,
)


class Export2025RequirementCoverageTests(unittest.TestCase):
    def test_java_mapping_uses_direct_lab_ids_and_contract_crosswalk(self) -> None:
        scenario = {
            "requirement_ids": ["lab-direct", "java-stable-requirement"],
        }

        mapped = java_requirement_ids_for_scenario(
            scenario,
            known_lab_requirement_ids={"lab-direct"},
            linked_lab_requirement_ids={"lab-contract-a", "lab-contract-b"},
        )

        self.assertEqual(
            mapped,
            {"lab-direct", "lab-contract-a", "lab-contract-b"},
        )

    def test_java_api_contract_only_scenarios_are_not_normative_mappings(self) -> None:
        scenario = {
            "mapping_classification": "api-contract-only-not-normative-conformance",
            "requirement_ids": [],
        }

        mapped = java_requirement_ids_for_scenario(
            scenario,
            known_lab_requirement_ids={"lab-direct"},
            linked_lab_requirement_ids={"lab-contract"},
        )

        self.assertEqual(mapped, set())

    def test_section_totals_aggregate_by_2025_document(self) -> None:
        summary = aggregate_section_stats_by_document(
            {
                "hla-1516.1-2025:clause-6.1": {
                    "requirements": 2,
                    "either_mapped": 1,
                    "java_tck_mapped": 1,
                    "any_language_mapped": 2,
                },
                "hla-1516.1-2025:clause-6.2": {
                    "requirements": 3,
                    "either_mapped": 2,
                    "java_tck_mapped": 2,
                    "any_language_mapped": 2,
                },
                "hla-1516.2-2025:clause-6.1": {
                    "requirements": 1,
                    "either_mapped": 0,
                    "java_tck_mapped": 1,
                    "any_language_mapped": 1,
                },
            }
        )

        self.assertEqual(summary["hla-1516.1-2025"]["requirements"], 5)
        self.assertEqual(summary["hla-1516.1-2025"]["either_mapped"], 3)
        self.assertEqual(summary["hla-1516.1-2025"]["java_tck_mapped"], 3)
        self.assertEqual(summary["hla-1516.2-2025"]["any_language_mapped"], 1)

    def test_only_ieee_1516_1_and_1516_2_2025_are_in_primary_totals(self) -> None:
        requirements = {
            "current-api": {"document_id": "hla-1516.1-2025"},
            "current-data": {"document_id": "hla-1516.2-2025"},
            "umbrella": {"document_id": "hla-1516-2025"},
            "legacy": {"document_id": "hla-1516.1-2010"},
        }

        selected = target_standard_requirements(requirements)

        self.assertEqual(set(selected), {"current-api", "current-data"})


if __name__ == "__main__":
    unittest.main()
