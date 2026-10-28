"""Focused parser tests for the Ninja compile-time report helper."""

from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.report_compile_times import _ninja_dependencies, collect


class CompileTimeReportTests(unittest.TestCase):
    def test_collect_maps_ninja_output_to_source_and_uses_latest_duration(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            build_dir = Path(temporary_directory)
            source = build_dir / "source.cpp"
            output = Path("CMakeFiles") / "target.dir" / "source.cpp.obj"
            (build_dir / "compile_commands.json").write_text(
                json.dumps(
                    [
                        {
                            "directory": str(build_dir),
                            "file": str(source),
                            "output": output.as_posix(),
                            "command": "g++ -c source.cpp",
                        }
                    ]
                ),
                encoding="utf-8",
            )
            (build_dir / ".ninja_log").write_text(
                "# ninja log v7\n"
                f"0\t1250\t1\t{output.as_posix()}\toldhash\n"
                f"1250\t2875\t2\t{output.as_posix()}\tnewhash\n",
                encoding="utf-8",
            )

            completed = subprocess.CompletedProcess([], 0, "", "")
            with patch(
                "tools.report_compile_times.subprocess.run", return_value=completed
            ):
                records, _, _, _, _, unmatched = collect(build_dir)

        self.assertEqual(unmatched, 0)
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0]["source"], str(source.resolve()))
        self.assertEqual(records[0]["elapsed_ms"], 1625)

    def test_ninja_dependency_output_keeps_each_transitive_include_set(self) -> None:
        build_dir = Path("C:/work/build")
        output = "CMakeFiles/example.dir/source.cpp.obj"
        dependency_text = (
            f"{output}: #deps 2, deps mtime 100 (VALID)\n"
            "    C:/work/project/source.cpp\n"
            "    C:/work/project/include/public.hpp\n"
            "\n"
        )

        dependencies = _ninja_dependencies(dependency_text, build_dir)

        self.assertEqual(
            dependencies[
                str((build_dir / output).resolve(strict=False))
                .lower()
                .replace("\\", "/")
            ],
            [
                "c:/work/project/source.cpp",
                "c:/work/project/include/public.hpp",
            ],
        )


if __name__ == "__main__":
    unittest.main()
