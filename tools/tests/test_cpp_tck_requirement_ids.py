"""Tests for stable Requirements Lab IDs accepted by the C++ TCK catalog."""

from __future__ import annotations

import unittest

from tools.cpp_tck import is_lab_requirement_identifier


class LabRequirementIdentifierTests(unittest.TestCase):
    def test_accepts_corpus_requirement_ids(self) -> None:
        self.assertTrue(
            is_lab_requirement_identifier(
                "requirement-candidate-content-clauses-04-federation-management-page-038-l21-4"
            )
        )

    def test_accepts_api_baseline_requirement_ids(self) -> None:
        self.assertTrue(
            is_lab_requirement_identifier("req-federate-resigned-callback-behavior")
        )

    def test_rejects_non_lab_references(self) -> None:
        self.assertFalse(is_lab_requirement_identifier("cpp-1516.1-ownership-management"))
        self.assertFalse(is_lab_requirement_identifier(""))
        self.assertFalse(is_lab_requirement_identifier(None))


if __name__ == "__main__":
    unittest.main()
