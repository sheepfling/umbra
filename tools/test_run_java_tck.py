"""Focused, provider-independent checks for the portable Java TCK runner."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.run_java_tck import build_parser, run_matrix, run_tck


class RunUnverifiedTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.classes = self.root / "classes"
        self.classes.mkdir()

    def test_opt_in_is_passed_only_when_requested(self) -> None:
        inputs = dict(
            api_jar=self.root / "api.jar",
            provider_jars=[self.root / "provider.jar"],
            dependency_jars=[],
            fom_path=self.root / "model.xml",
            classes_directory=self.classes,
            java="java",
            javac="javac",
            skip_build=True,
        )
        with patch("tools.run_java_tck.run_command", return_value=0) as command:
            self.assertEqual(run_tck(**inputs), 0)
            self.assertFalse(
                any("hla.rti.tck.runUnverified" in arg for arg in command.call_args.args[0])
            )
            self.assertEqual(run_tck(**inputs, run_unverified=True), 0)
            self.assertIn(
                "-Dhla.rti.tck.runUnverified=true", command.call_args.args[0]
            )

    def test_matrix_can_opt_in_without_a_profile(self) -> None:
        for name in ("api.jar", "provider-a.jar", "provider-b.jar", "model.xml"):
            (self.root / name).touch()
        matrix = self.root / "matrix.json"
        matrix.write_text(
            json.dumps(
                [
                    {
                        "id": identifier,
                        "apiJar": "api.jar",
                        "providerJar": f"provider-{identifier}.jar",
                        "factoryName": identifier,
                        "fomPath": "model.xml",
                        "runUnverified": True,
                    }
                    for identifier in ("a", "b")
                ]
            ),
            encoding="utf-8",
        )
        arguments = build_parser().parse_args(
            [
                "matrix",
                "--configuration",
                str(matrix),
                "--classes-directory",
                str(self.classes),
                "--output-directory",
                str(self.root / "results"),
                "--skip-build",
            ]
        )
        with patch("tools.run_java_tck.run_tck", return_value=0) as run:
            self.assertEqual(run_matrix(arguments), 0)
            self.assertEqual(run.call_count, 2)
            for call in run.call_args_list:
                self.assertTrue(call.kwargs["run_unverified"])
                self.assertIsNone(call.kwargs["capability_profile"])


if __name__ == "__main__":
    unittest.main()
