"""Focused tests for installed-package provenance in the C++ TCK runner."""

from __future__ import annotations

import hashlib
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools.inspect_cpp_tck_link import inspect_link_response
from tools.run_cpp_tck import (
    build_parser,
    ctest_registered_scenarios,
    configure,
    custom_fedtime_provider_library,
    direct_run_environment,
    is_portable_dispatch_scenario,
    merge_json_evidence_parts,
    run_command,
    run_ctest,
    run_portable_dispatch_direct,
    resolve_inputs,
    resolve_scenario_set,
    verify_configured_package,
)


class ScenarioSetDefaultsTests(unittest.TestCase):
    def test_explicit_scenario_includes_candidate_cases_by_default(self) -> None:
        self.assertEqual(
            resolve_scenario_set(None, ["cpp-tck.example-candidate"]), "all"
        )

    def test_suite_run_remains_verified_by_default(self) -> None:
        self.assertEqual(resolve_scenario_set(None, []), "verified")

    def test_explicit_scenario_set_is_preserved(self) -> None:
        self.assertEqual(
            resolve_scenario_set("verified", ["cpp-tck.example"]), "verified"
        )
        self.assertEqual(resolve_scenario_set("all", []), "all")


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

    def test_custom_fedtime_direct_run_loads_adapter_provider(self) -> None:
        scenario_id = "cpp-tck.federation-supplied-custom-tradt"
        self.assertTrue(is_portable_dispatch_scenario(scenario_id))
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            build_directory = root / "build"
            build_directory.mkdir()
            provider = build_directory / "custom-fedtime-provider.dll"
            provider.write_bytes(b"test provider")
            manifest = build_directory / "portable-tck-fedtime-provider-Debug.txt"
            manifest.write_text(str(provider), encoding="utf-8")
            inputs = {
                "build_directory": build_directory,
                "package_prefix": root / "package",
            }
            arguments = SimpleNamespace(
                callback_model="both", configuration="Debug"
            )
            process_results = [
                SimpleNamespace(
                    returncode=0,
                    stdout=f"passed {scenario_id} [{model}]\n",
                    stderr="",
                )
                for model in ("evoked", "immediate")
            ]
            with (
                patch(
                    "tools.run_cpp_tck.direct_arguments",
                    return_value=["--scenario", scenario_id],
                ),
                patch(
                    "tools.run_cpp_tck.subprocess.run",
                    side_effect=process_results,
                ) as run,
                patch("builtins.print"),
            ):
                run_portable_dispatch_direct(
                    arguments,
                    inputs,
                    build_directory / "hla_rti_cpp_tck.exe",
                    {"id": scenario_id, "runner_id": scenario_id},
                    None,
                    None,
                )

            environment = run.call_args_list[0].kwargs["env"]
            self.assertEqual(
                str(provider), environment["UMBRA_FEDTIME_PROVIDER_LIBRARY"]
            )
            runtime_variable = (
                "PATH"
                if os.name == "nt"
                else (
                    "DYLD_LIBRARY_PATH"
                    if sys.platform == "darwin"
                    else "LD_LIBRARY_PATH"
                )
            )
            self.assertEqual(
                str(build_directory),
                environment[runtime_variable].split(os.pathsep, maxsplit=1)[0],
            )
            self.assertEqual(2, run.call_count)

    def test_custom_fedtime_provider_requires_configured_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            with self.assertRaisesRegex(
                ValueError, "provider path manifest is missing"
            ):
                custom_fedtime_provider_library(Path(temporary_directory), "Debug")


