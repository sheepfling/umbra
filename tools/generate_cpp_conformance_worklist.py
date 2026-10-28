"""Generate a non-evidentiary C++ conformance worklist from a Lab bundle.

The output is deliberately not a ComplianceTestCatalog. It lists every C++
crosswalk in the selected IEEE 1516.1-2025 document, including ambiguous and
unmatched work, so implementation slices begin from the corpus instead of a
handwritten approximation.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Literal, TypedDict, cast

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_OUTPUT = REPOSITORY_ROOT / ".compliance" / "cpp-conformance-worklist.json"
DEFAULT_DOCUMENT = "hla-1516.1-2025"


CrosswalkStatus = Literal["matched", "ambiguous", "unmatched"]


class _RawBundle(TypedDict, total=False):
    documents: object
    corpus_revision: object


class _RawDocument(TypedDict, total=False):
    document_id: object
    mappings: object
    api_crosswalks: object


class _RawMapping(TypedDict, total=False):
    id: object
    interface_kind: object
    interface_name: object
    requirement_ids: object
    transition_ids: object


class _RawCrosswalk(TypedDict, total=False):
    target: object
    implementation_mapping_id: object
    status: object
    candidate_api_surface_ids: object
    selected_api_surface_ids: object


class _MappingRecord(TypedDict):
    id: str
    interface_kind: str
    interface_name: str
    requirement_ids: list[str]
    transition_ids: list[str]


class _CppCrosswalk(TypedDict):
    implementation_mapping_id: str
    status: CrosswalkStatus
    candidate_api_surface_ids: list[str]
    selected_api_surface_ids: list[str]


class WorkItem(TypedDict):
    id: str
    implementation_mapping_id: str
    interface_kind: str
    interface_name: str
    requirement_ids: list[str]
    transition_ids: list[str]
    crosswalk_status: CrosswalkStatus
    candidate_api_surface_ids: list[str]
    selected_api_surface_ids: list[str]
    implementation_status: Literal["planned"]
    next_action: str


class WorklistSummary(TypedDict):
    total_cpp_bindings: int
    crosswalk_status_counts: dict[CrosswalkStatus, int]


class CppConformanceWorklist(TypedDict):
    schema_version: Literal[1]
    kind: Literal["requirements-lab-cpp-conformance-worklist"]
    document_id: str
    corpus_revision: str | None
    notes: str
    summary: WorklistSummary
    work_items: list[WorkItem]


def _ensure_object(value: object, *, context: str) -> None:
    if not isinstance(value, dict):
        raise TypeError(f"{context} must be a JSON object")
    for key in value:
        if not isinstance(key, str):
            raise TypeError(f"{context} has a non-string key")


def _required_string(value: object, *, context: str) -> str:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{context} must be a non-empty string")
    return value


def _string_list(value: object, *, context: str) -> list[str]:
    if not isinstance(value, list):
        raise TypeError(f"{context} must be an array of strings")
    result: list[str] = []
    for index, item in enumerate(value):
        if not isinstance(item, str) or not item:
            raise TypeError(f"{context}[{index}] must be a non-empty string")
        result.append(item)
    return result


def _object_list(value: object, *, context: str) -> list[object]:
    if not isinstance(value, list):
        raise TypeError(f"{context} must be an array")
    return list(value)


def _read_object(path: Path) -> _RawBundle:
    try:
        value: object = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON from {path}: {error}") from error
    _ensure_object(value, context=str(path))
    return cast(_RawBundle, value)


def _document(bundle: _RawBundle, document_id: str) -> _RawDocument:
    documents = _object_list(bundle.get("documents"), context="bundle.documents")
    matches: list[_RawDocument] = []
    for index, value in enumerate(documents):
        context = f"bundle.documents[{index}]"
        _ensure_object(value, context=context)
        item = cast(_RawDocument, value)
        actual_document_id = _required_string(
            item.get("document_id"), context=f"{context}.document_id"
        )
        if actual_document_id == document_id:
            matches.append(item)
    if len(matches) != 1:
        raise ValueError(f"bundle must contain exactly one {document_id!r} document")
    return matches[0]


def _mapping_record(value: object, *, context: str) -> _MappingRecord:
    _ensure_object(value, context=context)
    record = cast(_RawMapping, value)
    return {
        "id": _required_string(record.get("id"), context=f"{context}.id"),
        "interface_kind": _required_string(
            record.get("interface_kind"), context=f"{context}.interface_kind"
        ),
        "interface_name": _required_string(
            record.get("interface_name"), context=f"{context}.interface_name"
        ),
        "requirement_ids": _string_list(
            record.get("requirement_ids"), context=f"{context}.requirement_ids"
        ),
        "transition_ids": _string_list(
            record.get("transition_ids"), context=f"{context}.transition_ids"
        ),
    }


def _cpp_crosswalk(record: _RawCrosswalk, *, context: str) -> _CppCrosswalk:
    raw_status = _required_string(record.get("status"), context=f"{context}.status")
    if raw_status == "matched":
        status: CrosswalkStatus = "matched"
    elif raw_status == "ambiguous":
        status = "ambiguous"
    elif raw_status == "unmatched":
        status = "unmatched"
    else:
        raise ValueError(
            f"{context}.status must be 'matched', 'ambiguous', or 'unmatched'"
        )
    return {
        "implementation_mapping_id": _required_string(
            record.get("implementation_mapping_id"),
            context=f"{context}.implementation_mapping_id",
        ),
        "status": status,
        "candidate_api_surface_ids": _string_list(
            record.get("candidate_api_surface_ids"),
            context=f"{context}.candidate_api_surface_ids",
        ),
        "selected_api_surface_ids": _string_list(
            record.get("selected_api_surface_ids"),
            context=f"{context}.selected_api_surface_ids",
        ),
    }


def _action(crosswalk_status: CrosswalkStatus) -> str:
    if crosswalk_status == "matched":
        return "Implement the standard symbol, then create a real Catch2 catalog entry."
    if crosswalk_status == "ambiguous":
        return "Resolve the exact C++ API surface in the Requirements Lab before cataloging a test."
    if crosswalk_status == "unmatched":
        return "Add or review the C++ crosswalk in the Requirements Lab before cataloging a test."
    raise AssertionError(f"unhandled C++ crosswalk status: {crosswalk_status}")


def build_worklist(bundle: object, document_id: str) -> CppConformanceWorklist:
    _ensure_object(bundle, context="bundle")
    bundle_record = cast(_RawBundle, bundle)
    document = _document(bundle_record, document_id)
    mapping_values = _object_list(
        document.get("mappings"), context=f"{document_id}.mappings"
    )
    crosswalk_values = _object_list(
        document.get("api_crosswalks"), context=f"{document_id}.api_crosswalks"
    )

    mapping_by_id: dict[str, _MappingRecord] = {}
    for index, value in enumerate(mapping_values):
        context = f"{document_id}.mappings[{index}]"
        mapping = _mapping_record(value, context=context)
        if mapping["id"] in mapping_by_id:
            raise ValueError(f"{context}.id duplicates mapping {mapping['id']!r}")
        mapping_by_id[mapping["id"]] = mapping

    work_items: list[WorkItem] = []
    seen_cpp_mapping_ids: set[str] = set()
    for index, value in enumerate(crosswalk_values):
        context = f"{document_id}.api_crosswalks[{index}]"
        _ensure_object(value, context=context)
        crosswalk_record = cast(_RawCrosswalk, value)
        target = _required_string(
            crosswalk_record.get("target"), context=f"{context}.target"
        )
        if target != "cpp":
            continue
        crosswalk = _cpp_crosswalk(crosswalk_record, context=context)
        mapping_id = crosswalk["implementation_mapping_id"]
        if mapping_id in seen_cpp_mapping_ids:
            raise ValueError(
                f"{context} duplicates the C++ crosswalk for {mapping_id!r}"
            )
        seen_cpp_mapping_ids.add(mapping_id)
        mapping = mapping_by_id.get(mapping_id)
        if mapping is None:
            raise ValueError(
                f"{context}.implementation_mapping_id refers to missing mapping {mapping_id!r}"
            )
        work_items.append(
            {
                "id": f"cpp:{mapping_id}",
                "implementation_mapping_id": mapping_id,
                "interface_kind": mapping["interface_kind"],
                "interface_name": mapping["interface_name"],
                "requirement_ids": mapping["requirement_ids"],
                "transition_ids": mapping["transition_ids"],
                "crosswalk_status": crosswalk["status"],
                "candidate_api_surface_ids": crosswalk["candidate_api_surface_ids"],
                "selected_api_surface_ids": crosswalk["selected_api_surface_ids"],
                "implementation_status": "planned",
                "next_action": _action(crosswalk["status"]),
            }
        )
    work_items.sort(key=lambda item: item["implementation_mapping_id"])
    summary: Counter[CrosswalkStatus] = Counter(
        item["crosswalk_status"] for item in work_items
    )
    corpus_revision = bundle_record.get("corpus_revision")
    if corpus_revision is not None and not isinstance(corpus_revision, str):
        raise ValueError("bundle.corpus_revision must be a string when present")
    return {
        "schema_version": 1,
        "kind": "requirements-lab-cpp-conformance-worklist",
        "document_id": document_id,
        "corpus_revision": corpus_revision,
        "notes": (
            "Generated planning worklist only. It is not a ComplianceTestCatalog, "
            "implementation manifest, or evidence."
        ),
        "summary": {
            "total_cpp_bindings": len(work_items),
            "crosswalk_status_counts": dict(sorted(summary.items())),
        },
        "work_items": work_items,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--document-id", default=DEFAULT_DOCUMENT)
    args = parser.parse_args()
    try:
        worklist = build_worklist(_read_object(args.bundle.resolve()), args.document_id)
        output = args.output.resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(
            json.dumps(worklist, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )
        summary = worklist["summary"]
        print(
            "wrote C++ conformance worklist: "
            f"{summary['total_cpp_bindings']} bindings, "
            f"statuses={summary['crosswalk_status_counts']}"
        )
        return 0
    except (OSError, TypeError, ValueError) as error:
        print(f"cpp-conformance-worklist: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
