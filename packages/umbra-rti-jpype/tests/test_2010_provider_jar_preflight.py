from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from zipfile import ZipFile, ZIP_DEFLATED


ROOT = Path(__file__).resolve().parents[3]
_SPEC = importlib.util.spec_from_file_location(
    "verify_1516e_provider_jar", ROOT / "tools" / "verify_1516e_provider_jar.py"
)
assert _SPEC is not None and _SPEC.loader is not None
_MODULE = importlib.util.module_from_spec(_SPEC)
sys.modules[_SPEC.name] = _MODULE
_SPEC.loader.exec_module(_MODULE)


class ProviderJarPreflightTests(unittest.TestCase):
    def _jar(self, entries: dict[str, str | bytes]) -> Path:
        handle = tempfile.NamedTemporaryFile(suffix=".jar", delete=False)
        handle.close()
        path = Path(handle.name)
        with ZipFile(path, "w", ZIP_DEFLATED) as archive:
            for name, value in entries.items():
                archive.writestr(name, value)
        self.addCleanup(lambda: path.unlink(missing_ok=True))
        return path

    def test_provider_descriptor_is_required_and_2010_only(self) -> None:
        valid = self._jar(
            {
                "META-INF/services/hla.rti1516e.RtiFactory": "vendor.Provider\n",
                "vendor/Provider.class": b"",
            }
        )
        result = _MODULE.verify([valid])[0]
        self.assertEqual(result.status, "pass")
        self.assertEqual(result.service_entries, ("vendor.Provider",))

        dependency_only_class = self._jar({"vendor/Provider.class": b""})
        provider_without_class = self._jar(
            {"META-INF/services/hla.rti1516e.RtiFactory": "vendor.Provider\n"}
        )
        self.assertEqual(
            _MODULE.verify([provider_without_class], dependency_jars=[dependency_only_class])[0].status,
            "pass",
        )

        missing = self._jar({"vendor/Provider.class": b""})
        self.assertEqual(_MODULE.verify([missing])[0].status, "fail")

        mixed = self._jar(
            {
                "META-INF/services/hla.rti1516e.RtiFactory": "vendor.Provider\n",
                "META-INF/services/hla.rti1516_2025.RtiFactory": "other.Provider\n",
            }
        )
        mixed_result = _MODULE.verify([mixed])[0]
        self.assertEqual(mixed_result.status, "fail")
        self.assertTrue(any("2025" in item for item in mixed_result.findings))

    def test_api_jar_requires_canonical_2010_entries(self) -> None:
        complete = {entry: b"" for entry in _MODULE.REQUIRED_API_ENTRIES}
        complete_jar = self._jar(complete)
        self.assertEqual(_MODULE.verify([], complete_jar)[0].status, "pass")

        incomplete_jar = self._jar({"hla/rti1516e/RtiFactory.class": b""})
        result = _MODULE.verify([], incomplete_jar)[0]
        self.assertEqual(result.status, "fail")
        self.assertTrue(any("missing" in item for item in result.findings))


if __name__ == "__main__":
    unittest.main()
