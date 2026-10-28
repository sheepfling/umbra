# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import MappingRowInputs


def run_object_mapping_rows(inputs: MappingRowInputs) -> None:
    contract_links = inputs.contract_links
    declared_executables = inputs.declared_executables
    index = inputs.index
    ownership_transfer = inputs.ownership_transfer
    standard_requirements = inputs.standard_requirements
    tests = inputs.tests
    # The service-report encoding reconciliation is deliberately represented
    # by exact plan rows rather than a source-only queue.  Keep one of those
    # rows protected as the canonical example for the one-case handoff: its
    # mapping state, source pointer, standard subsection, and focused lane
    # must all be visible without reopening the full plan.
    service_report_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-service-report-query-federation-restore-status-unit"
        ),
        None,
    )
    if not isinstance(service_report_case, dict):
        raise AssertionError("service-report encoding reconciliation row is absent")
    if (
        service_report_case.get("traceability_state") != "requirements-mapped"
        or query_rti_work.source_location_text(service_report_case)
        != "cpp/tests/mom_service_report_encoding_catch2.cpp:1490"
        or "hla-1516.1-2025:clause-11.5.1"
        not in service_report_case.get("standard_sections", [])
    ):
        raise AssertionError("service-report encoding row lost its bounded mapping")
    service_report_case_card = query_rti_work.case_card_record(
        service_report_case,
        index,
        "umbra-cpp-mom-service-report-query-federation-restore-status-unit",
    )
    if (
        service_report_case_card.get("traceability_state") != "requirements-mapped"
        or service_report_case_card.get("lane") != "service-report-encoding"
        or "ctest" not in service_report_case_card.get("commands", {})
    ):
        raise AssertionError("exact service-report case card lost its mapping handles")
    if "traceability: requirements-mapped" not in query_rti_work.text_case_card(
        service_report_case_card,
        compact=True,
    ):
        raise AssertionError("case-card text omitted the mapping state")
    focused_case_expectations = {
        "umbra-cpp-process-time-advance-time-regulation-pending-integration": (
            "time-role",
            3,
        ),
        "umbra-cpp-process-time-advance-federation-scheduler-integration": (
            "process-time-advance-federation-scheduler",
            4,
        ),
        "umbra-cpp-public-fresh-registry-federation-save-restore-filesystem-asymmetric-mixed-negotiated-confirmation-integration": (
            "attribute-ownership-acquisition",
            19,
        ),
        "umbra-cpp-public-fresh-registry-federation-save-restore-filesystem-asymmetric-mixed-negotiated-confirmation-reverse-integration": (
            "attribute-ownership-acquisition-if-available",
            19,
        ),
    }
    for case_id, (expected_lane, expected_pairs) in focused_case_expectations.items():
        mapped_case = next(
            (test for test in tests if test.get("id") == case_id),
            None,
        )
        if not isinstance(mapped_case, dict):
            raise AssertionError(f"focused case is absent from the plan: {case_id}")
        case_card = query_rti_work.case_card_record(mapped_case, index, case_id)
        explicit_ctest_filter = mapped_case.get("ctest_filter")
        if isinstance(explicit_ctest_filter, str) and explicit_ctest_filter.strip():
            expected_ctest = (
                "ctest --test-dir <build-dir> -C Debug -R "
                f'"{explicit_ctest_filter}" --output-on-failure'
            )
        else:
            lane_handles = index.get("mapping", {}).get("lane_handles", {})
            expected_ctest = query_rti_work.lane_ctest_command(
                lane_handles.get(expected_lane)
                if isinstance(lane_handles, dict)
                else None
            )
        if (
            case_card.get("lane") != expected_lane
            or case_card.get("source_target") is not None
            or len(case_card.get("requirement_section_mappings", [])) != expected_pairs
            or case_card.get("test_case") != mapped_case.get("test_case")
            or not isinstance(expected_ctest, str)
            or case_card.get("commands", {}).get("ctest") != expected_ctest
        ):
            raise AssertionError(
                f"exact case card lost focused lane/source/CTest/mapping handles: {case_id}"
            )
    object_ddm_item = next(
        (
            item
            for item in index.get("items", [])
            if item.get("id") == "object-ddm-ownership"
        ),
        None,
    )
    if not isinstance(object_ddm_item, dict):
        raise AssertionError("object-ddm roadmap family is absent")
    if "passive-regional-interaction-transition" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("passive regional interaction lane is not roadmap-indexed")
    if "focused-positive-dimensional-regional-interaction" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "focused positive-dimensional regional interaction lane is not roadmap-indexed"
        )
    if "multi-region-regional-interaction" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("multi-region interaction lane is not roadmap-indexed")
    if "subscription-dimension-validation" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "regional subscription-dimension lane is not roadmap-indexed"
        )
    if "subscription-empty-set" not in object_ddm_item.get("focused_lane_tags", []):
        raise AssertionError(
            "empty regional subscription-set lane is not roadmap-indexed"
        )
    if "passive-regional-object-attribute-transition" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "passive regional object-attribute lane is not roadmap-indexed"
        )
    if "whole-class-unsubscribe" not in object_ddm_item.get("focused_lane_tags", []):
        raise AssertionError("whole-class default-region lane is not roadmap-indexed")
    if "no-common-dimension" not in object_ddm_item.get("focused_lane_tags", []):
        raise AssertionError("no-common-dimension lane is not roadmap-indexed")
    if "mixed-region-interaction-validation" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("mixed-region interaction lane is not roadmap-indexed")
    passive_regional_transition_focus = query_rti_work.focused_lane_result(
        index, tests, "passive-regional-interaction-transition", limit=0
    )
    if passive_regional_transition_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused passive regional interaction lane is not complete"
        )
    if passive_regional_transition_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused passive regional interaction mapped count drifted"
        )
    if passive_regional_transition_focus.get("requirement_count") != 4:
        raise AssertionError(
            "focused passive regional interaction requirement count drifted"
        )
    if passive_regional_transition_focus.get("standard_section_count") != 3:
        raise AssertionError(
            "focused passive regional interaction section count drifted"
        )
    if passive_regional_transition_focus.get("requirement_section_pair_count") != 4:
        raise AssertionError(
            "focused passive regional interaction direct-pair count drifted"
        )
    passive_regional_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for row in passive_regional_transition_focus.get(
            "requirement_section_mappings", []
        )
    }
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-237-l28-8",
        "hla-1516.1-2025:clause-9.10.3",
    ) not in passive_regional_pairs:
        raise AssertionError(
            "regional active/passive requirement lost its direct §9.10.3 mapping"
        )
    passive_regional_transition_handles = passive_regional_transition_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(passive_regional_transition_handles, dict)
        or passive_regional_transition_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
    ):
        raise AssertionError(
            "focused passive regional interaction target handle drifted"
        )
    if "interaction-subscription-mode-exclusivity" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "active/passive interaction mode lane is not roadmap-indexed"
        )
    interaction_mode_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-request-subscriptions-report-integration"
        ),
        None,
    )
    if not isinstance(interaction_mode_case, dict):
        raise AssertionError("active/passive interaction mode test plan row is absent")
    if (
        query_rti_work.source_location_text(interaction_mode_case)
        != ("cpp/tests/ieee1516_2025_federation_mom_request_subscriptions_catch2.cpp:4")
        or interaction_mode_case.get("assertions") != 129
    ):
        raise AssertionError("active/passive interaction mode source/evidence drifted")
    interaction_mode_focus = query_rti_work.focused_lane_result(
        index, tests, "interaction-subscription-mode-exclusivity", limit=0
    )
    if (
        interaction_mode_focus.get("lane_state") != "complete"
        or interaction_mode_focus.get("mapped_test_count") != 1
        or interaction_mode_focus.get("assertion_count") != 129
        or interaction_mode_focus.get("requirement_count") != 1
        or interaction_mode_focus.get("standard_section_count") != 1
        or interaction_mode_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("active/passive interaction mode focus card drifted")
    interaction_mode_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for row in interaction_mode_focus.get("requirement_section_mappings", [])
    }
    if (
        "requirement-candidate-content-clauses-11-management-object-model-page-290-l174-57",
        "hla-1516.1-2025:clause-11.4.1",
    ) not in interaction_mode_pairs:
        raise AssertionError(
            "interaction-subscription report lost its direct §11.4.1 mapping"
        )
    interaction_mode_handles = interaction_mode_focus.get("lane_handles")
    if (
        not isinstance(interaction_mode_handles, dict)
        or interaction_mode_handles.get("catch2_target") != "umbra_ieee1516_2025_catch2"
        or "Embedded MOM requestSubscriptions reports active passive and directed subscription state"
        not in interaction_mode_handles.get("ctest_filter", "")
    ):
        raise AssertionError("active/passive interaction mode execution handle drifted")
    if "interaction-sent-class-preservation" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("sent interaction-class lane is not roadmap-indexed")
    sent_class_case = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-receive-order-interaction-integration"
        ),
        None,
    )
    if not isinstance(sent_class_case, dict):
        raise AssertionError("sent interaction-class test plan row is absent")
    if (
        query_rti_work.source_location_text(sent_class_case)
        != ("cpp/tests/receive_order_interaction_catch2.cpp:97")
        or sent_class_case.get("assertions") != 89
    ):
        raise AssertionError("sent interaction-class source/evidence drifted")
    sent_class_focus = query_rti_work.focused_lane_result(
        index, tests, "interaction-sent-class-preservation", limit=0
    )
    if (
        sent_class_focus.get("lane_state") != "complete"
        or sent_class_focus.get("mapped_test_count") != 1
        or sent_class_focus.get("assertion_count") != 89
        or sent_class_focus.get("requirement_count") != 11
        or sent_class_focus.get("standard_section_count") != 5
        or sent_class_focus.get("requirement_section_pair_count") != 11
    ):
        raise AssertionError("sent interaction-class focus card drifted")
    sent_class_requirement_clauses = {
        "requirement-candidate-content-clauses-05-declaration-management-page-083-l176-57": "clause-5.1.3",
        "requirement-candidate-content-clauses-05-declaration-management-page-084-l32-10": "clause-5.1.3",
        "requirement-candidate-content-clauses-06-object-management-page-124-l17-4": "clause-6.12.4",
        "requirement-candidate-content-clauses-06-object-management-page-125-l88-25": "clause-6.13",
    }
    case_requirement_ids = query_rti_work.strings(
        sent_class_case.get("lab_requirement_ids")
    )
    for requirement_id, expected_clause in sent_class_requirement_clauses.items():
        if requirement_id not in case_requirement_ids:
            raise AssertionError(
                "interaction-send requirement disappeared from its test row"
            )
        standard = query_rti_work.requirement_view(
            requirement_id, standard_requirements, contract_links
        ).get("standard")
        if (
            not isinstance(standard, dict)
            or standard.get("document_id") != "hla-1516.1-2025"
            or standard.get("clause_id") != expected_clause
        ):
            raise AssertionError(
                "interaction-send requirement lost its direct 2025 clause mapping"
            )
    sent_class_handles = sent_class_focus.get("lane_handles")
    if (
        not isinstance(sent_class_handles, dict)
        or sent_class_handles.get("catch2_target")
        != "umbra_receive_order_interaction_catch2"
        or "Embedded receive-order Send Interaction honors 2025 promotion and callback lifecycle"
        not in sent_class_handles.get("ctest_filter", "")
    ):
        raise AssertionError("sent interaction-class execution handle drifted")

    ddm_expansion_lane = "ddm-clause6-service-expansion"
    ddm_expansion_requirement = "requirement-candidate-content-clauses-09-data-distribution-management-page-224-l125-40"
    if ddm_expansion_lane not in object_ddm_item.get("focused_lane_tags", []):
        raise AssertionError("Clause 6 DDM-expansion lane is not roadmap-indexed")
    ddm_expansion_case_ids = {
        "umbra-cpp-regional-object-attribute-routing-integration",
        "umbra-cpp-interaction-region-integration",
        "umbra-cpp-regional-attribute-value-update-request-integration",
    }
    ddm_expansion_cases = {
        test.get("id"): test
        for test in tests
        if test.get("id") in ddm_expansion_case_ids
    }
    if set(ddm_expansion_cases) != ddm_expansion_case_ids:
        raise AssertionError(
            "one of the three Clause 6 regional-service test rows is absent"
        )
    for test_id, test in ddm_expansion_cases.items():
        if ddm_expansion_lane not in query_rti_work.strings(
            test.get("tags")
        ) or ddm_expansion_requirement not in query_rti_work.strings(
            test.get("lab_requirement_ids")
        ):
            raise AssertionError(
                "Clause 6 DDM-expansion test lost its lane or l125 mapping"
            )
        surfaces = query_rti_work.strings(test.get("selected_cpp_api_surface_ids"))
        expected_surface = {
            "umbra-cpp-regional-object-attribute-routing-integration": "api.2025.cpp.rtiambassador.registerobjectinstancewithregions.8a2b3b69ac22",
            "umbra-cpp-interaction-region-integration": "api.2025.cpp.rtiambassador.sendinteractionwithregions.dc826d06f8e1",
            "umbra-cpp-regional-attribute-value-update-request-integration": "api.2025.cpp.rtiambassador.requestattributevalueupdatewithregions.be07d650bd8d",
        }[test_id]
        if expected_surface not in surfaces:
            raise AssertionError(
                "Clause 6 DDM-expansion test lost its regional API surface"
            )
    ddm_expansion_standard = query_rti_work.requirement_view(
        ddm_expansion_requirement, standard_requirements, contract_links
    ).get("standard")
    if (
        not isinstance(ddm_expansion_standard, dict)
        or ddm_expansion_standard.get("document_id") != "hla-1516.1-2025"
        or ddm_expansion_standard.get("clause_id") != "clause-9.1.6"
    ):
        raise AssertionError("DDM-expansion requirement lost its direct §9.1.6 mapping")
    ddm_expansion_focus = query_rti_work.focused_lane_result(
        index, tests, ddm_expansion_lane, limit=0
    )
    if (
        ddm_expansion_focus.get("lane_state") != "complete"
        or ddm_expansion_focus.get("mapped_test_count") != 3
        or ddm_expansion_focus.get("assertion_count") != 61
        or ddm_expansion_focus.get("requirement_count") != 42
        or ddm_expansion_focus.get("standard_section_count") != 17
        or ddm_expansion_focus.get("requirement_section_pair_count") != 42
    ):
        raise AssertionError("Clause 6 DDM-expansion focused query card drifted")

    negotiated_confirmation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-negotiated-attribute-ownership-divestiture-integration"
        ),
        None,
    )
    if not isinstance(negotiated_confirmation, dict):
        raise AssertionError("negotiated ownership confirmation plan row is absent")
    if (
        query_rti_work.source_location_text(negotiated_confirmation)
        != (
            "cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp:204"
        )
        or negotiated_confirmation.get("assertions") != 103
    ):
        raise AssertionError(
            "negotiated ownership confirmation source/evidence drifted"
        )
    if negotiated_confirmation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("negotiated ownership confirmation row is not mapped")
    negotiated_confirmation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "negotiated-divestiture-confirmation-pending",
        limit=0,
    )
    if (
        negotiated_confirmation_focus.get("lane_state") != "complete"
        or negotiated_confirmation_focus.get("mapped_test_count") != 1
        or negotiated_confirmation_focus.get("assertion_count") != 103
        or negotiated_confirmation_focus.get("requirement_count") != 14
        or negotiated_confirmation_focus.get("standard_section_count") != 7
        or negotiated_confirmation_focus.get("requirement_section_pair_count") != 14
    ):
        raise AssertionError("negotiated ownership confirmation lane card drifted")
    negotiated_confirmation_handles = negotiated_confirmation_focus.get("lane_handles")
    if (
        not isinstance(negotiated_confirmation_handles, dict)
        or negotiated_confirmation_handles.get("catch2_target")
        != "umbra_negotiated_attribute_ownership_divestiture_pending_catch2"
    ):
        raise AssertionError("negotiated ownership confirmation target handle drifted")
    if "negotiated-divestiture-confirmation-pending" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "negotiated ownership confirmation lane is not roadmap-indexed"
        )
    cancellation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-cancellation-integration"
        ),
        None,
    )
    if not isinstance(cancellation, dict):
        raise AssertionError(
            "attribute-ownership acquisition cancellation plan row is absent"
        )
    if (
        query_rti_work.source_location_text(cancellation)
        != ("cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp:199")
        or cancellation.get("assertions") != 51
    ):
        raise AssertionError(
            "attribute-ownership acquisition cancellation source/evidence drifted"
        )
    if cancellation.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "attribute-ownership acquisition cancellation row is not mapped"
        )
    cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "cancel-attribute-ownership-acquisition-confirmation-boundary",
        limit=0,
    )
    if (
        cancellation_focus.get("lane_state") != "complete"
        or cancellation_focus.get("mapped_test_count") != 1
        or cancellation_focus.get("assertion_count") != 51
        or cancellation_focus.get("requirement_count") != 18
        or cancellation_focus.get("standard_section_count") != 5
        or cancellation_focus.get("requirement_section_pair_count") != 18
    ):
        raise AssertionError(
            "attribute-ownership acquisition cancellation lane card drifted"
        )
    cancellation_handles = cancellation_focus.get("lane_handles")
    if (
        not isinstance(cancellation_handles, dict)
        or cancellation_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_cancellation_catch2"
    ):
        raise AssertionError(
            "attribute-ownership acquisition cancellation target handle drifted"
        )
    if cancellation_handles.get("ctest_filter") != (
        "^umbra\\.attribute_ownership_acquisition_cancellation\\.catch2\\.Embedded Cancel "
        "Attribute Ownership Acquisition honors the 2025 confirmation boundary$"
    ):
        raise AssertionError(
            "attribute-ownership acquisition cancellation CTest filter drifted"
        )
    if (
        "cancel-attribute-ownership-acquisition-confirmation-boundary"
        not in object_ddm_item.get("focused_lane_tags", [])
    ):
        raise AssertionError(
            "attribute-ownership acquisition cancellation lane is not roadmap-indexed"
        )
    acquisition_release = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-acquisition-integration"
        ),
        None,
    )
    if not isinstance(acquisition_release, dict):
        raise AssertionError(
            "attribute-ownership acquisition release plan row is absent"
        )
    if (
        query_rti_work.source_location_text(acquisition_release)
        != (
            "cpp/tests/ieee1516_2025_attribute_ownership_acquisition_release_callbacks_catch2.cpp:4"
        )
        or acquisition_release.get("assertions") != 73
    ):
        raise AssertionError(
            "attribute-ownership acquisition release source/evidence drifted"
        )
    if acquisition_release.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "attribute-ownership acquisition release row is not mapped"
        )
    acquisition_release_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-acquisition-release-callback",
        limit=0,
    )
    if (
        acquisition_release_focus.get("lane_state") != "complete"
        or acquisition_release_focus.get("mapped_test_count") != 1
        or acquisition_release_focus.get("assertion_count") != 73
        or acquisition_release_focus.get("requirement_count") != 15
        or acquisition_release_focus.get("standard_section_count") != 5
        or acquisition_release_focus.get("requirement_section_pair_count") != 15
    ):
        raise AssertionError(
            "attribute-ownership acquisition release lane card drifted"
        )
    acquisition_release_handles = acquisition_release_focus.get("lane_handles")
    if (
        not isinstance(acquisition_release_handles, dict)
        or acquisition_release_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
    ):
        raise AssertionError(
            "attribute-ownership acquisition release target handle drifted"
        )
    if acquisition_release_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded Attribute Ownership Acquisition "
        "honors 2025 release and denial callbacks$"
    ):
        raise AssertionError(
            "attribute-ownership acquisition release CTest filter drifted"
        )
    if "attribute-ownership-acquisition-release-callback" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "attribute-ownership acquisition release lane is not roadmap-indexed"
        )
    release_denied_multi = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-release-denied-multi-acquirer-integration"
        ),
        None,
    )
    if not isinstance(release_denied_multi, dict):
        raise AssertionError("multi-acquirer Release Denied plan row is absent")
    if (
        query_rti_work.source_location_text(release_denied_multi)
        != (
            "cpp/tests/ieee1516_2025_attribute_ownership_release_denied_multi_acquirer_catch2.cpp:4"
        )
        or release_denied_multi.get("assertions") != 53
    ):
        raise AssertionError("multi-acquirer Release Denied source/evidence drifted")
    if release_denied_multi.get("traceability_state") != "requirements-mapped":
        raise AssertionError("multi-acquirer Release Denied row is not mapped")
    release_denied_multi_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-release-denied-multi-acquirer",
        limit=0,
    )
    if (
        release_denied_multi_focus.get("lane_state") != "complete"
        or release_denied_multi_focus.get("mapped_test_count") != 1
        or release_denied_multi_focus.get("assertion_count") != 53
        or release_denied_multi_focus.get("requirement_count") != 4
        or release_denied_multi_focus.get("standard_section_count") != 3
        or release_denied_multi_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("multi-acquirer Release Denied lane card drifted")
    release_denied_multi_handles = release_denied_multi_focus.get("lane_handles")
    if (
        not isinstance(release_denied_multi_handles, dict)
        or release_denied_multi_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
    ):
        raise AssertionError("multi-acquirer Release Denied target handle drifted")
    if release_denied_multi_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded Attribute Ownership Release Denied "
        "reaches all 2025 regular acquirers$"
    ):
        raise AssertionError("multi-acquirer Release Denied CTest filter drifted")
    if "attribute-ownership-release-denied-multi-acquirer" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "multi-acquirer Release Denied lane is not roadmap-indexed"
        )
    divestiture_if_wanted = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-divestiture-if-wanted-integration"
        ),
        None,
    )
    if not isinstance(divestiture_if_wanted, dict):
        raise AssertionError("Divestiture If Wanted plan row is absent")
    if (
        query_rti_work.source_location_text(divestiture_if_wanted)
        != (
            "cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp:136"
        )
        or divestiture_if_wanted.get("assertions") != 66
    ):
        raise AssertionError("Divestiture If Wanted source/evidence drifted")
    if divestiture_if_wanted.get("traceability_state") != "requirements-mapped":
        raise AssertionError("Divestiture If Wanted row is not mapped")
    divestiture_if_wanted_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-divestiture-if-wanted-pending",
        limit=0,
    )
    if (
        divestiture_if_wanted_focus.get("lane_state") != "complete"
        or divestiture_if_wanted_focus.get("mapped_test_count") != 1
        or divestiture_if_wanted_focus.get("assertion_count") != 66
        or divestiture_if_wanted_focus.get("requirement_count") != 10
        or divestiture_if_wanted_focus.get("standard_section_count") != 5
        or divestiture_if_wanted_focus.get("requirement_section_pair_count") != 10
    ):
        raise AssertionError("Divestiture If Wanted lane card drifted")
    divestiture_if_wanted_handles = divestiture_if_wanted_focus.get("lane_handles")
    if (
        not isinstance(divestiture_if_wanted_handles, dict)
        or divestiture_if_wanted_handles.get("catch2_target")
        != "umbra_attribute_ownership_divestiture_if_wanted_pending_catch2"
    ):
        raise AssertionError("Divestiture If Wanted target handle drifted")
    if divestiture_if_wanted_handles.get("ctest_filter") != (
        "^umbra\\.attribute_ownership_divestiture_if_wanted_pending\\.catch2\\.Embedded "
        "Attribute Ownership Divestiture If Wanted transfers only to 2025 pending acquirers$"
    ):
        raise AssertionError("Divestiture If Wanted CTest filter drifted")
    if "attribute-ownership-divestiture-if-wanted-pending" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("Divestiture If Wanted lane is not roadmap-indexed")
    ownership_query = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-query-integration"
        ),
        None,
    )
    if not isinstance(ownership_query, dict):
        raise AssertionError("attribute-ownership query plan row is absent")
    if (
        query_rti_work.source_location_text(ownership_query)
        != ("cpp/tests/ieee1516_2025_embedded_query_attribute_ownership_catch2.cpp:4")
        or ownership_query.get("assertions") != 49
    ):
        raise AssertionError("attribute-ownership query source/evidence drifted")
    if ownership_query.get("traceability_state") != "requirements-mapped":
        raise AssertionError("attribute-ownership query row is not mapped")
    ownership_query_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-query-results-removal",
        limit=0,
    )
    if (
        ownership_query_focus.get("lane_state") != "complete"
        or ownership_query_focus.get("mapped_test_count") != 1
        or ownership_query_focus.get("assertion_count") != 49
        or ownership_query_focus.get("requirement_count") != 10
        or ownership_query_focus.get("standard_section_count") != 2
        or ownership_query_focus.get("requirement_section_pair_count") != 10
    ):
        raise AssertionError("attribute-ownership query lane card drifted")
    ownership_query_handles = ownership_query_focus.get("lane_handles")
    if (
        not isinstance(ownership_query_handles, dict)
        or ownership_query_handles.get("catch2_target") != "umbra_ieee1516_2025_catch2"
    ):
        raise AssertionError("attribute-ownership query target handle drifted")
    if ownership_query_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded Query Attribute Ownership reports 2025 "
        "federate and unowned attributes$"
    ):
        raise AssertionError("attribute-ownership query CTest filter drifted")
    if "attribute-ownership-query-results-removal" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("attribute-ownership query lane is not roadmap-indexed")
    evoked_regional_snapshot = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-evoked-regional-interaction-source-region-snapshot-integration"
        ),
        None,
    )
    if not isinstance(evoked_regional_snapshot, dict):
        raise AssertionError(
            "evoked regional source-region snapshot plan row is absent"
        )
    if (
        query_rti_work.source_location_text(evoked_regional_snapshot)
        != (
            "cpp/tests/ieee1516_2025_evoked_regional_interaction_source_region_snapshot_catch2.cpp:3"
        )
        or evoked_regional_snapshot.get("assertions") != 42
    ):
        raise AssertionError(
            "evoked regional source-region snapshot source/evidence drifted"
        )
    if evoked_regional_snapshot.get("traceability_state") != "requirements-mapped":
        raise AssertionError("evoked regional source-region snapshot row is not mapped")
    evoked_regional_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "evoked-regional-interaction-source-region-snapshot",
        limit=0,
    )
    if (
        evoked_regional_focus.get("lane_state") != "complete"
        or evoked_regional_focus.get("mapped_test_count") != 1
        or evoked_regional_focus.get("assertion_count") != 42
        or evoked_regional_focus.get("requirement_count") != 9
        or evoked_regional_focus.get("standard_section_count") != 9
        or evoked_regional_focus.get("requirement_section_pair_count") != 9
    ):
        raise AssertionError("evoked regional source-region snapshot lane card drifted")
    evoked_regional_handles = evoked_regional_focus.get("lane_handles")
    if not isinstance(evoked_regional_handles, dict):
        raise AssertionError(
            "evoked regional source-region snapshot lane lost its handles"
        )
    if evoked_regional_handles.get("catch2_target") != (
        "umbra_ieee1516_2025_catch2"
    ) or evoked_regional_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded evoked regional interaction retains its send-time source region$"
    ):
        raise AssertionError(
            "evoked regional source-region snapshot CTest handles drifted"
        )
    register_default_region = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-register-object-instance-with-regions-default-region-generated-name-unit"
        ),
        None,
    )
    if not isinstance(register_default_region, dict):
        raise AssertionError("default-region generated-name plan row is absent")
    if query_rti_work.source_location_text(register_default_region) != (
        "cpp/tests/federation_registry_catch2.cpp:12264"
    ):
        raise AssertionError("default-region generated-name source pointer drifted")
    if register_default_region.get("assertions") != 48:
        raise AssertionError("default-region generated-name assertion count drifted")
    if register_default_region.get("traceability_state") != "requirements-mapped":
        raise AssertionError("default-region generated-name row is not mapped")
    register_default_region_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "register-object-instance-with-regions-default-region-generated-name",
        limit=0,
    )
    if (
        register_default_region_focus.get("lane_state") != "complete"
        or register_default_region_focus.get("mapped_test_count") != 2
        or register_default_region_focus.get("assertion_count") != 96
        or register_default_region_focus.get("requirement_count") != 4
        or register_default_region_focus.get("standard_section_count") != 2
        or register_default_region_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("default-region generated-name lane card drifted")
    register_default_region_handles = register_default_region_focus.get("lane_handles")
    if not isinstance(register_default_region_handles, dict):
        raise AssertionError("default-region generated-name lane lost its handles")
    if register_default_region_handles.get("catch2_target") != (
        "umbra_federation_registry_catch2"
    ) or register_default_region_handles.get("ctest_filter") != (
        "^umbra\\.federation_registry\\.catch2\\.Register Object Instance With Regions uses the default region and execution-wide generated names$"
    ):
        raise AssertionError("default-region generated-name CTest handles drifted")
    register_default_region_trace = query_rti_work.roadmap_links_for_test(
        index, register_default_region, limit=0
    )
    if (
        not register_default_region_trace
        or register_default_region_trace[0].get("id") != "object-ddm-ownership"
    ):
        raise AssertionError(
            "default-region generated-name trace lost its owning family"
        )
    register_default_region_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="object-ddm-ownership", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "object-ddm-ownership"
        ),
        None,
    )
    if not isinstance(
        register_default_region_family, dict
    ) or "register-object-instance-with-regions-default-region-generated-name" not in (
        register_default_region_family.get("query_tags") or []
    ):
        raise AssertionError(
            "object/DDM family lost the default-region generated-name focus alias"
        )
    public_register_default_region = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-register-object-instance-with-regions-default-region-generated-name-integration"
        ),
        None,
    )
    if not isinstance(public_register_default_region, dict):
        raise AssertionError(
            "public process default-region generated-name plan row is absent"
        )
    if query_rti_work.source_location_text(public_register_default_region) != (
        "cpp/tests/ieee1516_2025_connection_default_region_registration_catch2.cpp:459"
    ):
        raise AssertionError(
            "public process default-region generated-name source pointer drifted"
        )
    if public_register_default_region.get("assertions") != 48:
        raise AssertionError(
            "public process default-region generated-name assertion count drifted"
        )
    if (
        public_register_default_region.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "public process default-region generated-name row is not mapped"
        )
    public_register_default_region_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "public-process-register-object-instance-with-regions-default-region-generated-name",
        limit=0,
    )
    if (
        public_register_default_region_focus.get("lane_state") != "complete"
        or public_register_default_region_focus.get("mapped_test_count") != 1
        or public_register_default_region_focus.get("assertion_count") != 48
        or public_register_default_region_focus.get("requirement_count") != 4
        or public_register_default_region_focus.get("standard_section_count") != 2
        or public_register_default_region_focus.get("requirement_section_pair_count")
        != 4
    ):
        raise AssertionError(
            "public process default-region generated-name lane card drifted"
        )
    public_register_default_region_handles = public_register_default_region_focus.get(
        "lane_handles"
    )
    if not isinstance(public_register_default_region_handles, dict):
        raise AssertionError(
            "public process default-region generated-name lane lost its handles"
        )
    if public_register_default_region_handles.get("catch2_target") != (
        "umbra_ieee1516_2025_connection_catch2"
    ) or public_register_default_region_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador registers available-dimensional attributes on the default region and exposes generated names through a configured process endpoint$"
    ):
        raise AssertionError(
            "public process default-region generated-name CTest handles drifted"
        )
    deferred_state_image_restore = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-save-commit-filesystem-process-restart-deferred-update-region-association-integration"
        ),
        None,
    )
    if not isinstance(deferred_state_image_restore, dict):
        raise AssertionError("deferred state-image restore plan row is absent")
    if query_rti_work.source_location_text(deferred_state_image_restore) != (
        "cpp/tests/ieee1516_2025_federation_registry_deferred_update_region_association_filesystem_restart_catch2.cpp:3"
    ):
        raise AssertionError("deferred state-image restore source pointer drifted")
    if deferred_state_image_restore.get("assertions") != 79:
        raise AssertionError("deferred state-image restore assertion count drifted")
    if deferred_state_image_restore.get("traceability_state") != "requirements-mapped":
        raise AssertionError("deferred state-image restore row is not mapped")
    deferred_state_image_restore_focus = query_rti_work.focused_lane_result(
        index, tests, "deferred-update-region-association-state-image-restore", limit=0
    )
    if (
        deferred_state_image_restore_focus.get("lane_state") != "complete"
        or deferred_state_image_restore_focus.get("mapped_test_count") != 1
        or deferred_state_image_restore_focus.get("assertion_count") != 79
        or deferred_state_image_restore_focus.get("requirement_count") != 7
        or deferred_state_image_restore_focus.get("standard_section_count") != 5
        or deferred_state_image_restore_focus.get("requirement_section_pair_count") != 7
    ):
        raise AssertionError("deferred state-image restore lane card drifted")
    deferred_state_image_restore_handles = deferred_state_image_restore_focus.get(
        "lane_handles"
    )
    if not isinstance(deferred_state_image_restore_handles, dict):
        raise AssertionError("deferred state-image restore lane lost its handles")
    if deferred_state_image_restore_handles.get("catch2_target") != (
        "umbra_federation_registry_catch2"
    ) or deferred_state_image_restore_handles.get("ctest_filter") != (
        "^umbra\\.federation_registry\\.catch2\\.Filesystem fresh-registry restore promotes a deferred update-region association after ownership acquisition$"
    ):
        raise AssertionError("deferred state-image restore CTest handles drifted")
    deferred_state_image_restore_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_state_image_restore, limit=0
    )
    if (
        not deferred_state_image_restore_trace
        or deferred_state_image_restore_trace[0].get("id") != "time-save-restore"
    ):
        raise AssertionError(
            "deferred state-image restore trace lost its owning family"
        )
    deferred_state_image_restore_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="time-save-restore", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "time-save-restore"
        ),
        None,
    )
    if not isinstance(
        deferred_state_image_restore_family, dict
    ) or "deferred-update-region-association-state-image-restore" not in (
        deferred_state_image_restore_family.get("query_tags") or []
    ):
        raise AssertionError(
            "time/save/restore family lost the deferred state-image restore focus alias"
        )
    deferred_state_image = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-state-image-deferred-update-region-association-unit"
        ),
        None,
    )
    if not isinstance(deferred_state_image, dict):
        raise AssertionError("deferred state-image plan row is absent")
    if query_rti_work.source_location_text(deferred_state_image) != (
        "cpp/tests/federation_registry_catch2.cpp:11943"
    ):
        raise AssertionError("deferred state-image source pointer drifted")
    if deferred_state_image.get("assertions") != 9:
        raise AssertionError("deferred state-image assertion count drifted")
    if deferred_state_image.get("traceability_state") != "requirements-mapped":
        raise AssertionError("deferred state-image row is not mapped")
    deferred_state_image_focus = query_rti_work.focused_lane_result(
        index, tests, "deferred-update-region-association-state-image", limit=0
    )
    if (
        deferred_state_image_focus.get("lane_state") != "complete"
        or deferred_state_image_focus.get("mapped_test_count") != 1
        or deferred_state_image_focus.get("assertion_count") != 9
        or deferred_state_image_focus.get("requirement_count") != 2
        or deferred_state_image_focus.get("standard_section_count") != 2
        or deferred_state_image_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("deferred state-image lane card drifted")
    deferred_state_image_handles = deferred_state_image_focus.get("lane_handles")
    if not isinstance(deferred_state_image_handles, dict):
        raise AssertionError("deferred state-image lane lost its handles")
    if deferred_state_image_handles.get("catch2_target") != (
        "umbra_federation_registry_catch2"
    ) or deferred_state_image_handles.get("ctest_filter") != (
        "^umbra\\.federation_registry\\.catch2\\.Federation state images preserve deferred update-region associations across a v1 codec round trip$"
    ):
        raise AssertionError("deferred state-image CTest handles drifted")
    deferred_state_image_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_state_image, limit=0
    )
    if (
        not deferred_state_image_trace
        or deferred_state_image_trace[0].get("id") != "time-save-restore"
    ):
        raise AssertionError("deferred state-image trace lost its owning family")
    deferred_state_image_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="time-save-restore", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "time-save-restore"
        ),
        None,
    )
    if not isinstance(
        deferred_state_image_family, dict
    ) or "deferred-update-region-association-state-image" not in (
        deferred_state_image_family.get("query_tags") or []
    ):
        raise AssertionError(
            "time/save/restore family lost the deferred state-image focus alias"
        )
    deferred_cancel = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-ownership-transfer-update-region-deferred-cancel-integration"
        ),
        None,
    )
    if not isinstance(deferred_cancel, dict):
        raise AssertionError("cancelled deferred ownership-transfer plan row is absent")
    if query_rti_work.source_location_text(deferred_cancel) != (
        "cpp/tests/ownership_transfer_update_region_catch2.cpp:620"
    ):
        raise AssertionError(
            "cancelled deferred ownership-transfer source pointer drifted"
        )
    if deferred_cancel.get("assertions") != 66:
        raise AssertionError(
            "cancelled deferred ownership-transfer assertion count drifted"
        )
    if deferred_cancel.get("traceability_state") != "requirements-mapped":
        raise AssertionError("cancelled deferred ownership-transfer row is not mapped")
    deferred_cancel_focus = query_rti_work.focused_lane_result(
        index, tests, "ownership-transfer-update-region-deferred-cancel", limit=0
    )
    if (
        deferred_cancel_focus.get("lane_state") != "complete"
        or deferred_cancel_focus.get("mapped_test_count") != 1
        or deferred_cancel_focus.get("assertion_count") != 66
        or deferred_cancel_focus.get("requirement_count") != 1
        or deferred_cancel_focus.get("standard_section_count") != 2
        or deferred_cancel_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("cancelled deferred ownership-transfer lane card drifted")
    deferred_cancel_handles = deferred_cancel_focus.get("lane_handles")
    if not isinstance(deferred_cancel_handles, dict):
        raise AssertionError(
            "cancelled deferred ownership-transfer lane lost its handles"
        )
    if deferred_cancel_handles.get("catch2_target") != (
        "umbra_ownership_transfer_update_region_catch2"
    ) or deferred_cancel_handles.get("ctest_filter") != (
        "^umbra\\.ownership_transfer_update_region\\.catch2\\.Embedded ownership transfer drops a non-owner's cancelled 2025 update-region association$"
    ):
        raise AssertionError(
            "cancelled deferred ownership-transfer CTest handles drifted"
        )
    deferred_cancel_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_cancel, limit=0
    )
    if (
        not deferred_cancel_trace
        or deferred_cancel_trace[0].get("id") != "object-ddm-ownership"
    ):
        raise AssertionError(
            "cancelled deferred ownership-transfer trace lost its owning family"
        )
    deferred_transfer = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-ownership-transfer-update-region-deferred-integration"
        ),
        None,
    )
    if not isinstance(deferred_transfer, dict):
        raise AssertionError("deferred ownership-transfer plan row is absent")
    if query_rti_work.source_location_text(deferred_transfer) != (
        "cpp/tests/ownership_transfer_update_region_catch2.cpp:478"
    ):
        raise AssertionError("deferred ownership-transfer source pointer drifted")
    if deferred_transfer.get("assertions") != 51:
        raise AssertionError("deferred ownership-transfer assertion count drifted")
    if deferred_transfer.get("traceability_state") != "requirements-mapped":
        raise AssertionError("deferred ownership-transfer row is not mapped")
    deferred_transfer_focus = query_rti_work.focused_lane_result(
        index, tests, "ownership-transfer-update-region-deferred", limit=0
    )
    if (
        deferred_transfer_focus.get("lane_state") != "complete"
        or deferred_transfer_focus.get("mapped_test_count") != 1
        or deferred_transfer_focus.get("assertion_count") != 51
        or deferred_transfer_focus.get("requirement_count") != 2
        or deferred_transfer_focus.get("standard_section_count") != 1
        or deferred_transfer_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("deferred ownership-transfer lane card drifted")
    deferred_handles = deferred_transfer_focus.get("lane_handles")
    if not isinstance(deferred_handles, dict):
        raise AssertionError("deferred ownership-transfer lane lost its handles")
    if deferred_handles.get("catch2_target") != (
        "umbra_ownership_transfer_update_region_catch2"
    ) or deferred_handles.get("ctest_filter") != (
        "^umbra\\.ownership_transfer_update_region\\.catch2\\.Embedded ownership transfer activates a non-owner's deferred 2025 update-region association$"
    ):
        raise AssertionError("deferred ownership-transfer CTest handles drifted")
    deferred_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_transfer, limit=0
    )
    if not deferred_trace or deferred_trace[0].get("id") != "object-ddm-ownership":
        raise AssertionError("deferred ownership-transfer trace lost its owning family")
    deferred_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="object-ddm-ownership", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "object-ddm-ownership"
        ),
        None,
    )
    if not isinstance(
        deferred_family, dict
    ) or "ownership-transfer-update-region-deferred" not in (
        deferred_family.get("query_tags") or []
    ):
        raise AssertionError(
            "object-DDM family lost the deferred ownership focus alias"
        )
    reacquisition_focus = query_rti_work.focused_lane_result(
        index, tests, "ownership-transfer-update-region-reacquisition", limit=0
    )
    if (
        reacquisition_focus.get("lane_state") != "complete"
        or reacquisition_focus.get("mapped_test_count") != 1
        or reacquisition_focus.get("assertion_count") != 106
        or reacquisition_focus.get("requirement_count") != 7
        or reacquisition_focus.get("standard_section_count") != 4
        or reacquisition_focus.get("requirement_section_pair_count") != 7
    ):
        raise AssertionError("ownership-transfer reacquisition lane card drifted")
    reacquisition_handles = reacquisition_focus.get("lane_handles")
    if not isinstance(reacquisition_handles, dict):
        raise AssertionError("ownership-transfer reacquisition lane lost its handles")
    if reacquisition_handles.get("catch2_target") != (
        "umbra_ownership_transfer_update_region_catch2"
    ) or reacquisition_handles.get("ctest_filter") != (
        "^umbra\\.ownership_transfer_update_region\\.catch2\\.Embedded ownership transfer clears the former owner's 2025 update-region association$"
    ):
        raise AssertionError("ownership-transfer reacquisition CTest handles drifted")
    reacquisition_links = query_rti_work.roadmap_links_for_test(
        index, ownership_transfer, limit=0
    )
    if (
        not reacquisition_links
        or reacquisition_links[0].get("id") != "object-ddm-ownership"
    ):
        raise AssertionError(
            "ownership-transfer reacquisition trace lost its owning family"
        )
    if "umbra_ownership_transfer_update_region_catch2" not in declared_executables:
        raise AssertionError("deferred ownership-transfer focused target disappeared")
