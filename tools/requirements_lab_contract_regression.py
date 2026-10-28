"""Offline regression checks for canonical standard-section overrides."""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Literal, TypedDict, cast

SCRIPT_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_ROOT))

import requirements_lab


class _RequirementReference(TypedDict):
    id: str
    requirements_lab_requirement_id: str
    clause_id: str
    source: str
    source_symbol: str
    tests: list[str]


class _CanonicalOverride(TypedDict):
    requirements_lab_requirement_id: str
    document_id: str
    exported_clause_id: str
    canonical_clause_id: str
    source_path: str
    known_issue_id: str


class _SourceHeadingOverride(_CanonicalOverride):
    source_heading: str


class _ReferenceContract(TypedDict):
    document_id: str
    kind: Literal["requirements-reference"]
    requirements: list[_RequirementReference]
    canonical_standard_section_overrides: list[
        _CanonicalOverride | _SourceHeadingOverride
    ]


class _LabRequirement(TypedDict):
    id: str
    clause_id: str


class _LabDocument(TypedDict):
    document_id: str
    requirements: list[_LabRequirement]


def _check_reference_contract(
    contract: _ReferenceContract, document: _LabDocument
) -> tuple[str, ...]:
    return requirements_lab._check_requirements_reference_contract(
        cast(requirements_lab.JsonObject, contract),
        cast(requirements_lab.JsonObject, document),
    )


def _with_source_heading(
    override: _CanonicalOverride | _SourceHeadingOverride, source_heading: str
) -> _SourceHeadingOverride:
    return {
        "requirements_lab_requirement_id": override["requirements_lab_requirement_id"],
        "document_id": override["document_id"],
        "exported_clause_id": override["exported_clause_id"],
        "canonical_clause_id": override["canonical_clause_id"],
        "source_path": override["source_path"],
        "known_issue_id": override["known_issue_id"],
        "source_heading": source_heading,
    }


def _contract(exported_clause: str, canonical_clause: str) -> _ReferenceContract:
    return {
        "document_id": "synthetic-2025",
        "kind": "requirements-reference",
        "requirements": [
            {
                "id": "umbra-canonical-section-override",
                "requirements_lab_requirement_id": "lab-requirement",
                "clause_id": canonical_clause,
                "source": "tools/requirements_lab.py",
                "source_symbol": "check_contract",
                "tests": ["tools/requirements_lab.py::check_contract"],
            }
        ],
        "canonical_standard_section_overrides": [
            {
                "requirements_lab_requirement_id": "lab-requirement",
                "document_id": "synthetic-2025",
                "exported_clause_id": exported_clause,
                "canonical_clause_id": canonical_clause,
                "source_path": "synthetic-clause.tex",
                "known_issue_id": "RL-999",
            }
        ],
    }


def _document(exported_clause: str) -> _LabDocument:
    return {
        "document_id": "synthetic-2025",
        "requirements": [{"id": "lab-requirement", "clause_id": exported_clause}],
    }


def main() -> int:
    accepted = _check_reference_contract(
        _contract("clause-6.27", "clause-6.26"),
        _document("clause-6.27"),
    )
    if accepted:
        raise AssertionError(
            "a source-backed canonical section override should reconcile the Lab export: "
            + "; ".join(accepted)
        )

    stale_export = _check_reference_contract(
        _contract("clause-6.26", "clause-6.26"),
        _document("clause-6.27"),
    )
    if not any(
        "canonical standard section override export drifted" in item
        for item in stale_export
    ):
        raise AssertionError(
            "a stale exported-clause override must be rejected: "
            + "; ".join(stale_export)
        )

    wrong_target_contract = _contract("clause-6.27", "clause-6.25")
    wrong_target_contract["canonical_standard_section_overrides"][0][
        "canonical_clause_id"
    ] = "clause-6.26"
    wrong_target = _check_reference_contract(
        wrong_target_contract,
        _document("clause-6.27"),
    )
    if not any(
        "canonical standard section override target drifted" in item
        for item in wrong_target
    ):
        raise AssertionError(
            "an override that disagrees with the contract's canonical clause must be rejected: "
            + "; ".join(wrong_target)
        )

    shared_crosswalk = _contract("clause-6.27", "clause-6.26")
    shared_crosswalk["requirements"][0]["requirements_lab_requirement_id"] = (
        "other-lab-requirement"
    )
    shared_crosswalk["requirements"][0]["clause_id"] = "clause-6.27"
    shared_crosswalk["canonical_standard_section_overrides"][0] = _with_source_heading(
        shared_crosswalk["canonical_standard_section_overrides"][0],
        "6.26 Synthetic source heading",
    )
    shared_document: _LabDocument = {
        "document_id": "synthetic-2025",
        "requirements": [
            {"id": "lab-requirement", "clause_id": "clause-6.27"},
            {"id": "other-lab-requirement", "clause_id": "clause-6.27"},
        ],
    }
    shared_crosswalk_result = _check_reference_contract(
        shared_crosswalk,
        shared_document,
    )
    if shared_crosswalk_result:
        raise AssertionError(
            "a source-heading-verified corpus crosswalk must not require local contract ownership: "
            + "; ".join(shared_crosswalk_result)
        )

    shared_crosswalk["canonical_standard_section_overrides"][0][
        "canonical_clause_id"
    ] = "clause-6.25"
    shared_wrong_target = _check_reference_contract(
        shared_crosswalk,
        shared_document,
    )
    if not any(
        "canonical standard section override target drifted" in item
        for item in shared_wrong_target
    ):
        raise AssertionError(
            "a shared crosswalk that disagrees with its source heading must be rejected: "
            + "; ".join(shared_wrong_target)
        )

    print("Canonical standard-section override regression checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
