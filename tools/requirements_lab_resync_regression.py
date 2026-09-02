"""Offline regression checks for Requirements-Lab resynchronization.

The exporter treats opaque record IDs and source locations as regenerable
identity, but ordinal numbering is still important audit evidence.  This
guard keeps an ordinal-only move visible even when the normative text and
record ID are unchanged.  It uses temporary bundles and never rewrites the
checked-in baseline.
"""

from __future__ import annotations

import copy
import json
import tempfile
from pathlib import Path


SCRIPT_ROOT = Path(__file__).resolve().parent
import sys

sys.path.insert(0, str(SCRIPT_ROOT))

import requirements_lab  # noqa: E402  (repository tool import after path setup)


DOCUMENT_ID = "hla-1516.1-2025"
COLLECTIONS = (
    "requirements",
    "transitions",
    "mappings",
    "api_surfaces",
    "api_crosswalks",
    "requirement_api_bindings",
)


def _bundle_from_records(records: list[dict[str, object]]) -> dict[str, object]:
    document: dict[str, object] = {"document_id": DOCUMENT_ID}
    document.update({collection: [] for collection in COLLECTIONS})
    document["requirements"] = records
    return {"bundle_id": "resync-regression", "documents": [document]}


def _bundle(record: dict[str, object]) -> dict[str, object]:
    return _bundle_from_records([record])


def _write_bundle(directory: Path, name: str, value: dict[str, object]) -> Path:
    path = directory / name
    path.write_text(json.dumps(value), encoding="utf-8")
    return path


def main() -> int:
    baseline_record: dict[str, object] = {
        "id": "requirement.example",
        "ordinal": 1,
        "clause": "10.1",
        "statement": "The example requirement remains semantically stable.",
        "source": {"path": "example.tex", "line": 10, "end_line": 12},
    }

    with tempfile.TemporaryDirectory(prefix="umbra-resync-regression-") as directory:
        root = Path(directory)
        baseline_path = _write_bundle(root, "baseline.json", _bundle(baseline_record))

        ordinal_only = copy.deepcopy(baseline_record)
        ordinal_only["ordinal"] = 3
        report = requirements_lab.resync_bundles(
            baseline_path,
            _write_bundle(root, "ordinal-only.json", _bundle(ordinal_only)),
            edition="2025",
        )
        document = report["documents"][0]
        delta = document["collections"]["requirements"]
        if report["changed_documents"] != 1 or document["status"] != "content-or-id-drift":
            raise AssertionError(f"ordinal-only move did not mark the document as changed: {report}")
        if not requirements_lab.resync_has_numbering_drift(report):
            raise AssertionError("ordinal-only move did not trip the numbering-drift predicate")
        if delta["missing_content"] or delta["added_content"] or delta["renumbered"]:
            raise AssertionError(f"ordinal-only move was misclassified as content/ID drift: {delta}")
        expected_ordinal = {
            "baseline_id": "requirement.example",
            "candidate_id": "requirement.example",
            "baseline_ordinal": 1,
            "candidate_ordinal": 3,
        }
        observed_ordinal = delta["ordinal_drift"]
        if len(observed_ordinal) != 1 or any(
            observed_ordinal[0].get(key) != value for key, value in expected_ordinal.items()
        ):
            raise AssertionError(f"ordinal-only move was not reported precisely: {observed_ordinal}")

        changed_text = copy.deepcopy(baseline_record)
        changed_text["ordinal"] = 4
        changed_text["statement"] = "The example requirement has revised wording."
        report = requirements_lab.resync_bundles(
            baseline_path,
            _write_bundle(root, "text-and-ordinal.json", _bundle(changed_text)),
            edition="2025",
        )
        delta = report["documents"][0]["collections"]["requirements"]
        if not delta["missing_content"] or not delta["added_content"]:
            raise AssertionError(f"text change was not retained as semantic drift: {delta}")
        if len(delta["ordinal_drift"]) != 1:
            raise AssertionError(
                "an ordinal move on a retained ID disappeared alongside text drift: "
                + str(delta)
            )

        id_and_ordinal = copy.deepcopy(baseline_record)
        id_and_ordinal["id"] = "requirement.example.regenerated"
        id_and_ordinal["ordinal"] = 5
        report = requirements_lab.resync_bundles(
            baseline_path,
            _write_bundle(root, "id-and-ordinal.json", _bundle(id_and_ordinal)),
            edition="2025",
        )
        delta = report["documents"][0]["collections"]["requirements"]
        if len(delta["renumbered"]) != 1 or len(delta["ordinal_drift"]) != 1:
            raise AssertionError(f"combined ID/ordinal drift was not separated: {delta}")
        if not requirements_lab.resync_has_numbering_drift(report):
            raise AssertionError("combined ID/ordinal move did not trip the numbering-drift predicate")

        clean_report = requirements_lab.resync_bundles(
            baseline_path,
            _write_bundle(root, "unchanged.json", _bundle(copy.deepcopy(baseline_record))),
            edition="2025",
        )
        if requirements_lab.resync_has_numbering_drift(clean_report):
            raise AssertionError("unchanged bundle incorrectly tripped the numbering-drift predicate")

        valid_pair = copy.deepcopy(baseline_record)
        valid_pair["id"] = "requirement.second"
        valid_pair["ordinal"] = 2
        malformed = _bundle_from_records(
            [
                copy.deepcopy(baseline_record),
                valid_pair,
            ]
        )
        malformed["documents"][0]["requirements"][1]["ordinal"] = 1
        integrity_findings = requirements_lab.check_bundle_ordinal_integrity(
            malformed,
            edition="2025",
        )
        if not any("duplicate requirement ordinal 1" in finding for finding in integrity_findings):
            raise AssertionError(
                "duplicate requirement ordinals were not rejected: "
                + "; ".join(integrity_findings)
            )

        malformed["documents"][0]["requirements"][1]["ordinal"] = "2"
        integrity_findings = requirements_lab.check_bundle_ordinal_integrity(
            malformed,
            edition="2025",
        )
        if not any("invalid ordinal" in finding for finding in integrity_findings):
            raise AssertionError(
                "non-integer requirement ordinals were not rejected: "
                + "; ".join(integrity_findings)
            )

        malformed["documents"][0]["requirements"][1].pop("ordinal")
        integrity_findings = requirements_lab.check_bundle_ordinal_integrity(
            malformed,
            edition="2025",
        )
        if not any("missing ordinal" in finding for finding in integrity_findings):
            raise AssertionError(
                "missing requirement ordinals were not rejected: "
                + "; ".join(integrity_findings)
            )

    print("requirements-lab: resync ordinal-numbering regression guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
