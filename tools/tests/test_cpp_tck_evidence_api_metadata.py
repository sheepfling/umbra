"""Tests for C++ TCK evidence identifying its implemented language API."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from typing import Any

from tools.cpp_tck import (
    CPP_API_LANGUAGE,
    CPP_API_METHOD_MAPPING_SOURCE,
    CPP_API_PROFILE,
    CPP_TCK_STANDARD,
    CppTckCatalog,
    CppTckScenario,
    validate_results,
)
from tools.run_cpp_tck import annotate_cpp_api_evidence


class CppTckEvidenceApiMetadataTests(unittest.TestCase):
    def setUp(self) -> None:
        self.scenario: CppTckScenario = {
            "id": "cpp-tck.api-metadata-smoke",
            "runner_id": "cpp-tck.api-metadata-smoke",
            "default_status": "ready",
            "promotion": "promoted",
            "callback_models": ["evoked"],
            "api_methods": ["RTIambassador.connect", "RTIambassador.disconnect"],
        }
        self.catalog: CppTckCatalog = {
            "scenarios": [self.scenario],
            "source_boundary": "C++ public API only",
            "requirement_dispositions": [],
        }
        self.payload: dict[str, Any] = {
            "kind": "hla-rti-cpp-tck-evidence",
            "standard": "IEEE 1516.1-2025",
            "results": [
                {
                    "id": self.scenario["runner_id"],
                    "status": "passed",
                    "callback_model": "evoked",
                }
            ],
        }
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.results_path = Path(self.temporary_directory.name) / "evidence.json"
        self.addCleanup(self.temporary_directory.cleanup)

    def _write_payload(self) -> None:
        self.results_path.write_text(
            json.dumps(self.payload),
            encoding="utf-8",
        )

    def test_annotates_and_validates_cpp_api_profile_and_catalog_mapping(self) -> None:
        self.payload.pop("standard")
        self._write_payload()

        annotate_cpp_api_evidence(self.results_path, [self.scenario])

        evidence = json.loads(self.results_path.read_text(encoding="utf-8"))
        self.assertEqual(evidence["standard"], CPP_TCK_STANDARD)
        self.assertEqual(evidence["api_language"], CPP_API_LANGUAGE)
        self.assertEqual(evidence["api_profile"], CPP_API_PROFILE)
        self.assertEqual(
            evidence["scenario_api_methods"],
            {self.scenario["runner_id"]: self.scenario["api_methods"]},
        )
        self.assertEqual(
            evidence["scenario_api_methods_source"],
            CPP_API_METHOD_MAPPING_SOURCE,
        )
        self.assertEqual(
            validate_results(
                self.results_path,
                self.catalog,
                promotion="promoted",
            ),
            [],
        )

    def test_rejects_wrong_language_profile_and_catalog_method_mapping(self) -> None:
        self._write_payload()
        annotate_cpp_api_evidence(self.results_path, [self.scenario])
        evidence = json.loads(self.results_path.read_text(encoding="utf-8"))
        evidence["api_language"] = "Java"
        evidence["api_profile"] = "IEEE 1516.1-2025 Java API"
        evidence["scenario_api_methods"][self.scenario["runner_id"]] = [
            "RTIambassador.joinFederationExecution"
        ]
        self.results_path.write_text(json.dumps(evidence), encoding="utf-8")

        findings = validate_results(
            self.results_path,
            self.catalog,
            promotion="promoted",
        )

        self.assertTrue(any("C++ API language" in finding for finding in findings))
        self.assertTrue(any("C++ API profile" in finding for finding in findings))
        self.assertTrue(any("does not match the C++ TCK catalog" in finding for finding in findings))

    def test_rejects_missing_2025_standard_and_api_metadata(self) -> None:
        self.payload.pop("standard")
        self._write_payload()

        findings = validate_results(
            self.results_path,
            self.catalog,
            promotion="promoted",
        )

        self.assertTrue(any("standard IEEE 1516.1-2025" in finding for finding in findings))
        self.assertTrue(any("C++ API language" in finding for finding in findings))
        self.assertTrue(any("C++ API profile" in finding for finding in findings))
        self.assertTrue(any("scenario_api_methods" in finding for finding in findings))


if __name__ == "__main__":
    unittest.main()
