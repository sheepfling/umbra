"""Validation tests for portable C++ TCK requirement dispositions."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from typing import Any

from tools.cpp_tck import CppTckCatalog, load_catalog


ROOT = Path(__file__).resolve().parents[2]
CATALOG_PATH = ROOT / "compliance" / "catalogs" / "cpp-tck-scenario-catalog.json"


class CppTckCatalogDispositionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.catalog: CppTckCatalog = json.loads(
            CATALOG_PATH.read_text(encoding="utf-8")
        )
        self.disposition: dict[str, Any] = {
            "requirement_id": "summary-rule-4",
            "standard_section": "hla-1516-2025:clause-4",
            "status": "external-responsibility",
            "owner_role": "joined federate application",
            "rationale": "The summary rule assigns this obligation to joined federates.",
            "evidence_boundary": "This disposition is not direct C++ TCK coverage.",
        }

    def _load_with_disposition(self) -> CppTckCatalog:
        self.catalog["requirement_dispositions"] = [self.disposition]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "cpp-tck-catalog.json"
            path.write_text(json.dumps(self.catalog), encoding="utf-8")
            return load_catalog(path)

    def test_accepts_canonical_1516_framework_summary_rule_disposition(self) -> None:
        catalog = self._load_with_disposition()

        self.assertEqual(
            catalog["requirement_dispositions"][0]["requirement_id"],
            "summary-rule-4",
        )

    def test_accepts_canonical_15161_detailed_rule_disposition(self) -> None:
        self.disposition.update(
            {
                "requirement_id": "detailed-rule-10",
                "standard_section": "hla-1516-2025:clause-6.5",
            }
        )

        catalog = self._load_with_disposition()

        self.assertEqual(
            catalog["requirement_dispositions"][0]["requirement_id"],
            "detailed-rule-10",
        )

    def test_rejects_non_15161_section_for_detailed_rule(self) -> None:
        self.disposition.update(
            {
                "requirement_id": "detailed-rule-10",
                "standard_section": "hla-1516.1-2025:clause-6.5",
            }
        )

        with self.assertRaisesRegex(ValueError, "needs a canonical 2025 section"):
            self._load_with_disposition()

    def test_rejects_framework_summary_rule_mapped_and_dispositioned(self) -> None:
        scenario = next(
            item
            for item in self.catalog["scenarios"]
            if item.get("requirement_mappings")
        )
        scenario["requirement_mappings"].append(
            {
                "requirement_id": "summary-rule-4",
                "standard_section": "hla-1516-2025:clause-4",
            }
        )

        with self.assertRaisesRegex(ValueError, "cannot be both directly mapped and dispositioned"):
            self._load_with_disposition()

    def test_rejects_noncanonical_section_for_framework_summary_rule(self) -> None:
        self.disposition["standard_section"] = "hla-1516.1-2025:clause-4"

        with self.assertRaisesRegex(ValueError, "needs a canonical 2025 section"):
            self._load_with_disposition()


if __name__ == "__main__":
    unittest.main()
