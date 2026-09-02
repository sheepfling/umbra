"""Export and check Umbra's portable HLA Requirements Lab baselines.

The Requirements Lab owns semantic HLA data and verification. This helper
keeps the repository boundary explicit: it invokes the Lab's public export
module, then validates an Umbra-owned API baseline or implementation contract
against the exported JSON without importing any Lab package into library code.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_CONTRACT = REPOSITORY_ROOT / "compliance" / "requirements-lab" / "api-baseline.json"
DEFAULT_CATCH2_PLAN = (
    REPOSITORY_ROOT / "compliance" / "requirements-lab" / "catch2-test-plan.json"
)
DEFAULT_LOCK = REPOSITORY_ROOT / "compliance" / "requirements-lab" / "requirements-lab.lock.json"
DEFAULT_LAB_ROOT = REPOSITORY_ROOT.parent / "Document-Recreation"
DEFAULT_OBSERVATIONS = REPOSITORY_ROOT / "docs" / "testing" / "REQUIREMENTS-LAB-OBSERVATIONS.md"
DEFAULT_OBSERVATION_BASELINE = (
    REPOSITORY_ROOT
    / "compliance"
    / "requirements-lab"
    / "observations-historical-baseline.json"
)
OBSERVATION_HEADING = re.compile(r"^###\s+(RL-\d{3})\s+—", re.MULTILINE)
OBSERVATION_HEADING_PREFIX = re.compile(r"^###\s+RL-\d{3}\b", re.MULTILINE)
OBSERVATION_SECTION = re.compile(
    r"^###\s+(RL-\d{3})\s+—[^\r\n]*\r?\n"
    r"(?P<body>.*?)(?=^###\s+RL-\d{3}\s+—|\Z)",
    re.MULTILINE | re.DOTALL,
)
# Keep the historical digest parser above stable: the first 157 entries are
# frozen against its existing section-block baseline.  Recurrence validation
# needs a narrower view, however, because this log also contains unnumbered
# level-3 local-slice headings between numbered observations.  A numbered
# observation ends at the next level-3 heading, not at the next numbered one.
OBSERVATION_NUMBERED_SECTION = re.compile(
    r"^###\s+(RL-\d{3})\s+—[^\r\n]*\r?\n"
    r"(?P<body>.*?)(?=^###\s|\Z)",
    re.MULTILINE | re.DOTALL,
)
OBSERVATION_RECURRENCE_MARKER = re.compile(
    r"""
    \b(?:
        recurr\w*
        | re[- ]?expos\w*
        | reintroduc\w*
        | reappear\w*
        | reoccur\w*
        | go[- ]?back\w*
        | regress\w*
        | still\s+unresolved
        | remain(?:s)?\s+unresolved
        | unresolved\s+(?:requirements?|issues?|defects?|problems?|findings?)
        | (?:requirements?|issues?|defects?|problems?|findings?)\s+
          (?:remain(?:s)?|is|are)\s+unresolved
        | unresolved\s+(?:old|prior|historical)\s+issue
        | (?:old|prior|historical)\s+issue\s+\w+\s+unresolved
        | (?:was|were|is|are|has|have|had)\s+
          (?:not|n['’]t)\s+(?:actually\s+)?(?:yet\s+)?(?:been\s+)?fixed
        | never\s+(?:actually\s+)?fixed
        | not\s+(?:actually\s+)?(?:yet\s+)?(?:been\s+)?fixed
        | (?:fix|mitigation|correction|remediation)\s+
          (?:did|does|has|have|had)\s+(?:not|n['’]t)\s+
          (?:hold|work|close|resolve|last)
    )\b
    """,
    re.IGNORECASE | re.VERBOSE,
)
POST_RL_157_CUTOFF = 157


def _next_post_cutoff_identifier(text: str) -> str:
    """Return the next local observation ID after the immutable cutoff.

    This is deliberately local ledger numbering.  It must never be confused
    with a Requirements-Lab requirement/API ID, and callers still run the
    ledger checker to reject gaps, duplicates, or historical edits.
    """
    numbers = [
        int(identifier[3:])
        for identifier in OBSERVATION_HEADING.findall(text)
    ]
    return f"RL-{max(numbers, default=POST_RL_157_CUTOFF) + 1:03d}"


def _historical_observation_digest(text: str) -> tuple[tuple[str, ...], str]:
    """Return the historical section order and its immutable-content digest."""
    normalized = text.replace("\r\n", "\n").replace("\r", "\n")
    historical_sections: list[tuple[str, str]] = []
    for match in OBSERVATION_SECTION.finditer(normalized):
        identifier = match.group(1)
        if int(identifier[3:]) <= POST_RL_157_CUTOFF:
            historical_sections.append((identifier, match.group(0)))
    canonical = "\n".join(section for _, section in historical_sections)
    return tuple(identifier for identifier, _ in historical_sections), hashlib.sha256(
        canonical.encode("utf-8")
    ).hexdigest()


def _check_historical_observation_baseline(
    text: str, path: Path = DEFAULT_OBSERVATION_BASELINE
) -> tuple[str, ...]:
    """Ensure RL-001..RL-157 cannot be rewritten or silently reordered."""
    try:
        baseline = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return (f"cannot read historical observations baseline {path}: {error}",)
    if not isinstance(baseline, dict):
        return ("historical observations baseline must be a JSON object",)
    if baseline.get("schema_version") != 1:
        return ("historical observations baseline has unsupported schema_version",)
    if baseline.get("historical_cutoff") != POST_RL_157_CUTOFF:
        return (
            "historical observations baseline cutoff must match "
            f"RL-{POST_RL_157_CUTOFF:03d}",
        )
    observed_order, observed_digest = _historical_observation_digest(text)
    required_order = tuple(
        f"RL-{number:03d}" for number in range(1, POST_RL_157_CUTOFF + 1)
    )
    findings: list[str] = []
    if set(observed_order) != set(required_order) or len(observed_order) != len(
        required_order
    ):
        findings.append(
            "historical observations must contain each RL-001..RL-157 "
            "identifier exactly once"
        )
    recorded_digest = baseline.get("sha256")
    if not isinstance(recorded_digest, str) or recorded_digest != observed_digest:
        findings.append(
            "historical RL-001..RL-157 observation content/order differs from "
            "observations-historical-baseline.json"
        )
    return tuple(findings)


def _lab_root(value: Path | None) -> Path:
    root = value or Path(os.environ.get("HLA_REQUIREMENTS_LAB_ROOT", DEFAULT_LAB_ROOT))
    root = root.resolve()
    if not (root / "hla_lab" / "publication" / "export_compliance_bundle.py").is_file():
        raise ValueError(
            f"{root} is not an HLA Requirements Lab checkout; set "
            "HLA_REQUIREMENTS_LAB_ROOT or pass --lab-root"
        )
    return root


def export_bundle(
    *,
    lab_root: Path,
    output: Path,
    revision: str,
    edition: str,
    python: str,
) -> None:
    """Run the Lab's public bundle exporter with a controlled working directory."""
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    command = (
        python,
        "-m",
        "hla_lab.publication.export_compliance_bundle",
        "--corpus-root",
        str(lab_root),
        "--edition",
        edition,
        "--revision",
        revision,
        "--bundle-id",
        "umbra-hla-requirements",
        "--output",
        str(output),
    )
    subprocess.run(command, cwd=lab_root, check=True)


def _json_object(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON from {path}: {error}") from error
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def _locked_revision(lock_path: Path) -> str:
    lock = _json_object(lock_path)
    if lock.get("schema_version") != 1:
        raise ValueError(f"{lock_path} has an unsupported schema_version")
    revision = lock.get("revision")
    if not isinstance(revision, str) or not revision.strip():
        raise ValueError(f"{lock_path} must declare a non-empty revision")
    return revision


def _document(bundle: dict[str, Any], document_id: str) -> dict[str, Any]:
    documents = bundle.get("documents")
    if not isinstance(documents, list):
        raise ValueError("bundle has no documents array")
    matches = [item for item in documents if item.get("document_id") == document_id]
    if len(matches) != 1:
        raise ValueError(f"bundle must contain exactly one {document_id!r} document")
    return matches[0]


def _requirement_ordinal_integrity(document: dict[str, Any]) -> tuple[str, ...]:
    """Validate the presentation ordinals in one exported document.

    The Lab's opaque requirement IDs are the durable reference key, while the
    ordinal is the human-facing numbering used during review.  Pairwise
    re-sync detects a changed ordinal for a retained record, but it cannot
    protect a standalone check from an exporter that emits duplicate, missing,
    invalid, or gapped ordinals.  Keep this check deliberately narrow: it
    applies only to the requirements collection and never invents numbering
    for API, mapping, or transition records that do not carry an ordinal.
    """

    document_id = document.get("document_id", "<unknown-document>")
    requirements = document.get("requirements")
    if requirements is None:
        return ()
    if not isinstance(requirements, list):
        return (f"{document_id}: requirements must be an array",)
    if not requirements:
        return ()

    findings: list[str] = []
    seen_ordinals: dict[int, list[str]] = defaultdict(list)
    ordinals: list[int | None] = []
    for index, record in enumerate(requirements):
        prefix = f"{document_id}: requirements[{index}]"
        if not isinstance(record, dict):
            findings.append(f"{prefix} must be an object")
            ordinals.append(None)
            continue
        if "ordinal" not in record:
            findings.append(f"{prefix} is missing ordinal")
            ordinals.append(None)
            continue
        ordinal = record.get("ordinal")
        # bool is an int subclass in Python but is not a valid presentation
        # number.  Reject it explicitly so true/false cannot masquerade as 1/0.
        if isinstance(ordinal, bool) or not isinstance(ordinal, int) or ordinal < 1:
            findings.append(
                f"{prefix} has invalid ordinal {ordinal!r}; expected a positive integer"
            )
            ordinals.append(None)
            continue
        record_id = record.get("id")
        label = record_id if isinstance(record_id, str) and record_id else f"index {index}"
        seen_ordinals[ordinal].append(label)
        ordinals.append(ordinal)

    for ordinal, record_ids in sorted(seen_ordinals.items()):
        if len(record_ids) > 1:
            findings.append(
                f"{document_id}: duplicate requirement ordinal {ordinal} "
                f"({', '.join(record_ids)})"
            )

    if not findings and all(ordinal is not None for ordinal in ordinals):
        expected = list(range(1, len(requirements) + 1))
        actual = [ordinal for ordinal in ordinals if ordinal is not None]
        if actual != expected:
            missing = sorted(set(expected) - set(actual))
            unexpected = sorted(set(actual) - set(expected))
            details: list[str] = []
            if missing:
                details.append(f"missing {missing}")
            if unexpected:
                details.append(f"unexpected {unexpected}")
            findings.append(
                f"{document_id}: requirement ordinals must be contiguous 1..{len(requirements)}"
                + (f" ({'; '.join(details)})" if details else "")
            )

    return tuple(findings)


def check_bundle_ordinal_integrity(
    bundle: dict[str, Any], *, edition: str = "all"
) -> tuple[str, ...]:
    """Return findings for malformed requirement ordinal sequences.

    ``edition`` follows the resync command's 2010/2025/all selector.  The
    helper is intentionally usable by offline regression tests and callers
    that already loaded a candidate bundle, so it performs no filesystem I/O.
    """

    if edition not in {"2010", "2025", "all"}:
        raise ValueError("edition must be one of '2010', '2025', or 'all'")
    documents = bundle.get("documents")
    if not isinstance(documents, list):
        return ("bundle has no documents array",)
    selected_suffix = f"-{edition}" if edition != "all" else None
    findings: list[str] = []
    seen_document_ids: set[str] = set()
    for document in documents:
        if not isinstance(document, dict):
            findings.append("bundle documents must contain objects")
            continue
        document_id = document.get("document_id")
        if not isinstance(document_id, str) or not document_id:
            findings.append("bundle document is missing a non-empty document_id")
            continue
        if selected_suffix is not None and not document_id.endswith(selected_suffix):
            continue
        if document_id in seen_document_ids:
            findings.append(f"bundle contains duplicate document_id {document_id!r}")
            continue
        seen_document_ids.add(document_id)
        findings.extend(_requirement_ordinal_integrity(document))
    return tuple(findings)


_RESYNC_COLLECTIONS = (
    "requirements",
    "transitions",
    "mappings",
    "api_surfaces",
    "api_crosswalks",
    "requirement_api_bindings",
)


def _resync_normalize(value: Any, *, key: str | None = None) -> Any:
    """Normalize one Lab record for an edition-content comparison.

    Requirements-Lab record IDs are useful immutable references, but they can
    be regenerated when a source export moves a line or changes an ordinal.
    The re-sync comparison therefore removes only known derived identity and
    source-location fields.  Normative text, clauses, transitions, signatures,
    mappings, and all other fields remain part of the comparison.
    """

    if isinstance(value, dict):
        normalized: dict[str, Any] = {}
        for child_key, child_value in value.items():
            if child_key in {"id", "ordinal", "title"}:
                continue
            if child_key == "source" and isinstance(child_value, dict):
                normalized[child_key] = _resync_normalize(child_value, key=child_key)
                continue
            normalized[child_key] = _resync_normalize(child_value, key=child_key)
        return normalized
    if isinstance(value, list):
        return [_resync_normalize(item) for item in value]
    if key in {"line", "end_line"}:
        return None
    if isinstance(value, str):
        # Exporters preserve source whitespace inconsistently across editions
        # and regenerated PDFs.  Do not turn harmless wrapping into semantic
        # record drift, while retaining every non-whitespace character.
        return " ".join(value.split())
    return value


def _resync_digest(document: dict[str, Any]) -> str:
    canonical = _resync_normalize(document)
    encoded = json.dumps(canonical, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def _resync_record_fingerprint(record: object) -> str:
    canonical = _resync_normalize(record)
    encoded = json.dumps(canonical, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def _resync_collection_delta(
    baseline_records: object,
    candidate_records: object,
) -> dict[str, Any]:
    if not isinstance(baseline_records, list) or not isinstance(candidate_records, list):
        return {
            "baseline_count": None,
            "candidate_count": None,
            "missing_content": ["collection is not an array"],
            "added_content": [],
            "renumbered": [],
            "ordinal_drift": [],
        }

    baseline_by_fingerprint: dict[str, list[dict[str, Any]]] = defaultdict(list)
    candidate_by_fingerprint: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record in baseline_records:
        if isinstance(record, dict):
            baseline_by_fingerprint[_resync_record_fingerprint(record)].append(record)
    for record in candidate_records:
        if isinstance(record, dict):
            candidate_by_fingerprint[_resync_record_fingerprint(record)].append(record)

    missing_content: list[str] = []
    added_content: list[str] = []
    renumbered: list[dict[str, Any]] = []
    ordinal_drift: list[dict[str, Any]] = []
    recorded_ordinal_pairs: list[tuple[object, object, object, object]] = []
    for fingerprint in sorted(set(baseline_by_fingerprint) | set(candidate_by_fingerprint)):
        baseline_group = baseline_by_fingerprint.get(fingerprint, [])
        candidate_group = candidate_by_fingerprint.get(fingerprint, [])
        if len(baseline_group) > len(candidate_group):
            missing_content.extend([fingerprint] * (len(baseline_group) - len(candidate_group)))
        if len(candidate_group) > len(baseline_group):
            added_content.extend([fingerprint] * (len(candidate_group) - len(baseline_group)))
        for baseline_record, candidate_record in zip(baseline_group, candidate_group):
            baseline_id = baseline_record.get("id")
            candidate_id = candidate_record.get("id")
            if baseline_id != candidate_id:
                renumbered.append(
                    {
                        "baseline_id": baseline_id,
                        "candidate_id": candidate_id,
                        "content_sha256": fingerprint,
                    }
                )
            # ``ordinal`` is derived presentation numbering rather than
            # normative content, so it is intentionally excluded from the
            # semantic fingerprint above.  It must nevertheless be surfaced:
            # a Lab exporter can move a requirement's number while preserving
            # its text and opaque ID.  Keeping this as a separate delta lets
            # consumers distinguish numbering/reordering from content drift.
            baseline_has_ordinal = "ordinal" in baseline_record
            candidate_has_ordinal = "ordinal" in candidate_record
            baseline_ordinal = baseline_record.get("ordinal")
            candidate_ordinal = candidate_record.get("ordinal")
            if (
                (baseline_has_ordinal or candidate_has_ordinal)
                and baseline_ordinal != candidate_ordinal
            ):
                if isinstance(baseline_id, str) and isinstance(candidate_id, str):
                    recorded_ordinal_pairs.append(
                        (baseline_id, candidate_id, baseline_ordinal, candidate_ordinal)
                    )
                ordinal_drift.append(
                    {
                        "baseline_id": baseline_id,
                        "candidate_id": candidate_id,
                        "baseline_ordinal": baseline_ordinal,
                        "candidate_ordinal": candidate_ordinal,
                        "content_sha256": fingerprint,
                    }
                )

    # Content changes are intentionally reported as missing/added semantic
    # records.  If the exporter retained the opaque ID, however, still compare
    # its ordinal so a combined text-and-numbering edit cannot hide the
    # numbering movement.  Only unambiguous IDs are paired here; regenerated
    # or duplicate IDs remain represented by the content/ID deltas above.
    baseline_by_id: dict[str, list[dict[str, Any]]] = defaultdict(list)
    candidate_by_id: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record in baseline_records:
        record_id = record.get("id") if isinstance(record, dict) else None
        if isinstance(record_id, str):
            baseline_by_id[record_id].append(record)
    for record in candidate_records:
        record_id = record.get("id") if isinstance(record, dict) else None
        if isinstance(record_id, str):
            candidate_by_id[record_id].append(record)
    for record_id in sorted(set(baseline_by_id) & set(candidate_by_id), key=str):
        baseline_group = baseline_by_id[record_id]
        candidate_group = candidate_by_id[record_id]
        if len(baseline_group) != 1 or len(candidate_group) != 1:
            continue
        baseline_record = baseline_group[0]
        candidate_record = candidate_group[0]
        baseline_ordinal = baseline_record.get("ordinal")
        candidate_ordinal = candidate_record.get("ordinal")
        if (
            ("ordinal" in baseline_record or "ordinal" in candidate_record)
            and baseline_ordinal != candidate_ordinal
        ):
            pair = (record_id, record_id, baseline_ordinal, candidate_ordinal)
            if pair in recorded_ordinal_pairs:
                continue
            ordinal_drift.append(
                {
                    "baseline_id": record_id,
                    "candidate_id": record_id,
                    "baseline_ordinal": baseline_ordinal,
                    "candidate_ordinal": candidate_ordinal,
                    "content_sha256": _resync_record_fingerprint(candidate_record),
                }
            )

    return {
        "baseline_count": len(baseline_records),
        "candidate_count": len(candidate_records),
        "missing_content": missing_content,
        "added_content": added_content,
        "renumbered": renumbered,
        "ordinal_drift": ordinal_drift,
    }


def resync_bundles(
    baseline_path: Path,
    candidate_path: Path,
    *,
    edition: str,
) -> dict[str, Any]:
    """Build a deterministic edition-scoped content/ID/ordinal delta report."""

    baseline = _json_object(baseline_path)
    candidate = _json_object(candidate_path)
    baseline_documents = baseline.get("documents")
    candidate_documents = candidate.get("documents")
    if not isinstance(baseline_documents, list) or not isinstance(candidate_documents, list):
        raise ValueError("both bundles must contain a documents array")

    baseline_integrity = check_bundle_ordinal_integrity(baseline, edition=edition)
    candidate_integrity = check_bundle_ordinal_integrity(candidate, edition=edition)

    requested_ids = {
        "2010": {"hla-1516-2010", "hla-1516.1-2010", "hla-1516.2-2010"},
        "2025": {"hla-1516-2025", "hla-1516.1-2025", "hla-1516.2-2025"},
        "all": None,
    }[edition]
    baseline_by_id = {
        item.get("document_id"): item
        for item in baseline_documents
        if isinstance(item, dict)
    }
    candidate_by_id = {
        item.get("document_id"): item
        for item in candidate_documents
        if isinstance(item, dict)
    }
    document_ids = sorted(
        (set(baseline_by_id) | set(candidate_by_id))
        if requested_ids is None
        else (requested_ids & (set(baseline_by_id) | set(candidate_by_id)))
    )
    report_documents: list[dict[str, Any]] = []
    for document_id in document_ids:
        baseline_document = baseline_by_id.get(document_id)
        candidate_document = candidate_by_id.get(document_id)
        if baseline_document is None or candidate_document is None:
            report_documents.append(
                {
                    "document_id": document_id,
                    "status": "missing-document",
                    "baseline_present": baseline_document is not None,
                    "candidate_present": candidate_document is not None,
                }
            )
            continue
        collections = {
            collection: _resync_collection_delta(
                baseline_document.get(collection), candidate_document.get(collection)
            )
            for collection in _RESYNC_COLLECTIONS
        }
        baseline_ordinal_integrity = _requirement_ordinal_integrity(baseline_document)
        candidate_ordinal_integrity = _requirement_ordinal_integrity(candidate_document)
        changed_collections = {
            collection: delta
            for collection, delta in collections.items()
            if (
                delta["missing_content"]
                or delta["added_content"]
                or delta["renumbered"]
                or delta["ordinal_drift"]
            )
        }
        status = "unchanged"
        if changed_collections or baseline_ordinal_integrity or candidate_ordinal_integrity:
            status = "content-or-id-drift"
        report_documents.append(
            {
                "document_id": document_id,
                "status": status,
                "baseline_content_sha256": _resync_digest(baseline_document),
                "candidate_content_sha256": _resync_digest(candidate_document),
                "collections": collections,
                "ordinal_integrity": {
                    "baseline": list(baseline_ordinal_integrity),
                    "candidate": list(candidate_ordinal_integrity),
                },
            }
        )

    return {
        "edition": edition,
        "baseline": str(baseline_path.resolve()),
        "candidate": str(candidate_path.resolve()),
        "baseline_bundle_id": baseline.get("bundle_id"),
        "candidate_bundle_id": candidate.get("bundle_id"),
        "baseline_revision": baseline.get("corpus_revision"),
        "candidate_revision": candidate.get("corpus_revision"),
        "bundle_integrity": {
            "baseline": list(baseline_integrity),
            "candidate": list(candidate_integrity),
        },
        "documents": report_documents,
        "changed_documents": sum(
            document.get("status") != "unchanged" for document in report_documents
        ),
    }


def resync_has_numbering_drift(report: dict[str, Any]) -> bool:
    """Return whether a re-sync report contains an ID or ordinal move.

    Semantic additions/removals can be an intentional corpus update.  This
    narrower predicate lets CI reviewers gate specifically on a changed
    requirement/API/mapping/transition identity or presentation ordinal while
    still inspecting additive working-tree overlays separately.
    """

    documents = report.get("documents")
    if not isinstance(documents, list):
        documents = []
    for document in documents:
        if not isinstance(document, dict):
            continue
        collections = document.get("collections")
        if isinstance(collections, dict):
            for delta in collections.values():
                if not isinstance(delta, dict):
                    continue
                if delta.get("renumbered") or delta.get("ordinal_drift"):
                    return True
        ordinal_integrity = document.get("ordinal_integrity")
        if isinstance(ordinal_integrity, dict) and any(
            isinstance(findings, list) and findings
            for findings in ordinal_integrity.values()
        ):
            return True
    bundle_integrity = report.get("bundle_integrity")
    if isinstance(bundle_integrity, dict) and any(
        isinstance(findings, list) and findings
        for findings in bundle_integrity.values()
    ):
        return True
    return False


def _ids(record: dict[str, Any], name: str) -> tuple[str, ...]:
    values = record.get(name, [])
    if not isinstance(values, list) or not all(isinstance(item, str) for item in values):
        raise ValueError(f"record {record.get('id', '<unknown>')} has invalid {name}")
    return tuple(values)


def _header_path(value: object) -> Path | None:
    if not isinstance(value, str):
        return None
    root = (REPOSITORY_ROOT / "third_party" / "ieee1516.1-2025" / "include").resolve()
    candidate = (root / value).resolve()
    if not candidate.is_relative_to(root):
        return None
    return candidate


def _check_observations(
    path: Path = DEFAULT_OBSERVATIONS,
    *,
    baseline_path: Path = DEFAULT_OBSERVATION_BASELINE,
) -> tuple[str, ...]:
    """Return findings for malformed or ambiguously recorded observations.

    The first 157 entries are historical and immutable.  A later entry that
    describes a recurrence, including an old issue found still unresolved when
    its mitigation was expected to hold, must identify the earlier issue it
    re-exposes; this keeps a regression from being silently folded back into
    the old record.
    """
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        return (f"cannot read observations log {path}: {error}",)
    findings = list(_check_historical_observation_baseline(text, baseline_path))
    malformed_headings = [
        line.strip()
        for line in text.splitlines()
        if OBSERVATION_HEADING_PREFIX.match(line)
        and not OBSERVATION_HEADING.match(line)
    ]
    findings.extend(
        "observations log has a malformed RL heading (use an em dash): "
        f"{heading}"
        for heading in malformed_headings
    )
    identifiers = OBSERVATION_HEADING.findall(text)
    counts: dict[str, int] = {}
    for identifier in identifiers:
        counts[identifier] = counts.get(identifier, 0) + 1
    findings.extend(
        f"observations log contains duplicate identifier {identifier} ({count} headings)"
        for identifier, count in sorted(counts.items())
        if count > 1
    )
    post_cutoff_numbers = [
        int(identifier[3:])
        for identifier in identifiers
        if int(identifier[3:]) > POST_RL_157_CUTOFF
    ]
    if post_cutoff_numbers != sorted(post_cutoff_numbers):
        findings.append(
            f"post-RL-{POST_RL_157_CUTOFF} observations must remain in ascending identifier order"
        )
    if post_cutoff_numbers:
        expected_numbers = list(
            range(POST_RL_157_CUTOFF + 1, max(post_cutoff_numbers) + 1)
        )
        if post_cutoff_numbers != expected_numbers:
            findings.append(
                f"post-RL-{POST_RL_157_CUTOFF} observations must use each next RL-### identifier; "
                f"expected {expected_numbers}, found {post_cutoff_numbers}"
            )
    known_identifiers = set(identifiers)
    for match in OBSERVATION_NUMBERED_SECTION.finditer(text):
        identifier = match.group(1)
        number = int(identifier[3:])
        body = match.group("body")
        if number <= POST_RL_157_CUTOFF or not OBSERVATION_RECURRENCE_MARKER.search(body):
            continue
        cited_ids = set(re.findall(r"\bRL-\d{3}\b", body))
        cited_prior = [
            cited
            for cited in cited_ids
            if int(cited[3:]) < number and cited in known_identifiers
        ]
        unknown_prior = sorted(
            cited
            for cited in cited_ids
            if int(cited[3:]) < number and cited not in known_identifiers
        )
        if unknown_prior:
            findings.append(
                f"post-RL-{POST_RL_157_CUTOFF} recurrence {identifier} cites "
                "missing earlier observation(s): "
                + ", ".join(unknown_prior)
            )
        if not cited_prior:
            findings.append(
                f"post-RL-{POST_RL_157_CUTOFF} recurrence {identifier} must cite an earlier RL-### observation"
            )
    return tuple(findings)


def _check_source_and_tests(owned_id: object, owned: dict[str, Any]) -> list[str]:
    findings: list[str] = []
    source = owned.get("source")
    source_path = REPOSITORY_ROOT / source if isinstance(source, str) else None
    if source_path is None or not source_path.is_file():
        findings.append(f"{owned_id}: source path {source!r} is absent")
    else:
        source_symbol = owned.get("source_symbol")
        if not isinstance(source_symbol, str) or source_symbol not in source_path.read_text(
            encoding="utf-8"
        ):
            findings.append(f"{owned_id}: source symbol {source_symbol!r} is absent from {source}")
    tests = owned.get("tests")
    if not isinstance(tests, list) or not tests:
        findings.append(f"{owned_id}: at least one test reference is required")
    else:
        for test in tests:
            test_reference = str(test)
            test_file, separator, test_name = test_reference.partition("::")
            test_path = REPOSITORY_ROOT / test_file
            if not test_path.is_file():
                findings.append(f"{owned_id}: test path {test_file!r} is absent")
            elif separator and (
                not test_name or test_name not in test_path.read_text(encoding="utf-8")
            ):
                findings.append(f"{owned_id}: test selector {test_reference!r} is absent")
    return findings


def _check_requirements_reference_contract(
    contract: dict[str, Any], document: dict[str, Any]
) -> tuple[str, ...]:
    requirements = document.get("requirements")
    if not isinstance(requirements, list):
        return (f"bundle document {document.get('document_id')} has no requirements array",)
    lab_requirements = {item.get("id"): item for item in requirements if isinstance(item, dict)}
    owned_requirements = contract.get("requirements")
    if not isinstance(owned_requirements, list) or not owned_requirements:
        return ("requirements-reference contract must contain at least one requirement",)

    findings: list[str] = []
    for owned in owned_requirements:
        if not isinstance(owned, dict):
            findings.append("requirements-reference contract contains a non-object requirement")
            continue
        owned_id = owned.get("id", "<unknown>")
        lab_id = owned.get("requirements_lab_requirement_id")
        if not isinstance(lab_id, str):
            findings.append(f"{owned_id}: requirements_lab_requirement_id must be a string")
            continue
        lab_requirement = lab_requirements.get(lab_id)
        if lab_requirement is None:
            findings.append(f"{owned_id}: Lab requirement {lab_id!r} is absent")
            continue
        expected_clause = owned.get("clause_id")
        if not isinstance(expected_clause, str):
            findings.append(f"{owned_id}: clause_id must be a string")
        elif lab_requirement.get("clause_id") != expected_clause:
            findings.append(
                f"{owned_id}: clause ID drifted; expected {expected_clause!r}, "
                f"Lab has {lab_requirement.get('clause_id')!r}"
            )
        findings.extend(_check_source_and_tests(owned_id, owned))
    return tuple(findings)


def _check_api_reference_contract(
    contract: dict[str, Any], document: dict[str, Any]
) -> tuple[str, ...]:
    api_surfaces = document.get("api_surfaces")
    if not isinstance(api_surfaces, list):
        return (f"bundle document {document.get('document_id')} has no api_surfaces array",)
    lab_api_surfaces = {item.get("id"): item for item in api_surfaces if isinstance(item, dict)}
    owned_api_surfaces = contract.get("api_surfaces")
    if not isinstance(owned_api_surfaces, list) or not owned_api_surfaces:
        return ("api-reference contract must contain at least one api surface",)

    findings: list[str] = []
    for owned in owned_api_surfaces:
        if not isinstance(owned, dict):
            findings.append("api-reference contract contains a non-object api surface")
            continue
        owned_id = owned.get("id", "<unknown>")
        lab_id = owned.get("requirements_lab_api_surface_id")
        if not isinstance(lab_id, str):
            findings.append(f"{owned_id}: requirements_lab_api_surface_id must be a string")
            continue
        lab_api_surface = lab_api_surfaces.get(lab_id)
        if lab_api_surface is None:
            findings.append(f"{owned_id}: Lab API surface {lab_id!r} is absent")
            continue

        for field in ("owner", "language", "signature"):
            expected = owned.get(field)
            if not isinstance(expected, str):
                findings.append(f"{owned_id}: {field} must be a string")
            elif lab_api_surface.get(field) != expected:
                findings.append(
                    f"{owned_id}: {field} drifted; expected {expected!r}, "
                    f"Lab has {lab_api_surface.get(field)!r}"
                )
        try:
            expected_exceptions = _ids(owned, "exception_types")
            actual_exceptions = _ids(lab_api_surface, "exception_types")
        except ValueError as error:
            findings.append(str(error))
        else:
            if expected_exceptions != actual_exceptions:
                findings.append(
                    f"{owned_id}: exception types drifted; expected {expected_exceptions}, "
                    f"Lab has {actual_exceptions}"
                )
        findings.extend(_check_source_and_tests(owned_id, owned))
    return tuple(findings)


def check_contract(contract_path: Path, bundle_path: Path) -> tuple[str, ...]:
    """Return deterministic contract drift findings, or an empty tuple."""
    contract = _json_object(contract_path)
    bundle = _json_object(bundle_path)
    if contract.get("schema_version") != 1:
        return ("contract schema_version must be 1",)
    kind = contract.get("kind", "implementation-contract")
    if kind not in {
        "api-baseline",
        "implementation-contract",
        "requirements-reference",
        "api-reference",
    }:
        return (f"unsupported contract kind {kind!r}",)
    document_id = contract.get("document_id")
    if not isinstance(document_id, str):
        return ("contract document_id must be a string",)
    document = _document(bundle, document_id)
    if kind == "requirements-reference":
        return _check_requirements_reference_contract(contract, document)
    if kind == "api-reference":
        return _check_api_reference_contract(contract, document)
    mappings = document.get("mappings")
    if not isinstance(mappings, list):
        return (f"bundle document {document_id} has no mappings array",)
    lab_mappings = {item.get("id"): item for item in mappings if isinstance(item, dict)}
    lab_api_surfaces = {
        item.get("id"): item
        for item in document.get("api_surfaces", [])
        if isinstance(item, dict)
    }
    findings: list[str] = []
    owned_mappings = contract.get("mappings")
    if not isinstance(owned_mappings, list) or not owned_mappings:
        return ("contract must contain at least one mapping",)
    for owned in owned_mappings:
        if not isinstance(owned, dict):
            findings.append("contract contains a non-object mapping")
            continue
        owned_id = owned.get("id", "<unknown>")
        lab_id = owned.get("requirements_lab_mapping_id")
        if not isinstance(lab_id, str):
            findings.append(f"{owned_id}: requirements_lab_mapping_id must be a string")
            continue
        lab_mapping = lab_mappings.get(lab_id)
        if lab_mapping is None:
            findings.append(f"{owned_id}: Lab mapping {lab_id!r} is absent")
            continue
        try:
            expected_requirements = _ids(owned, "requirement_ids")
            actual_requirements = _ids(lab_mapping, "requirement_ids")
            expected_transitions = _ids(owned, "transition_ids")
            actual_transitions = _ids(lab_mapping, "transition_ids")
        except ValueError as error:
            findings.append(str(error))
            continue
        if expected_requirements != actual_requirements:
            findings.append(
                f"{owned_id}: requirement IDs drifted; expected {expected_requirements}, "
                f"Lab has {actual_requirements}"
            )
        if expected_transitions != actual_transitions:
            findings.append(
                f"{owned_id}: transition IDs drifted; expected {expected_transitions}, "
                f"Lab has {actual_transitions}"
            )

        # The Lab models Connect as one aggregate service mapping while the
        # official C++ binding exposes four overload-level API surfaces.  An
        # implementation contract may select those exact API records without
        # changing or pretending to repair the Lab's aggregate crosswalk.
        if "requirements_lab_api_surface_ids" in owned:
            try:
                selected_api_surface_ids = _ids(owned, "requirements_lab_api_surface_ids")
            except ValueError as error:
                findings.append(str(error))
                selected_api_surface_ids = ()
            if not selected_api_surface_ids:
                findings.append(
                    f"{owned_id}: requirements_lab_api_surface_ids must select at least one API surface"
                )
            if len(selected_api_surface_ids) != len(set(selected_api_surface_ids)):
                findings.append(
                    f"{owned_id}: requirements_lab_api_surface_ids contains duplicate IDs"
                )
            for api_surface_id in selected_api_surface_ids:
                api_surface = lab_api_surfaces.get(api_surface_id)
                if api_surface is None:
                    findings.append(
                        f"{owned_id}: Lab API surface {api_surface_id!r} is absent"
                    )
                    continue
                if api_surface.get("owner") != "RTIambassador":
                    findings.append(
                        f"{owned_id}: selected API surface {api_surface_id!r} is not owned by RTIambassador"
                    )
                if api_surface.get("language") != "cpp":
                    findings.append(
                        f"{owned_id}: selected API surface {api_surface_id!r} is not a C++ surface"
                    )
                signature = api_surface.get("signature")
                if not isinstance(signature, str) or not re.search(r"\bconnect\s*\(", signature):
                    findings.append(
                        f"{owned_id}: selected API surface {api_surface_id!r} is not a Connect declaration"
                    )
        if kind == "api-baseline":
            header = owned.get("header")
            header_path = _header_path(header)
            if header_path is None or not header_path.is_file():
                findings.append(f"{owned_id}: header path {header!r} is absent")
                continue
            marker = owned.get("declaration_marker")
            minimum_matches = owned.get("minimum_matches")
            if not isinstance(marker, str) or not isinstance(minimum_matches, int):
                findings.append(f"{owned_id}: API baseline needs marker and minimum_matches")
                continue
            actual_matches = header_path.read_text(encoding="utf-8").count(marker)
            if actual_matches < minimum_matches:
                findings.append(
                    f"{owned_id}: expected at least {minimum_matches} occurrences of "
                    f"{marker!r} in {header}, found {actual_matches}"
                )
            continue

        findings.extend(_check_source_and_tests(owned_id, owned))
    return tuple(findings)


def _catch2_test_sources() -> tuple[str, ...]:
    """Load the native Catch2 source text used by the planning catalog.

    The plan intentionally stores human-readable Catch2 selectors rather than
    generated IDs.  Keeping the source lookup here makes the plan checker a
    local integrity guard only; it does not turn a selector into conformance
    evidence or require a particular build configuration.
    """

    sources: list[str] = []
    test_root = REPOSITORY_ROOT / "cpp" / "tests"
    for path in sorted(test_root.rglob("*.cpp")):
        try:
            # Catch2 names may be split across adjacent C++ string literals.
            # Whitespace-normalizing the source preserves exact selector text
            # while making a wrapped literal equivalent to its runtime name.
            sources.append(" ".join(path.read_text(encoding="utf-8").split()))
        except OSError:
            # Report the missing source through the normal plan finding rather
            # than allowing one unreadable optional test file to abort the
            # entire checker with a traceback.
            continue
    return tuple(sources)


def _catch2_selector_parts(value: str) -> tuple[str, ...]:
    """Return the individual selectors stored in a compact plan entry.

    A few encoding/foundation entries intentionally map one plan record to
    two or three adjacent Catch2 cases and separate their names with a
    semicolon.  Each component is still an exact source selector.
    """

    return tuple(part.strip() for part in value.split(";") if part.strip())


def check_catch2_plan(plan_path: Path, bundle_path: Path) -> tuple[str, ...]:
    """Return findings for the Catch2 plan's 2025 Lab references.

    This is deliberately separate from typed implementation contracts.  The
    plan is planning input, so an entry may omit requirement or API references
    when the Lab has no suitable row.  Whenever a reference is supplied,
    however, it must resolve against the selected 2025 document.  The source
    selector is also checked locally so a stale test name cannot survive a
    requirements resync unnoticed.
    """

    plan = _json_object(plan_path)
    bundle = _json_object(bundle_path)
    findings: list[str] = []
    if plan.get("schema_version") != 1:
        findings.append("Catch2 plan schema_version must be 1")
    if plan.get("runner") != "catch2":
        findings.append("Catch2 plan runner must be 'catch2'")
    if plan.get("status") != "active":
        findings.append("Catch2 plan status must be 'active'")
    document_id = plan.get("document_id")
    if document_id != "hla-1516.1-2025":
        findings.append(
            "Catch2 plan document_id must be 'hla-1516.1-2025' for the 2025 C++ lane"
        )
        return tuple(findings)
    try:
        document = _document(bundle, document_id)
    except ValueError as error:
        return tuple([*findings, str(error)])

    requirements = document.get("requirements")
    api_surfaces = document.get("api_surfaces")
    if not isinstance(requirements, list):
        findings.append(f"bundle document {document_id} has no requirements array")
        requirements = []
    if not isinstance(api_surfaces, list):
        findings.append(f"bundle document {document_id} has no api_surfaces array")
        api_surfaces = []
    # The plan is anchored to the Part 1.1 API document, but its C++ tests
    # also cover 1516.2 FOM and encoding rules.  Requirements therefore resolve
    # across the complete 2025 edition while API surfaces remain Part 1.1
    # binding records.
    lab_requirement_ids = {
        item.get("id") for item in requirements if isinstance(item, dict)
    }
    for candidate_document in bundle.get("documents", []):
        if not isinstance(candidate_document, dict):
            continue
        candidate_id = candidate_document.get("document_id")
        if not isinstance(candidate_id, str) or not candidate_id.endswith("-2025"):
            continue
        candidate_requirements = candidate_document.get("requirements")
        if isinstance(candidate_requirements, list):
            lab_requirement_ids.update(
                item.get("id")
                for item in candidate_requirements
                if isinstance(item, dict)
            )
    lab_api_surfaces = {
        item.get("id"): item for item in api_surfaces if isinstance(item, dict)
    }

    entries = plan.get("tests")
    if not isinstance(entries, list) or not entries:
        findings.append("Catch2 plan must contain a non-empty tests array")
        return tuple(findings)

    source_texts = _catch2_test_sources()
    if not source_texts:
        findings.append("Catch2 plan source directory contains no readable .cpp files")

    seen_entry_ids: set[str] = set()
    for index, entry in enumerate(entries):
        prefix = f"Catch2 plan entry {index + 1}"
        if not isinstance(entry, dict):
            findings.append(f"{prefix} must be an object")
            continue
        entry_id = entry.get("id")
        if not isinstance(entry_id, str) or not entry_id.strip():
            findings.append(f"{prefix}: id must be a non-empty string")
            entry_id = f"<entry-{index + 1}>"
        elif entry_id in seen_entry_ids:
            findings.append(f"{prefix}: duplicate id {entry_id!r}")
        else:
            seen_entry_ids.add(entry_id)

        test_case = entry.get("test_case")
        if not isinstance(test_case, str) or not test_case.strip():
            findings.append(f"{entry_id}: test_case must be a non-empty string")
        else:
            for selector in _catch2_selector_parts(test_case):
                normalized_selector = " ".join(selector.split())
                if not any(normalized_selector in source for source in source_texts):
                    findings.append(
                        f"{entry_id}: Catch2 selector {selector!r} is absent from cpp/tests"
                    )

        tags = entry.get("tags")
        if not isinstance(tags, list) or not tags or not all(
            isinstance(tag, str) and tag.strip() for tag in tags
        ):
            findings.append(f"{entry_id}: tags must be a non-empty string array")
        status = entry.get("status")
        if not isinstance(status, str) or not status.strip():
            findings.append(f"{entry_id}: status must be a non-empty string")
        next_action = entry.get("next_action")
        if not isinstance(next_action, str) or not next_action.strip():
            findings.append(f"{entry_id}: next_action must be a non-empty string")

        selected_requirements = entry.get("selected_requirements_lab_requirement_ids")
        if selected_requirements is not None:
            if not isinstance(selected_requirements, list) or not all(
                isinstance(item, str) and item.strip() for item in selected_requirements
            ):
                findings.append(
                    f"{entry_id}: selected_requirements_lab_requirement_ids must be a string array"
                )
            else:
                if len(selected_requirements) != len(set(selected_requirements)):
                    findings.append(
                        f"{entry_id}: selected_requirements_lab_requirement_ids contains duplicates"
                    )
                for requirement_id in selected_requirements:
                    if requirement_id not in lab_requirement_ids:
                        findings.append(
                            f"{entry_id}: Lab requirement {requirement_id!r} is absent"
                        )

        selected_api_surfaces = entry.get("selected_cpp_api_surface_ids")
        if selected_api_surfaces is not None:
            if not isinstance(selected_api_surfaces, list) or not all(
                isinstance(item, str) and item.strip() for item in selected_api_surfaces
            ):
                findings.append(
                    f"{entry_id}: selected_cpp_api_surface_ids must be a string array"
                )
            else:
                if len(selected_api_surfaces) != len(set(selected_api_surfaces)):
                    findings.append(
                        f"{entry_id}: selected_cpp_api_surface_ids contains duplicates"
                    )
                for api_surface_id in selected_api_surfaces:
                    api_surface = lab_api_surfaces.get(api_surface_id)
                    if api_surface is None:
                        findings.append(
                            f"{entry_id}: Lab API surface {api_surface_id!r} is absent"
                        )
                    elif api_surface.get("language") != "cpp":
                        findings.append(
                            f"{entry_id}: selected API surface {api_surface_id!r} is not a C++ surface"
                        )

    return tuple(findings)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    export = subparsers.add_parser("export", help="Export a Lab CorpusBundle into .compliance")
    export.add_argument("--lab-root", type=Path)
    export.add_argument("--output", type=Path, default=DEFAULT_BUNDLE)
    export.add_argument("--lock", type=Path, default=DEFAULT_LOCK)
    export.add_argument("--revision", help="Override the reviewed Requirements Lab revision")
    export.add_argument("--edition", choices=("2010", "2025", "all"), default="all")
    export.add_argument("--python", default=sys.executable, help="Python environment for the Lab")

    check = subparsers.add_parser("check", help="Check an Umbra baseline or contract against a bundle")
    check.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    check.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)

    check_plan = subparsers.add_parser(
        "check-plan",
        help="Check the native Catch2 plan's 2025 Lab references and selectors",
    )
    check_plan.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    check_plan.add_argument("--plan", type=Path, default=DEFAULT_CATCH2_PLAN)

    check_observations = subparsers.add_parser(
        "check-observations",
        help="Check the immutable historical and post-RL-157 recurrence ledger",
    )
    check_observations.add_argument(
        "--observations", type=Path, default=DEFAULT_OBSERVATIONS
    )
    check_observations.add_argument(
        "--baseline", type=Path, default=DEFAULT_OBSERVATION_BASELINE
    )

    resync = subparsers.add_parser(
        "resync",
        help=(
            "Compare two bundles by normalized edition content and report "
            "missing, added, regenerated IDs, or ordinal-numbering drift"
        ),
    )
    resync.add_argument("--baseline", type=Path, default=DEFAULT_BUNDLE)
    resync.add_argument("--candidate", type=Path, required=True)
    resync.add_argument("--edition", choices=("2010", "2025", "all"), default="2025")
    resync.add_argument(
        "--fail-on-diff",
        action="store_true",
        help="return status 1 when content, record IDs, or ordinals differ",
    )
    resync.add_argument(
        "--fail-on-numbering-drift",
        action="store_true",
        help="return status 1 only when an ID or ordinal-numbering move is found",
    )

    args = parser.parse_args()
    try:
        if args.command == "export":
            export_bundle(
                lab_root=_lab_root(args.lab_root),
                output=args.output,
                revision=args.revision or _locked_revision(args.lock),
                edition=args.edition,
                python=args.python,
            )
            print(f"exported Requirements Lab bundle to {args.output}")
            return 0
        if args.command == "resync":
            report = resync_bundles(
                args.baseline,
                args.candidate,
                edition=args.edition,
            )
            print(json.dumps(report, ensure_ascii=True, indent=2, sort_keys=True))
            if args.fail_on_diff and (
                report["changed_documents"]
                or any(report.get("bundle_integrity", {}).values())
            ):
                return 1
            if args.fail_on_numbering_drift and resync_has_numbering_drift(report):
                return 1
            return 0
        if args.command == "check-observations":
            findings = list(
                _check_observations(
                    args.observations,
                    baseline_path=args.baseline,
                )
            )
            if findings:
                print("requirements-lab: observation ledger drift detected", file=sys.stderr)
                for finding in findings:
                    print(f"- {finding}", file=sys.stderr)
                return 1
            observation_text = args.observations.read_text(encoding="utf-8")
            identifiers = OBSERVATION_HEADING.findall(observation_text)
            latest = identifiers[-1] if identifiers else "none"
            next_identifier = _next_post_cutoff_identifier(observation_text)
            print(
                "requirements-lab: observation ledger is valid "
                f"({len(identifiers)} numbered entries, latest {latest}, "
                f"next {next_identifier})"
            )
            return 0
        if args.command == "check-plan":
            findings = list(check_catch2_plan(args.plan, args.bundle))
            findings.extend(
                check_bundle_ordinal_integrity(
                    _json_object(args.bundle),
                    edition="2025",
                )
            )
            findings.extend(_check_observations())
            if findings:
                print("requirements-lab: Catch2 plan drift detected", file=sys.stderr)
                for finding in findings:
                    print(f"- {finding}", file=sys.stderr)
                return 1
            plan = _json_object(args.plan)
            document = _document(_json_object(args.bundle), plan["document_id"])
            entries = plan["tests"]
            requirement_ids = {
                requirement_id
                for entry in entries
                for requirement_id in entry.get(
                    "selected_requirements_lab_requirement_ids", []
                )
            }
            api_surface_ids = {
                api_surface_id
                for entry in entries
                for api_surface_id in entry.get("selected_cpp_api_surface_ids", [])
            }
            print(
                "requirements-lab: Catch2 plan matches the supplied bundle "
                f"({len(entries)} entries, {len(requirement_ids)} requirements, "
                f"{len(api_surface_ids)} C++ API surfaces; "
                f"{len(document.get('requirements', []))} Lab requirements available)"
            )
            return 0
        findings = list(check_contract(args.contract, args.bundle))
        findings.extend(
            check_bundle_ordinal_integrity(
                _json_object(args.bundle),
                edition="all",
            )
        )
        findings.extend(_check_observations())
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"requirements-lab: {error}", file=sys.stderr)
        return 2
    if findings:
        print("requirements-lab: contract drift detected", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("requirements-lab: baseline or contract matches the supplied bundle")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
