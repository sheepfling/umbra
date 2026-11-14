"""Focused coverage for indexed portable C++ TCK handoffs."""

from __future__ import annotations

from tools import query_rti_work


def test_active_tck_handoff_keeps_umbrella_requirement_section_pairs(
    monkeypatch,
) -> None:
    scenario_id = "cpp-tck.resign-cancel-pending-ownership-acquisitions-action"
    scenarios = {
        scenario_id: {
            "id": scenario_id,
            "title": "Cancel pending ownership acquisition during resignation",
            "cpp_test": "src/portable_tck.cpp#scenarioResignCancelPendingOwnershipAcquisitionsPortable",
            "contract_refs": [],
            "requirement_mappings": [
                {
                    "requirement_id": "cpp-1516.1-object-management",
                    "standard_section": "hla-1516.1-2025:clause-6.8",
                },
                {
                    "requirement_id": "cpp-1516.1-object-management",
                    "standard_section": "hla-1516.1-2025:clause-6.9",
                },
            ],
        }
    }
    monkeypatch.setattr(
        query_rti_work,
        "load_cpp_tck_scenarios",
        lambda: scenarios,
    )

    result = query_rti_work.indexed_active_handoff_record(
        {
            "active_handoff": {
                "id": f"{scenario_id}-2026-11-12",
                "plan_id": scenario_id,
                "mapping_seed_plan_id": scenario_id,
                "family": "portable-cpp-tck-2025",
                "lane": "resignation-actions",
                "state": "ready",
                "test_case": scenarios[scenario_id]["title"],
            }
        },
        [],
    )

    assert result is not None
    assert result["found"] is True
    assert result["requirement_section_pair_count"] == 2
    assert [
        (row["lab_requirement_id"], row["standard_section"])
        for row in result["requirement_section_pairs"]
    ] == [
        (
            "cpp-1516.1-object-management",
            "hla-1516.1-2025:clause-6.8",
        ),
        (
            "cpp-1516.1-object-management",
            "hla-1516.1-2025:clause-6.9",
        ),
    ]

    catalog_evidence = query_rti_work.cpp_tck_requirement_mapping_evidence(
        {
            "kind": "portable-cpp-tck-catalog",
            "scenarios": [scenarios[scenario_id]],
        },
        [],
        scenario_ids=[scenario_id],
    )
    assert catalog_evidence["scenario_count"] == 1
    assert catalog_evidence["mapping_count"] == 2
    assert catalog_evidence["scenarios"][0]["requirement_mappings"] == [
        {
            "requirement_id": "cpp-1516.1-object-management",
            "standard_section": "hla-1516.1-2025:clause-6.8",
        },
        {
            "requirement_id": "cpp-1516.1-object-management",
            "standard_section": "hla-1516.1-2025:clause-6.9",
        },
    ]