class CTestParallelismTests(unittest.TestCase):
    def test_default_worker_count_uses_all_available_cpus(self) -> None:
        with patch("tools.run_cpp_tck.os.cpu_count", return_value=32):
            arguments = build_parser().parse_args(
                ["--package-prefix", "installed-package"]
            )
        self.assertEqual(32, arguments.build_parallel)

    def test_ctest_receives_explicit_worker_budget(self) -> None:
        arguments = SimpleNamespace(
            ctest="ctest",
            configuration="Debug",
            scenario=[],
            callback_model="both",
            build_parallel=19,
        )
        inputs = {
            "build_directory": Path("consumer-build"),
            "package_prefix": Path("installed-package"),
        }

        def write_success_junit(
            command: list[str], *, cwd: Path, env: object
        ) -> None:
            junit_path = Path(command[command.index("--output-junit") + 1])
            junit_path.write_text(
                '<testsuite tests="1" skipped="0"><testcase name="pass" /></testsuite>',
                encoding="utf-8",
            )

        with (
            patch("tools.run_cpp_tck.direct_run_environment", return_value={}),
            patch(
                "tools.run_cpp_tck.run_command", side_effect=write_success_junit
            ) as run,
        ):
            run_ctest(arguments, inputs, [])

        command = run.call_args.args[0]
        self.assertEqual("19", command[command.index("--parallel") + 1])
        self.assertIn("--output-junit", command)

    def test_ctest_rejects_successful_run_with_skipped_tests(self) -> None:
        arguments = SimpleNamespace(
            ctest="ctest",
            configuration="Debug",
            scenario=[],
            callback_model="both",
            build_parallel=2,
        )
        inputs = {
            "build_directory": Path("consumer-build"),
            "package_prefix": Path("installed-package"),
        }

        def write_skipped_junit(
            command: list[str], *, cwd: Path, env: object
        ) -> None:
            junit_path = Path(command[command.index("--output-junit") + 1])
            junit_path.write_text(
                '<testsuite tests="1" skipped="1">'
                '<testcase name="fixture-missing" status="notrun">'
                '<skipped message="missing fixture" />'
                '</testcase></testsuite>',
                encoding="utf-8",
            )

        with (
            patch("tools.run_cpp_tck.direct_run_environment", return_value={}),
            patch("tools.run_cpp_tck.run_command", side_effect=write_skipped_junit),
            self.assertRaisesRegex(
                ValueError, "CTest exited successfully but skipped selected TCK cases"
            ),
        ):
            run_ctest(arguments, inputs, [])


