"""Keep the C++ TCK header allowlist aligned with the pinned 2025 API set."""

from __future__ import annotations

import json
import unittest

from tools.cpp_tck import OFFICIAL_API_HEADERS, ROOT


class CppTckOfficialHeaderInventoryTests(unittest.TestCase):
    def test_allowlist_matches_pinned_2025_header_manifest(self) -> None:
        manifest_path = (
            ROOT / "third_party" / "ieee1516.1-2025" / "header-digests.json"
        )
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest_headers = {entry["path"] for entry in manifest["files"]}

        self.assertEqual(OFFICIAL_API_HEADERS, manifest_headers)


if __name__ == "__main__":
    unittest.main()
