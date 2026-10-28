"""Focused tests for installed-package provenance in the C++ TCK runner."""

from __future__ import annotations

import hashlib
import os
import sys
import tempfile
import unittest
from pathlib import Path

from tools.inspect_cpp_tck_link import inspect_link_response
from tools.run_cpp_tck import direct_run_environment, verify_configured_package


class ConfiguredPackageTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.prefix = self.root / "selected-provider"
        self.build = self.root / "consumer-build"
        self.build.mkdir()
        self.inputs = {
            "package_prefix": self.prefix,
            "build_directory": self.build,
        }

    def write_cache(self, actual_prefix: Path) -> None:
        include = actual_prefix / "include"
        package_config = actual_prefix / "lib" / "cmake" / "umbra_rti"
        include.mkdir(parents=True, exist_ok=True)
        package_config.mkdir(parents=True, exist_ok=True)
        (self.build / "CMakeCache.txt").write_text(
            "CMAKE_PREFIX_PATH:UNINITIALIZED=" + str(self.prefix) + "\n"
            "umbra_rti_DIR:PATH=" + str(package_config) + "\n"
            "HLA_RTI_TCK_API_INCLUDE_DIR:PATH=" + str(include) + "\n",
            encoding="utf-8",
        )

    def test_matching_installed_package_is_accepted(self) -> None:
        self.write_cache(self.prefix)
        configured = verify_configured_package(self.inputs)
        self.assertEqual(
            configured["HLA_RTI_TCK_API_INCLUDE_DIR"],
            (self.prefix / "include").resolve(),
        )

    def test_stale_package_dir_is_rejected_despite_requested_prefix(self) -> None:
        self.write_cache(self.root / "another-provider")
        with self.assertRaisesRegex(ValueError, "outside --package-prefix"):
            verify_configured_package(self.inputs)

    def test_missing_configuration_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "cache was not found"):
            verify_configured_package(self.inputs)


class DirectRunEnvironmentTests(unittest.TestCase):
    def test_selected_package_runtime_directory_precedes_existing_path(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            prefix = Path(temporary_directory)
            inputs = {"package_prefix": prefix}
            variable = (
                "PATH"
                if os.name == "nt"
                else (
                    "DYLD_LIBRARY_PATH"
                    if sys.platform == "darwin"
                    else "LD_LIBRARY_PATH"
                )
            )
            environment = direct_run_environment(
                inputs,
                base_environment={variable: "existing-runtime"},
            )
            runtime_directory = prefix / ("bin" if os.name == "nt" else "lib")
            self.assertEqual(
                environment[variable],
                os.pathsep.join((str(runtime_directory), "existing-runtime")),
            )


class IsolatedLinkProvenanceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.package = self.root / "selected-provider"
        self.library = self.package / "lib" / "libprovider.a"
        self.library.parent.mkdir(parents=True)
        self.library.write_bytes(b"provider-one")
        self.response = self.root / "link.rsp"

    def test_matching_library_is_fingerprinted_once(self) -> None:
        self.response.write_text(
            f"object.obj {self.library.as_posix()} {self.library.as_posix()} -lsystem",
            encoding="utf-8",
        )
        result = inspect_link_response(
            self.response, self.package, require_package_libraries=True
        )
        self.assertEqual(len(result["libraries"]), 1)
        self.assertEqual(result["libraries"][0]["references"], 2)
        self.assertTrue(result["libraries"][0]["within_package_prefix"])
        self.assertEqual(
            result["libraries"][0]["sha256"],
            hashlib.sha256(b"provider-one").hexdigest(),
        )

    def test_library_from_another_package_is_rejected(self) -> None:
        self.response.write_text(self.library.as_posix(), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "outside --package-prefix"):
            inspect_link_response(
                self.response,
                self.root / "another-provider",
                require_package_libraries=True,
            )

    def test_quoted_native_path_with_spaces_is_supported(self) -> None:
        spaced = self.package / "lib with spaces" / "libprovider.a"
        spaced.parent.mkdir()
        spaced.write_bytes(b"spaced-provider")
        self.response.write_text(f'"{spaced}"', encoding="utf-8")
        result = inspect_link_response(
            self.response, self.package, require_package_libraries=True
        )
        self.assertEqual(result["libraries"][0]["path"], str(spaced.resolve()))

    def test_relative_library_uses_explicit_link_directory(self) -> None:
        self.response.write_text("lib/libprovider.a", encoding="utf-8")
        result = inspect_link_response(
            self.response,
            self.package,
            working_directory=self.package,
            require_package_libraries=True,
        )
        self.assertEqual(result["libraries"][0]["path"], str(self.library.resolve()))

    def test_missing_library_is_rejected(self) -> None:
        self.response.write_text(
            (self.package / "lib" / "missing.a").as_posix(), encoding="utf-8"
        )
        with self.assertRaisesRegex(ValueError, "was not found"):
            inspect_link_response(
                self.response, self.package, require_package_libraries=True
            )

    def test_response_without_libraries_is_rejected(self) -> None:
        self.response.write_text("object.obj -lsystem", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no explicit library paths"):
            inspect_link_response(
                self.response, self.package, require_package_libraries=True
            )


if __name__ == "__main__":
    unittest.main()
