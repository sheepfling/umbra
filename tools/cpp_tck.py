"""Validate the portable C++ IEEE 1516.1-2025 TCK boundary and evidence."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "compliance" / "catalogs" / "cpp-tck-scenario-catalog.json"
SOURCE_ROOT = ROOT / "packages" / "hla-rti-cpp-tck"
EXPECTED_ADAPTER_MANAGED_SKIPS = {
    (
        "cpp-tck.connection-loss-cleanup",
        "requires an adapter-managed connection-loss fixture",
    ),
    (
        "cpp-tck.connection-loss-cleanup-contract",
        "requires an adapter-managed connection-loss fixture",
    ),
    (
        "cpp-tck.automatic-resign-directive-delete-objects",
        "requires an adapter-managed connection-loss fixture",
    ),
    (
        "cpp-tck.automatic-resign-directive-delete-objects-contract",
        "requires an adapter-managed connection-loss fixture",
    ),
    (
        "cpp-tck.base-subscription-rejects-derived-only-attribute",
        "requires an adapter-supplied known-class advisory FOM",
    ),
    (
        "cpp-tck.base-subscription-rejects-derived-only-attribute-contract",
        "requires an adapter-supplied known-class advisory FOM",
    ),
}
STANDARD_HEADERS = {
    "algorithm",
    "array",
    "atomic",
    "chrono",
    "cmath",
    "condition_variable",
    "cstddef",
    "cstdint",
    "cstdlib",
    "exception",
    "filesystem",
    "fstream",
    "future",
    "functional",
    "iostream",
    "iterator",
    "limits",
    "map",
    "memory",
    "mutex",
    "optional",
    "set",
    "sstream",
    "stdexcept",
    "string",
    "system_error",
    "thread",
    "type_traits",
    "utility",
    "vector",
}
API_CONTRACT_ONLY_CLASSIFICATION = "api-contract-only-not-normative-conformance"
STANDARD_CALLBACK_MODELS = ("evoked", "immediate")
PLATFORM_INCLUDES = {
    ("service_report_creation_failure_tck.cpp", "windows.h"),
}


LOCAL_TCK_HEADERS = {
    "pending_negotiated_restore_tck.hpp",
    "service_report_creation_failure_tck.hpp",
    "tck_harness.hpp",
    "tck_directed_retraction_mixed_fanout.hpp",
    "tck_ownership_cancellation_idle.hpp",
    "tck_ownership_denial_idle.hpp",
    "tck_ownership_denial_query.hpp",
    "tck_ownership_transition_query.hpp",
    "tck_ownership_reacquisition_query.hpp",
    "tck_ownership_competing_if_available.hpp",
    "tck_ownership_mixed_if_available.hpp",
    "tck_ownership_unpublish_owned.hpp",
    "tck_restore_mixed_pending_time_role_callbacks.hpp",
    "tck_timed_restore_regional_attribute_multi_recipient.hpp",
    "tck_data_encoding.hpp",
    "tck_api_foundation.hpp",
    "tck_configuration_authorization.hpp",
    "tck_federation_mom_save_conditionals.hpp",
    "tck_federation_mom_save_status.hpp",
    "tck_factory_identity.hpp",
    "tck_timestamped_interaction_boundary_scenarios.hpp",
    "tck_flush_queue_nonlimiting_galt_scenarios.hpp",
    "tck_joined_federate_mom_state_save_restore.hpp",
    "tck_joined_federate_mom_galt_lits_periodic.hpp",
    "tck_joined_federate_mom_tso_length_periodic.hpp",
    "tck_joined_federate_mom_removed_object_count.hpp",
    "tck_joined_federate_mom_updates_sent_counts.hpp",
    "tck_joined_federate_mom_updates_sent_mim_attribute.hpp",
    "tck_joined_federate_mom_object_instances_updated_mim_attribute.hpp",
    "tck_joined_federate_mom_object_instances_reflected_mim_attribute.hpp",
    "tck_joined_federate_mom_reflections_received_mim_attribute.hpp",
    "tck_joined_federate_mom_object_instances_discovered_mim_attribute.hpp",
    "tck_joined_federate_mom_object_instances_removed_mim_attribute.hpp",
    "tck_joined_federate_mom_object_instances_deleted_mim_attribute.hpp",
    "tck_joined_federate_mom_object_lifecycle.hpp",
    "tck_joined_federate_mom_deletable_object_report.hpp",
    "tck_joined_federate_mom_updated_object_report.hpp",
    "tck_joined_federate_mom_updated_timestamped_object_report.hpp",
    "tck_joined_federate_mom_updated_multi_class_object_report.hpp",
    "tck_joined_federate_mom_updated_null_object_report.hpp",
    "tck_joined_federate_mom_reflected_null_object_report.hpp",
    "tck_joined_federate_mom_reflected_object_report.hpp",
    "tck_joined_federate_mom_reflected_timestamped_object_report.hpp",
    "tck_joined_federate_mom_reflected_multi_class_object_report.hpp",
    "tck_joined_federate_mom_deletable_object_count.hpp",
    "tck_receive_order_attribute_update.hpp",
    "tck_joined_federate_mom_interactions_received_counts.hpp",
    "tck_joined_federate_mom_directed_interactions_received_counts.hpp",
    "tck_joined_federate_mom_interactions_sent_counts.hpp",
    "tck_joined_federate_mom_directed_interactions_sent_counts.hpp",
    "tck_mom_transportation_type_change_request.hpp",
    "tck_joined_federate_mom_reflection_counts.hpp",
    "tck_joined_federate_mom_updated_object_count_periodic.hpp",
    "tck_joined_federate_mom_discovered_object_count_periodic.hpp",
    "tck_joined_federate_mom_deleted_object_count_periodic.hpp",
    "tck_joined_federate_mom_time_state_durations.hpp",
    "tck_order_type_controls.hpp",
    "tck_transport_order_type_change.hpp",
    "tck_receive_order_callback_cancellation.hpp",
    "tck_attribute_value_update_request_response.hpp",
    "tck_update_rate_scenarios.hpp",
    "tck_federation_mom_content_scenarios.hpp",
    "tck_federation_synchronization_mom_queries.hpp",
    "tck_multi_attribute_delivery_scenarios.hpp",
    "tck_portable_member_control_scenarios.hpp",
    "tck_transport_order_scenarios.hpp",
    "tck_attribute_relevance_scenarios.hpp",
    "tck_nrg_regulator_resignation.hpp",
    "tck_nrg_disabled_regulator_activation.hpp",
    "tck_constrained_tar_disable_release.hpp",
    "tck_save_active_callback_resignation.hpp",
    "tck_timed_save_fqr_callback_resignation.hpp",
    "tck_timed_save_tara_callback_resignation.hpp",
    "tck_timed_save_nmra_callback_resignation.hpp",
    "tck_timed_save_mixed_fqr_tar.hpp",
    "tck_timed_save_ordinary_before_fqr.hpp",
    "tck_timed_save_all_advance_modes.hpp",
    "tck_timed_save_per_member_tso.hpp",
    "tck_timed_save_flush_queue_admission.hpp",
    "tck_timed_save_available_admission.hpp",
    "tck_timed_save_next_message_admission.hpp",
    "tck_timed_save_available_next_message_member_admission.hpp",
    "tck_timed_save_resigned_constrained_member.hpp",
    "tck_restore_pending_flush_queue_request.hpp",
    "tck_restore_resignation_callback_lifetime.hpp",
    "tck_restore_pending_time_advance.hpp",
    "tck_restore_pending_time_regulation_callback.hpp",
    "tck_restore_pending_advance_modes.hpp",
    "tck_restore_pending_tso_nmr.hpp",
    "tck_restore_pending_tso_nmra_attribute.hpp",
    "tck_restore_pending_tso_nmra_delete.hpp",
    "tck_restore_mixed_tso_four_member.hpp",
    "tck_restore_terminal_tso_attribute.hpp",
    "tck_restore_terminal_tso_interaction.hpp",
    "tck_restore_terminal_tso_directed_interaction.hpp",
    "tck_restore_time_window_state.hpp",
    "tck_restore_deferred_lookahead.hpp",
    "tck_fqr_deferred_lookahead.hpp",
    "tck_restore_multi_pending_time_advance.hpp",
    "tck_restore_pending_flush_queue_immediate.hpp",
    "tck_restore_object_snapshot_status.hpp",
    "tck_restore_tso_retraction_uniqueness.hpp",
    "tck_restore_tso_directed_multi_recipient.hpp",
    "tck_restore_live_tso_retraction_record.hpp",
    "tck_restore_live_tso_directed_interaction.hpp",
    "tck_restore_live_tso_interaction_multi_recipient.hpp",
    "tck_restore_live_tso_attribute_multi_recipient.hpp",
    "tck_restore_default_region_attribute_multi_recipient.hpp",
    "tck_restore_regional_interaction_multi_recipient.hpp",
    "tck_restore_live_tso_object_deletion.hpp",  # Draft source; not catalog-promoted.
    "tck_joined_federate_mom_reflections_received_counts.hpp",
    "tck_main_scenarios.hpp",
    "tck_regional_auto_provide_scenarios.hpp",
    "tck_portable_mom_interaction_counts.hpp",
    "tck_portable_resignation_ownership.hpp",
    "tck_portable_service_report_scenarios.hpp",
    "tck_timestamped_interaction_scheduling_scenarios.hpp",
    "tck_custom_transportation_scenarios.hpp",
    "tck_custom_transportation_interaction_scenarios.hpp",
    "tck_object_removal_fanout_scenarios.hpp",
    "tck_post_resignation_valid_input_scenarios.hpp",
    "tck_federation_lifecycle_scenarios.hpp",
    "tck_inherited_derived_delivery_scenarios.hpp",
    "tck_callback_controls_scenarios.hpp",
    "tck_portable_mom_declaration_queries.hpp",
    "tck_portable_region_service_reports.hpp",
    "tck_lookup_lifecycle_scenarios.hpp",
    "tck_fom_module_scenarios.hpp",
    "tck_region_lifecycle_scenarios.hpp",
    "tck_delay_subscription_scenarios.hpp",
    "tck_logical_time.hpp",
    "tck_time_coordinated_interaction.hpp",
    "tck_empty_attribute_subscription.hpp",
    "tck_attribute_subscription_mode_transitions.hpp",
    "tck_nmr_future_timestamped_object_deletion.hpp",
    "tck_nmra_future_timestamped_object_deletion.hpp",
    "tck_tara_future_timestamped_object_deletion.hpp",
    "tck_portable_scenario_runner.hpp",
    "tck_timestamped_attribute_tara_router.hpp",
    "tck_timestamped_object_deletion_router.hpp",
    "tck_timestamped_attribute_pending_tara.hpp",
    "tck_named_update_rate_subscription.hpp",
    "tck_default_rate_no_reduction.hpp",
    "tck_accumulating_attribute_subscriptions.hpp",
    "tck_passive_subscription_no_start_registration.hpp",
    "tck_selective_attribute_unsubscription.hpp",
    "tck_whole_class_attribute_unsubscription.hpp",
    "tck_empty_attribute_unsubscription.hpp",
    "tck_options.hpp",
}
OFFICIAL_API_HEADERS = {
    "RTI/Enums.h",
    "RTI/Exception.h",
    "RTI/FederateAmbassador.h",
    "RTI/NullFederateAmbassador.h",
    "RTI/Handle.h",
    "RTI/RTI1516.h",
    "RTI/RTIambassador.h",
    "RTI/RTIambassadorFactory.h",
    "RTI/RangeBounds.h",
    "RTI/RtiConfiguration.h",
    "RTI/Typedefs.h",
    "RTI/VariableLengthData.h",
    "RTI/auth/AuthorizationResult.h",
    "RTI/auth/Authorizer.h",
    "RTI/auth/AuthorizerFactory.h",
    "RTI/auth/Credentials.h",
    "RTI/auth/HLAnoCredentials.h",
    "RTI/auth/HLAauthorizerFactoryFactory.h",
    "RTI/auth/HLAplainTextPassword.h",
    "RTI/encoding/BasicDataElements.h",
    "RTI/encoding/EncodingExceptions.h",
    "RTI/encoding/HLAextendableVariantRecord.h",
    "RTI/encoding/HLAfixedArray.h",
    "RTI/encoding/HLAfixedRecord.h",
    "RTI/encoding/HLAlogicalTime.h",
    "RTI/encoding/HLAlogicalTimeInterval.h",
    "RTI/encoding/HLAopaqueData.h",
    "RTI/encoding/HLAvariableArray.h",
    "RTI/encoding/HLAvariantRecord.h",
    "RTI/time/LogicalTime.h",
    "RTI/time/LogicalTimeFactory.h",
    "RTI/time/LogicalTimeInterval.h",
    "RTI/time/HLAfloat64Interval.h",
    "RTI/time/HLAfloat64Time.h",
    "RTI/time/HLAfloat64TimeFactory.h",
    "RTI/time/HLAinteger64Interval.h",
    "RTI/time/HLAinteger64Time.h",
    "RTI/time/HLAinteger64TimeFactory.h",
    "RTI/time/HLAlogicalTimeFactoryFactory.h",
    "RTI/libfedtime/LogicalTimeFactoryFactory.h",
}

SCENARIO_INVENTORY_PATTERN = re.compile(
    r"std::vector<std::string>\s+allScenarioIds\(\)\s*\{\s*"
    r"return\s*\{(?P<body>.*?)\};",
    re.DOTALL,
)


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def callback_models_for_scenario(scenario: dict[str, Any]) -> tuple[str, ...]:
    models = scenario.get("callback_models", STANDARD_CALLBACK_MODELS)
    return tuple(models)


def load_catalog(path: Path) -> dict[str, Any]:
    catalog = load_json(path)
    if catalog.get("schema_version") != 1 or catalog.get("kind") != "portable-cpp-tck-catalog":
        raise ValueError(f"unsupported C++ TCK catalog: {path}")
    scenarios = catalog.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        raise ValueError("C++ TCK catalog must contain scenarios")
    ids = [item.get("id") for item in scenarios]
    if any(not isinstance(item, str) or not item for item in ids) or len(ids) != len(set(ids)):
        raise ValueError("C++ TCK scenario IDs must be unique non-empty strings")
    parity_catalog = catalog.get("parity_catalog")
    if not isinstance(parity_catalog, str) or not parity_catalog:
        raise ValueError("C++ TCK catalog must name its cross-language parity catalog")
    if not isinstance(catalog.get("promotion_rule"), str) or not catalog["promotion_rule"]:
        raise ValueError("C++ TCK catalog must document its promotion rule")
    parity_path = ROOT / parity_catalog
    parity = load_json(parity_path)
    if parity.get("kind") != "portable-java-tck-catalog":
        raise ValueError(f"unsupported parity catalog: {parity_path}")
    parity_ids = {
        item.get("id") for item in parity.get("scenarios", [])
        if isinstance(item, dict)
    }
    for scenario in scenarios:
        for field in (
            "category",
            "title",
            "cpp_test",
            "runner_id",
            "default_status",
            "promotion",
        ):
            if not isinstance(scenario.get(field), str) or not scenario[field]:
                raise ValueError(f"{scenario.get('id', '<unknown>')} missing {field}")
        if not scenario.get("api_methods"):
            raise ValueError(f"{scenario['id']} must link standard C++ API methods")
        mapping_classification = scenario.get("mapping_classification")
        if not scenario.get("requirement_ids"):
            if mapping_classification != API_CONTRACT_ONLY_CLASSIFICATION:
                raise ValueError(
                    f"{scenario['id']} must have stable requirement IDs or the explicit "
                    "API-contract-only classification"
                )
            if any(
                scenario.get(field)
                for field in (
                    "selected_requirements_lab_requirement_ids",
                    "standard_sections",
                    "requirement_mappings",
                )
            ):
                raise ValueError(
                    f"{scenario['id']} cannot claim normative mappings when classified "
                    "API-contract-only"
                )
        elif mapping_classification == API_CONTRACT_ONLY_CLASSIFICATION:
            raise ValueError(
                f"{scenario['id']} cannot combine normative requirement IDs with the "
                "API-contract-only classification"
            )
        if not scenario.get("contract_refs"):
            raise ValueError(f"{scenario['id']} must reference compliance contracts")
        native_equivalents = scenario.get("native_equivalent_runner_ids", [])
        if not isinstance(native_equivalents, list) or any(
            not isinstance(item, str) or not item for item in native_equivalents
        ):
            raise ValueError(
                f"{scenario['id']} has invalid native equivalent runner IDs"
            )
        if scenario["default_status"] not in {"run", "adapter-required", "unsupported"}:
            raise ValueError(f"{scenario['id']} has invalid default_status")
        if scenario["promotion"] not in {"promoted", "candidate"}:
            raise ValueError(f"{scenario['id']} has invalid promotion")
        callback_models = scenario.get("callback_models", list(STANDARD_CALLBACK_MODELS))
        if (
            not isinstance(callback_models, list)
            or not callback_models
            or any(
                not isinstance(model, str) or model not in STANDARD_CALLBACK_MODELS
                for model in callback_models
            )
            or len(callback_models) != len(set(callback_models))
        ):
            raise ValueError(f"{scenario['id']} has invalid callback_models")
        if set(callback_models) != set(STANDARD_CALLBACK_MODELS):
            scope_note = scenario.get("callback_model_scope_note")
            if not isinstance(scope_note, str) or not scope_note.strip():
                raise ValueError(
                    f"{scenario['id']} needs a callback_model_scope_note when its "
                    "evidence is limited to one callback model"
                )
        for reference in scenario["contract_refs"]:
            contract = ROOT / "compliance" / "requirements-lab" / reference
            if not contract.is_file():
                raise ValueError(f"{scenario['id']} references absent contract {reference}")
        source, separator, symbol = scenario["cpp_test"].partition("#")
        if not separator or not symbol:
            raise ValueError(f"{scenario['id']} cpp_test must be source#symbol")
        source_path = SOURCE_ROOT / source
        if not source_path.is_file():
            raise ValueError(f"{scenario['id']} references absent source {source}")
        if symbol not in source_path.read_text(encoding="utf-8"):
            raise ValueError(f"{scenario['id']} references absent source symbol {symbol}")
        if scenario["id"].startswith("java-tck."):
            if scenario["id"] not in parity_ids:
                raise ValueError(
                    f"{scenario['id']} is not present in the Java TCK parity catalog"
                )
            if scenario["runner_id"] != scenario["id"]:
                raise ValueError(
                    f"{scenario['id']} must reuse its Java TCK ID as runner_id"
                )
        elif scenario["id"].startswith("cpp-tck.") and not scenario.get("parity"):
            raise ValueError(f"{scenario['id']} must explain its parity extension status")
    represented_java_ids = {
        scenario["id"]
        for scenario in scenarios
        if scenario["id"].startswith("java-tck.")
    }
    for scenario in scenarios:
        parity_id = scenario.get("parity")
        if isinstance(parity_id, str) and parity_id.startswith("java-tck."):
            if parity_id not in parity_ids:
                raise ValueError(
                    f"{scenario['id']} references absent Java parity scenario {parity_id}"
                )
            represented_java_ids.add(parity_id)
    missing_java_ids = sorted(
        scenario["id"]
        for scenario in parity.get("scenarios", [])
        if scenario.get("default_status") == "run"
        and scenario.get("id") not in represented_java_ids
    )
    if missing_java_ids:
        raise ValueError(
            "Java TCK run scenarios are missing from the C++ parity catalog: "
            + ", ".join(missing_java_ids)
        )
    return catalog


def validate_sources() -> list[str]:
    findings: list[str] = []
    source = SOURCE_ROOT / "src"
    if not source.is_dir():
        return [f"portable C++ TCK source root is absent: {source}"]
    forbidden_tokens = (
        "umbra",
        "he" + "lios",
        "internal/",
        "jni",
        "nativebridge",
        "catch2",
    )
    include_pattern = re.compile(r"^\s*#include\s+([<\"])([^>\"]+)[>\"]\s*$")
    for path in source.rglob("*"):
        if path.suffix.lower() not in {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        lowered = text.lower()
        for token in forbidden_tokens:
            if token in lowered:
                findings.append(f"portable C++ TCK contains forbidden token {token!r}: {path}")
        for line_number, line in enumerate(text.splitlines(), start=1):
            match = include_pattern.match(line)
            if match is None:
                continue
            delimiter, include = match.groups()
            if (
                delimiter == '"'
                and not (
                    (path.name == "portable_tck.cpp" and include == "main.cpp")
                    or include in LOCAL_TCK_HEADERS
                )
            ):
                findings.append(f"portable C++ TCK uses a local include at {path}:{line_number}: {include}")
            elif (
                delimiter == "<"
                and include not in OFFICIAL_API_HEADERS
                and include not in STANDARD_HEADERS
                and (path.name, include) not in PLATFORM_INCLUDES
            ):
                findings.append(f"portable C++ TCK includes a non-standard header at {path}:{line_number}: {include}")
    fom_directory = SOURCE_ROOT / "fom"
    required_fom_names = (
        "p0-tck.xml",
        "fom-model-tck.xml",
        "ddm-tck.xml",
        "ddm-multi-attribute-tck.xml",
        "ddm-three-dimensional-tck.xml",
        "switches-tck.xml",
        "fom-extension-tck.xml",
        "invalid-malformed-tck.xml",
        "invalid-namespace-tck.xml",
        "invalid-duplicate-tck.xml",
    )
    for name in required_fom_names:
        fom = fom_directory / name
        if not fom.is_file():
            findings.append(f"portable C++ TCK FOM is absent: {fom}")
    for fom_path in sorted(fom_directory.glob("*.xml")):
        if any(token in fom_path.read_text(encoding="utf-8").lower() for token in forbidden_tokens):
            findings.append(
                f"portable C++ TCK FOM contains a forbidden provider token: {fom_path}"
            )
    return findings


def scenario_inventory() -> list[str]:
    source_root = SOURCE_ROOT / "src"
    main_path = source_root / "main.cpp"
    main_source = main_path.read_text(encoding="utf-8")
    match = SCENARIO_INVENTORY_PATTERN.search(main_source)
    if match is None:
        return []
    main_inventory = re.findall(r'"([^"]+)"', match.group("body"))
    inventory = list(main_inventory)
    # Scenario routers can live in cohesive source files outside the main
    # inventory declaration. Include their portable IDs so extracting a route
    # does not make catalog validation report a false missing-inventory error.
    scenario_id_pattern = re.compile(
        r'constexpr\s+char\s+\w+\[\]\s*=\s*"(cpp-tck\.[^"]+)"'
    )
    for path in sorted(source_root.rglob("*.cpp")):
        if path == main_path:
            continue
        source_text = path.read_text(encoding="utf-8")
        for scenario_id in scenario_id_pattern.findall(source_text):
            if scenario_id not in inventory:
                inventory.append(scenario_id)
    return inventory


def validate_scenario_inventory(catalog: dict[str, Any]) -> list[str]:
    """Ensure the executable's default scenario inventory matches the catalog."""
    source_path = SOURCE_ROOT / "src" / "main.cpp"
    inventory = scenario_inventory()
    if not inventory:
        return [f"portable C++ TCK scenario inventory is absent: {source_path}"]
    catalog_ids = {scenario["id"] for scenario in catalog["scenarios"]}
    findings: list[str] = []
    duplicates = sorted(
        scenario_id
        for scenario_id in set(inventory)
        if inventory.count(scenario_id) > 1
    )
    if duplicates:
        findings.append(
            "portable C++ TCK scenario inventory contains duplicates: "
            + ", ".join(duplicates)
        )
    inventory_ids = set(inventory)
    missing = sorted(catalog_ids - inventory_ids)
    if missing:
        findings.append(
            "portable C++ TCK scenario inventory is missing catalog scenarios: "
            + ", ".join(missing)
        )
    unexpected = sorted(inventory_ids - catalog_ids)
    if unexpected:
        findings.append(
            "portable C++ TCK scenario inventory contains unknown scenarios: "
            + ", ".join(unexpected)
        )
    return findings


