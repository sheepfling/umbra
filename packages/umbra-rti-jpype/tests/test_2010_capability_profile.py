from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tools.verify_1516e_capability_profile import verify


class CapabilityProfile2010Tests(unittest.TestCase):
    def test_checked_in_shared_profile_is_catalog_linked(self) -> None:
        profile = Path("packages/umbra-rti-java-tck-2010/profiles/mock-java-rti-2010.properties")
        result = verify(profile)
        self.assertEqual(result["status"], "pass")
        self.assertGreaterEqual(result["entries"], 60)

    def test_unknown_or_duplicate_scenario_keys_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid.properties"
            path.write_text(
                "scenario.python-2010-tck.factory-discovery=run\n"
                "scenario.python-2010-tck.factory-discovery=pass\n"
                "scenario.python-2010-tck.typoed-scenario=run\n",
                encoding="utf-8",
            )
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(any("repeats scenario id" in item for item in result["findings"]))
        self.assertTrue(any("unknown scenario id" in item for item in result["findings"]))

    def test_jni_null_profile_exposes_only_the_bounded_connection_slice(self) -> None:
        profile = Path(
            "packages/umbra-rti-java-tck-2010/profiles/jni-null-provider-2010.properties"
        )
        result = verify(profile)
        self.assertEqual(result["status"], "pass")
        self.assertGreaterEqual(result["entries"], 60)
        declaration = profile.read_text(encoding="utf-8")
        self.assertIn("scenario.java-2010-tck.connect-disconnect=run", declaration)
        self.assertIn("scenario.python-2010-tck.connect-disconnect=run", declaration)


if __name__ == "__main__":
    unittest.main()