class ScenarioFomDefaultsTests(unittest.TestCase):
    def test_category_c_mom_interaction_has_default_extension_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "mom-category-c-interaction-tck.xml",
            Path(bindings["mom-category-c-interaction"]).name,
        )

    def test_attribute_fdd_preferred_order_has_default_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "attribute-fdd-preferred-order-tck.xml",
            Path(bindings["attribute-fdd-preferred-order-default"]).name,
        )

    def test_directed_interaction_subscription_accumulation_has_default_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "directed-interaction-subscription-accumulation-tck.xml",
            Path(bindings["directed-interaction-subscription-accumulation"]).name,
        )

    def test_timestamped_update_before_delete_has_default_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "attribute-fdd-preferred-order-tck.xml",
            Path(bindings["timestamped-update-before-delete"]).name,
        )

    def test_transport_order_has_default_transportation_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "transport-default-best-effort-tck.xml",
            Path(bindings["transport-order-fdd-default"]).name,
        )

    def test_fqr_undefined_galt_cases_have_default_foms(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "p0-tck.xml",
            Path(bindings["fqr-undefined-galt-nrg-disabled"]).name,
        )
        self.assertEqual(
            "nrg-interaction-tck.xml",
            Path(bindings["fqr-undefined-galt-nrg-enabled"]).name,
        )

    def test_pending_advance_restore_has_default_nrg_enabled_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "non-regulated-grant-enabled.xml",
            Path(bindings["restore-pending-time-advance"]).name,
        )
        self.assertEqual(
            "non-regulated-grant-enabled.xml",
            Path(bindings["restore-pending-flush-queue-request"]).name,
        )

    def test_nrg_regulator_release_has_default_enabled_switch_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "non-regulated-grant-enabled.xml",
            Path(bindings["nrg-regulator-release"]).name,
        )

    def test_federation_supplied_custom_tradt_has_default_fom(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "time-representation-custom-tradt-tck.xml",
            Path(bindings["federation-supplied-custom-tradt"]).name,
        )

    def test_additional_join_cases_have_default_duplicate_and_conflict_foms(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        self.assertEqual(
            "delay-subscription-evaluation-tck.xml",
            Path(bindings["delay-subscription-evaluation"]).name,
        )
        self.assertEqual(
            "p0-tck.xml",
            Path(bindings["nmr-undefined-galt-nrg-disabled"]).name,
        )
        self.assertEqual(
            "nrg-interaction-tck.xml",
            Path(bindings["nmr-undefined-galt-nrg-enabled"]).name,
        )
        self.assertEqual(
            "p0-tck.xml",
            Path(bindings["nmra-undefined-galt-nrg-disabled"]).name,
        )
        self.assertEqual(
            "nrg-interaction-tck.xml",
            Path(bindings["nmra-undefined-galt-nrg-enabled"]).name,
        )
        self.assertEqual(
            "p0-tck.xml",
            Path(bindings["tara-undefined-galt-nrg-disabled"]).name,
        )
        self.assertEqual(
            "nrg-interaction-tck.xml",
            Path(bindings["tara-undefined-galt-nrg-enabled"]).name,
        )
        self.assertEqual(
            "support-switch-getter-lifecycle-tck.xml",
            Path(bindings["support-switch-getter-lifecycle"]).name,
        )
        self.assertEqual(
            "fom-empty-domain-content-tck.xml",
            Path(bindings["empty-domain-tables"]).name,
        )
        self.assertEqual(
            "fom-additional-duplicate-tck.xml",
            Path(bindings["additional-join-duplicate"]).name,
        )
        self.assertEqual(
            "fom-additional-order-conflict-tck.xml",
            Path(bindings["additional-join-conflict"]).name,
        )
        self.assertEqual(
            "fom-additional-interaction-order-conflict-tck.xml",
            Path(bindings["additional-join-interaction-conflict"]).name,
        )
        self.assertEqual(
            "fom-additional-transportation-conflict-tck.xml",
            Path(bindings["additional-join-transport-conflict"]).name,
        )
        self.assertEqual(
            "mom-set-switches-extra-parameter-tck.xml",
            Path(bindings["mom-set-switches-extra-parameter"]).name,
        )
        self.assertEqual(
            "mom-report-added-subclass-tck.xml",
            Path(bindings["mom-report-added-subclass-not-sent"]).name,
        )
        self.assertEqual(
            "service-report-invocation-extension-parameter-tck.xml",
            Path(bindings["mom-report-service-invocation-extension-parameter"]).name,
        )
        self.assertEqual(
            "mom-table-attribute-transportation-conflict-tck.xml",
            Path(bindings["mom-table-attribute-transportation-conflict"]).name,
        )
        self.assertEqual(
            "dimension-duplicate-base-tck.xml",
            Path(bindings["dimension-duplicate-base"]).name,
        )
        self.assertEqual(
            "dimension-duplicate-within-module-tck.xml",
            Path(bindings["dimension-duplicate-within-module"]).name,
        )
        self.assertEqual(
            "dimension-duplicate-conflicting-upper-bound-tck.xml",
            Path(bindings["dimension-duplicate-conflicting-upper-bound"]).name,
        )
        self.assertEqual(
            "dimension-duplicate-unspecified-upper-bound-tck.xml",
            Path(bindings["dimension-duplicate-unspecified-upper-bound"]).name,
        )

    def test_tag_data_type_and_sync_capability_foms_are_defaulted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = build_parser().parse_args(
                ["--package-prefix", temporary_directory]
            )
            inputs = resolve_inputs(arguments)

        bindings = dict(binding.split("=", 1) for binding in inputs["scenario_foms"])
        for key, fixture in (
            (
                "tag-data-type-category-valid",
                "tag-data-type-category-valid-tck.xml",
            ),
            ("tag-data-type-basic-user", "tag-data-type-basic-user-tck.xml"),
            (
                "tag-data-type-basic-synchronization",
                "tag-data-type-basic-synchronization-tck.xml",
            ),
            (
                "tag-synchronization-capability-register",
                "tag-synchronization-capability-register-tck.xml",
            ),
        ):
            self.assertEqual(fixture, Path(bindings[key]).name)

    def test_current_package_adapter_includes_focus_lane_scenario_foms(self) -> None:
        repository_root = Path(__file__).resolve().parents[2]
        adapter_cmake = (
            repository_root
            / "packages/hla-rti-cpp-tck/adapters/current-package/CMakeLists.txt"
        )
        adapter_configuration = adapter_cmake.read_text(encoding="utf-8")
        self.assertIn(
            'portable-tck-fedtime-provider-$<CONFIG>.txt', adapter_configuration
        )

        for key, fixture in (
            (
                "federation-supplied-custom-tradt",
                "time-representation-custom-tradt-tck.xml",
            ),
            (
                "delay-subscription-evaluation",
                "delay-subscription-evaluation-tck.xml",
            ),
            (
                "directed-interaction-subscription-accumulation",
                "directed-interaction-subscription-accumulation-tck.xml",
            ),
            (
                "transport-order-fdd-default",
                "transport-default-best-effort-tck.xml",
            ),
            ("nmr-undefined-galt-nrg-disabled", "p0-tck.xml"),
            ("nmr-undefined-galt-nrg-enabled", "nrg-interaction-tck.xml"),
            ("fqr-undefined-galt-nrg-disabled", "p0-tck.xml"),
            ("fqr-undefined-galt-nrg-enabled", "nrg-interaction-tck.xml"),
            ("nmra-undefined-galt-nrg-disabled", "p0-tck.xml"),
            ("nmra-undefined-galt-nrg-enabled", "nrg-interaction-tck.xml"),
            ("tara-undefined-galt-nrg-disabled", "p0-tck.xml"),
            ("tara-undefined-galt-nrg-enabled", "nrg-interaction-tck.xml"),
            (
                "support-switch-getter-lifecycle",
                "support-switch-getter-lifecycle-tck.xml",
            ),
            (
                "tag-data-type-category-valid",
                "tag-data-type-category-valid-tck.xml",
            ),
            (
                "tag-data-type-basic-user",
                "tag-data-type-basic-user-tck.xml",
            ),
            (
                "tag-data-type-basic-synchronization",
                "tag-data-type-basic-synchronization-tck.xml",
            ),
            (
                "tag-synchronization-capability-register",
                "tag-synchronization-capability-register-tck.xml",
            ),
            (
                "mom-set-switches-extra-parameter",
                "mom-set-switches-extra-parameter-tck.xml",
            ),
            (
                "mom-report-added-subclass-not-sent",
                "mom-report-added-subclass-tck.xml",
            ),
            (
                "mom-report-service-invocation-extension-parameter",
                "service-report-invocation-extension-parameter-tck.xml",
            ),
            (
                "mom-table-attribute-transportation-conflict",
                "mom-table-attribute-transportation-conflict-tck.xml",
            ),
            (
                "mom-category-c-interaction",
                "mom-category-c-interaction-tck.xml",
            ),
            ("service-usage-first", "service-usage-first-tck.xml"),
            ("service-usage-second", "service-usage-second-tck.xml"),
        ):
            self.assertIn(
                f'"{key}=${{_repository_root}}/packages/hla-rti-cpp-tck/fom/{fixture}"',
                adapter_configuration,
            )

        for key, fixture in (
            ("object-class-name-period", "object-class-name-period-tck.xml"),
            ("object-class-name-reserved-hla", "object-class-name-reserved-hla-tck.xml"),
            ("object-class-name-reserved-na", "object-class-name-reserved-na-tck.xml"),
            (
                "object-class-name-allowed-punctuation",
                "object-class-name-allowed-punctuation-tck.xml",
            ),
        ):
            self.assertIn(f'"{key}|{fixture}"', adapter_configuration)

        self.assertIn(
            '"dimension-duplicate-within-module=${_repository_root}/packages/'
            'hla-rti-cpp-tck/fom/dimension-duplicate-within-module-tck.xml"',
            adapter_configuration,
        )

    def test_default_configuration_uses_adapter_owned_fixture_set(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            build_directory = Path(temporary_directory)
            arguments = SimpleNamespace(
                cmake="cmake",
                scenario_fom=None,
                generator=None,
                cxx_compiler=None,
            )
            inputs = {
                "build_directory": build_directory,
                "scenario_foms": ["partial-python-default=fixture.xml"],
            }
            with (
                patch("tools.run_cpp_tck.cmake_definitions", return_value=[]),
                patch("tools.run_cpp_tck.run_command") as run,
            ):
                configure(arguments, inputs)

            command = run.call_args.args[0]
            self.assertIn("-UHLA_RTI_TCK_ADAPTER_SCENARIO_FOMS", command)
            self.assertIn("-UHLA_RTI_TCK_ADAPTER_RTI_ADDRESS", command)
            self.assertNotIn("-C", command)
            self.assertFalse(
                (build_directory / "scenario-fom-bindings.cmake").exists()
            )

    def test_ninja_configuration_exports_compile_database(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            arguments = SimpleNamespace(
                cmake="cmake",
                scenario_fom=None,
                generator="Ninja",
                cxx_compiler=None,
            )
            inputs = {
                "build_directory": Path(temporary_directory),
                "scenario_foms": [],
            }
            with (
                patch("tools.run_cpp_tck.cmake_definitions", return_value=[]),
                patch("tools.run_cpp_tck.run_command") as run,
            ):
                configure(arguments, inputs)

        command = run.call_args.args[0]
        self.assertIn("-G", command)
        self.assertIn("Ninja", command)
        self.assertIn("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", command)


class DirectCommandLengthTests(unittest.TestCase):
    def test_long_cpp_tck_command_uses_common_argument_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            executable = root / "hla_rti_cpp_tck.exe"
            long_value = "x" * 25000

            def inspect_command(
                command: list[str], *, cwd: Path, env: object, check: bool
            ) -> None:
                self.assertEqual(command[0], str(executable))
                self.assertEqual(command[1], "--common-arguments-file")
                arguments_file = Path(command[2])
                self.assertTrue(arguments_file.is_file())
                self.assertEqual(
                    arguments_file.read_text(encoding="utf-8"),
                    f"--fom\n{long_value}\n--scenario\ncpp-tck.sample\n",
                )
                self.assertEqual(cwd, root)
                self.assertIsNone(env)
                self.assertTrue(check)

            with patch("tools.run_cpp_tck.subprocess.run", side_effect=inspect_command) as run:
                run_command(
                    [
                        str(executable),
                        "--fom",
                        long_value,
                        "--scenario",
                        "cpp-tck.sample",
                    ],
                    cwd=root,
                )

            run.assert_called_once()


class MessageHistoryAdapterTests(unittest.TestCase):
    def test_adapter_managed_scenarios_are_removed_from_plain_ctest(self) -> None:
        selected = [
            {"id": "cpp-tck.connection-fault-federate-request-replay"},
            {"id": "cpp-tck.connection-fault-registration-response-replay"},
            {"id": "cpp-tck.connection-fault-create-federation-response-replay"},
            {"id": "cpp-tck.connection-fault-reservation-response-replay"},
            {"id": "cpp-tck.connection-fault-delete-object-response-replay"},
            {"id": "cpp-tck.connection-fault-attribute-update-response-replay"},
            {"id": "cpp-tck.connection-fault-send-interaction-response-replay"},
            {"id": "cpp-tck.connection-fault-time-regulation-response-replay"},
            {"id": "cpp-tck.connection-fault-time-constrained-response-replay"},
            {"id": "cpp-tck.connection-fault-time-advance-request-response-replay"},
            {"id": "cpp-tck.connection-fault-time-advance-request-available-response-replay"},
            {"id": "cpp-tck.connection-fault-next-message-request-response-replay"},
            {"id": "cpp-tck.connection-fault-next-message-request-available-response-replay"},
        ]
        arguments = SimpleNamespace(
            connection_loss_fixture="",
            ownership_service_report_fixture="",
            message_history_recovery_fixture="process-fixture.exe",
        )
        self.assertEqual([], ctest_registered_scenarios(arguments, selected))

    def test_fault_evidence_survives_result_part_merging(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            base = root / "base.json"
            first_adapter = root / "first-adapter.json"
            second_adapter = root / "second-adapter.json"
            output = root / "merged.json"
            base.write_text(
                json.dumps({"results": [{"id": "base"}]}), encoding="utf-8"
            )
            first_adapter.write_text(
                json.dumps(
                    {
                        "results": [{"id": "first"}],
                        "transport_fault_evidence": {
                            "cpp-tck.first": {"evoked": {"same_session": True}}
                        },
                    }
                ),
                encoding="utf-8",
            )
            second_adapter.write_text(
                json.dumps(
                    {
                        "results": [{"id": "second"}],
                        "transport_fault_evidence": {
                            "cpp-tck.second": {"immediate": {"same_session": True}}
                        },
                    }
                ),
                encoding="utf-8",
            )

            merge_json_evidence_parts([base, first_adapter, second_adapter], output)

            merged = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(3, len(merged["results"]))
            self.assertEqual(
                {"cpp-tck.first", "cpp-tck.second"},
                set(merged["transport_fault_evidence"]),
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