def validate(catalog_path: Path = DEFAULT_CATALOG) -> dict[str, Any]:
    catalog = load_catalog(catalog_path)
    findings = validate_sources()
    findings.extend(validate_scenario_inventory(catalog))
    inventory_count = len(scenario_inventory())
    promotion_counts = {
        state: sum(1 for item in catalog["scenarios"] if item["promotion"] == state)
        for state in ("promoted", "candidate")
    }
    return {
        "valid": not findings,
        "findings": findings,
        "scenario_count": len(catalog["scenarios"]),
        "scenario_inventory_count": inventory_count,
        "promotion_counts": promotion_counts,
        "source_boundary": catalog["source_boundary"],
    }


def is_expected_adapter_skip(result: dict[str, Any]) -> bool:
    return (
        result.get("status"),
        result.get("id"),
        result.get("message"),
    ) in {
        ("skipped", scenario_id, message)
        for scenario_id, message in EXPECTED_ADAPTER_MANAGED_SKIPS
    }


def validate_results(
    path: Path,
    catalog: dict[str, Any],
    promotion: str | None = None,
    scenario_ids: list[str] | None = None,
) -> list[str]:
    data = load_json(path)
    findings: list[str] = []
    if data.get("kind") != "hla-rti-cpp-tck-evidence":
        findings.append(f"not C++ TCK evidence: {path}")
    records = data.get("results")
    if not isinstance(records, list):
        return ["evidence results must be an array"]
    selected = [
        item
        for item in catalog["scenarios"]
        if promotion is None or item["promotion"] == promotion
    ]
    if scenario_ids:
        if len(set(scenario_ids)) != len(scenario_ids):
            raise ValueError("--scenario values must be unique")
        by_id = {item["id"]: item for item in catalog["scenarios"]}
        unknown = [scenario_id for scenario_id in scenario_ids if scenario_id not in by_id]
        if unknown:
            raise ValueError("unknown catalog scenario(s): " + ", ".join(unknown))
        selected = [by_id[scenario_id] for scenario_id in scenario_ids]
        if promotion is not None:
            wrong_promotion = [
                item["id"] for item in selected if item["promotion"] != promotion
            ]
            if wrong_promotion:
                raise ValueError(
                    "scenario(s) do not belong to requested promotion set: "
                    + ", ".join(wrong_promotion)
                )
    scenario_by_runner = {item["runner_id"]: item for item in selected}
    expected = set(scenario_by_runner)
    actual = {item.get("id") for item in records}
    missing = sorted(expected - actual)
    if missing:
        findings.append("evidence is missing scenarios: " + ", ".join(missing))
    unexpected = sorted(actual - expected)
    if unexpected:
        findings.append("evidence contains unexpected scenarios: " + ", ".join(unexpected))
    seen: set[tuple[Any, Any]] = set()
    for result in records:
        if result.get("status") not in {"passed", "skipped", "failed"}:
            findings.append(f"evidence has invalid result status: {result}")
        if result.get("callback_model") not in {"evoked", "immediate"}:
            findings.append(f"evidence has invalid callback model: {result}")
        if result.get("id") not in expected:
            continue
        allowed_models = callback_models_for_scenario(
            scenario_by_runner[result["id"]]
        )
        if result.get("callback_model") not in allowed_models:
            findings.append(
                "evidence uses an undeclared callback model for "
                f"{result.get('id')}: {result.get('callback_model')}"
            )
        key = (result.get("id"), result.get("callback_model"))
        if key in seen:
            findings.append(f"evidence contains duplicate scenario result: {key}")
        seen.add(key)
    expected_keys = {
        (scenario_id, callback_model)
        for scenario_id, scenario in scenario_by_runner.items()
        for callback_model in callback_models_for_scenario(scenario)
    }
    missing_keys = sorted(expected_keys - seen)
    if missing_keys:
        findings.append(
            "evidence is missing scenario/callback results: "
            + ", ".join(f"{scenario}/{callback}" for scenario, callback in missing_keys)
        )
    if any(result.get("status") == "failed" for result in records):
        findings.append("evidence contains failed scenarios")
    if promotion == "promoted" and any(
        result.get("status") != "passed"
        and not is_expected_adapter_skip(result)
        for result in records
        if result.get("id") in expected
    ):
        findings.append("promoted evidence cannot contain skipped or failed scenarios")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--results", type=Path)
    parser.add_argument(
        "--promotion",
        choices=("all", "promoted", "candidate"),
        default="all",
        help="Evidence set to validate; promoted requires passing results for each declared callback model",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        default=[],
        help="Validate this catalog scenario in the evidence; repeat for a focused slice",
    )
    arguments = parser.parse_args()
    try:
        result = validate(arguments.catalog)
        if arguments.results:
            catalog = load_catalog(arguments.catalog)
            selected_promotion = None if arguments.promotion == "all" else arguments.promotion
            result["findings"].extend(
                validate_results(
                    arguments.results,
                    catalog,
                    selected_promotion,
                    arguments.scenario,
                )
            )
            result["valid"] = not result["findings"]
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0 if result["valid"] else 1
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(json.dumps({"valid": False, "findings": [str(error)]}, indent=2))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
