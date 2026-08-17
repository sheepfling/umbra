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
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_OUTPUT = REPOSITORY_ROOT / ".compliance" / "cpp-conformance-worklist.json"
DEFAULT_DOCUMENT = "hla-1516.1-2025"


def _object(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON from {path}: {error}") from error
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return value


def _document(bundle: dict[str, Any], document_id: str) -> dict[str, Any]:
    documents = bundle.get("documents")
    if not isinstance(documents, list):
        raise ValueError("bundle has no documents array")
    matches = [item for item in documents if isinstance(item, dict) and item.get("document_id") == document_id]
    if len(matches) != 1:
        raise ValueError(f"bundle must contain exactly one {document_id!r} document")
    return matches[0]


def _action(crosswalk_status: str) -> str:
    if crosswalk_status == "matched":
        return "Implement the standard symbol, then create a real Catch2 catalog entry."
    if crosswalk_status == "ambiguous":
        return "Resolve the exact C++ API surface in the Requirements Lab before cataloging a test."
    if crosswalk_status == "unmatched":
        return "Add or review the C++ crosswalk in the Requirements Lab before cataloging a test."
    return "Wait for a C++ crosswalk before cataloging a test."


def build_worklist(bundle: dict[str, Any], document_id: str) -> dict[str, Any]:
    document = _document(bundle, document_id)
    mappings = document.get("mappings")
    crosswalks = document.get("api_crosswalks")
    if not isinstance(mappings, list) or not isinstance(crosswalks, list):
        raise ValueError(f"{document_id} has no mappings or API crosswalks array")
    mapping_by_id = {
        item.get("id"): item
        for item in mappings
        if isinstance(item, dict) and isinstance(item.get("id"), str)
    }
    work_items: list[dict[str, object]] = []
    for crosswalk in crosswalks:
        if not isinstance(crosswalk, dict) or crosswalk.get("target") != "cpp":
            continue
        mapping_id = crosswalk.get("implementation_mapping_id")
        status = crosswalk.get("status")
        mapping = mapping_by_id.get(mapping_id)
        if not isinstance(mapping_id, str) or not isinstance(status, str) or mapping is None:
            raise ValueError("bundle contains an invalid C++ crosswalk")
        work_items.append(
            {
                "id": f"cpp:{mapping_id}",
                "implementation_mapping_id": mapping_id,
                "interface_kind": mapping.get("interface_kind"),
                "interface_name": mapping.get("interface_name"),
                "requirement_ids": mapping.get("requirement_ids", []),
                "transition_ids": mapping.get("transition_ids", []),
                "crosswalk_status": status,
                "candidate_api_surface_ids": crosswalk.get("candidate_api_surface_ids", []),
                "selected_api_surface_ids": crosswalk.get("selected_api_surface_ids", []),
                "implementation_status": "planned",
                "next_action": _action(status),
            }
        )
    work_items.sort(key=lambda item: str(item["implementation_mapping_id"]))
    summary = Counter(str(item["crosswalk_status"]) for item in work_items)
    return {
        "schema_version": 1,
        "kind": "requirements-lab-cpp-conformance-worklist",
        "document_id": document_id,
        "corpus_revision": bundle.get("corpus_revision"),
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
        worklist = build_worklist(_object(args.bundle.resolve()), args.document_id)
        output = args.output.resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(worklist, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        summary = worklist["summary"]
        print(
            "wrote C++ conformance worklist: "
            f"{summary['total_cpp_bindings']} bindings, "
            f"statuses={summary['crosswalk_status_counts']}"
        )
        return 0
    except (OSError, ValueError) as error:
        print(f"cpp-conformance-worklist: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
