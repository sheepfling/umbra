from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile
import unittest

from _mock_java_fixture import SMOKE_TEST_CLASS, build_mock_java_rti, java_toolchain_available


@unittest.skipUnless(java_toolchain_available(), "requires java, javac, and jar")
class MockJavaFixtureTest(unittest.TestCase):
    def test_service_loader_and_callback_smoke_test(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            jar_path = build_mock_java_rti(Path(temporary_directory))
            result = subprocess.run(
                ["java", "-cp", str(jar_path), SMOKE_TEST_CLASS],
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("Mock Java RTI smoke test: OK", result.stdout)
