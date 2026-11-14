"""Ensure routed portable TCK scenarios count toward the source inventory."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools import cpp_tck


class CppTckScenarioInventoryTests(unittest.TestCase):
    def test_includes_ids_routed_by_scenario_dispatcher(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            source_root = Path(temporary_directory)
            source_directory = source_root / "src"
            source_directory.mkdir()
            (source_directory / "main.cpp").write_text(
                "std::vector<std::string> allScenarioIds() {\n"
                '  return {"cpp-tck.main-inventory"};\n'
                "}\n",
                encoding="utf-8",
            )
            (source_directory / "tck_scenario_dispatch.cpp").write_text(
                'if (id == "cpp-tck.dispatched") { return scenarioDispatched; }\n',
                encoding="utf-8",
            )

            with patch.object(cpp_tck, "SOURCE_ROOT", source_root):
                inventory = cpp_tck.scenario_inventory()

        self.assertCountEqual(
            inventory,
            ["cpp-tck.main-inventory", "cpp-tck.dispatched"],
        )


if __name__ == "__main__":
    unittest.main()
