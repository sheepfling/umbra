"""Tests that C++ TCK coverage follows C++ source ownership, not ID prefixes."""

from __future__ import annotations

import unittest

from tools.query_rti_work import cpp_tck_requirement_gap_inventory


class CppTckRequirementGapSourceFilterTests(unittest.TestCase):
    def test_counts_java_parity_id_with_a_real_cpp_test_source(self) -> None:
        requirement_id = (
            "requirement-candidate-content-clauses-12-programming-language-"
            "mappings-page-413-l45-14"
        )
        requirements = {
            requirement_id: {
                "id": requirement_id,
                "document_id": "hla-1516.1-2025",
                "clause_id": "clause-13.2",
                "ordinal": 14,
            }
        }
        catalog = {
            "kind": "portable-cpp-tck-catalog",
            "scenarios": [
                {
                    "id": "java-tck.api-surface-inventory",
                    "cpp_test": (
                        "src/tck_api_foundation.cpp#scenarioApiSurfaceInventory"
                    ),
                    "promotion": "promoted",
                    "requirement_mappings": [
                        {
                            "requirement_id": requirement_id,
                            "standard_section": "hla-1516.1-2025:clause-13.2",
                        }
                    ],
                }
            ],
        }

        result = cpp_tck_requirement_gap_inventory(
            requirements,
            catalog,
            [],
            query=requirement_id,
            limit=0,
        )

        self.assertEqual(result["total_requirement_count"], 1)
        self.assertEqual(result["mapped_requirement_count"], 1)
        self.assertEqual(result["uncovered_requirement_count"], 0)
        self.assertEqual(result["promoted_requirement_count"], 1)
        self.assertEqual(result["invalid_mapping_count"], 0)


if __name__ == "__main__":
    unittest.main()
