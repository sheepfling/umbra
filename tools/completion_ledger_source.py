"""Validate historical completion-ledger pointers against live plan locations."""

from __future__ import annotations

from pathlib import Path


def completion_ledger_source_pointer_findings(
    rows: list[object],
    live_sources: dict[str, str],
    reviewed_relocations: dict[str, tuple[str, str]],
    repository_root: Path,
) -> list[str]:
    """Keep historical pointers navigable without rewriting them to live lines."""
    root = repository_root.resolve()

    def checked_path(location: str) -> str | None:
        path_text, separator, line_text = location.rpartition(":")
        if not separator or not path_text or not line_text.isdigit() or int(line_text) < 1:
            return None
        path = Path(path_text)
        if path.is_absolute():
            return None
        resolved = (root / path).resolve()
        if not resolved.is_relative_to(root) or not resolved.is_file():
            return None
        return path.as_posix()

    findings: list[str] = []
    observed_relocations: set[str] = set()
    for row in rows:
        if not isinstance(row, dict):
            continue
        plan_id = row.get("plan_id")
        historical = row.get("source_location")
        current = live_sources.get(plan_id) if isinstance(plan_id, str) else None
        if not isinstance(historical, str) or not isinstance(current, str):
            continue  # Retired or source-unlocated history remains audit-only.
        old_path = checked_path(historical)
        new_path = checked_path(current)
        if old_path is None or new_path is None:
            findings.append(
                f"{plan_id}: invalid historical/current source pointer: {historical} -> {current}"
            )
        elif old_path != new_path:
            if reviewed_relocations.get(plan_id) != (old_path, new_path):
                findings.append(f"{plan_id}: unreviewed source relocation: {historical} -> {current}")
            else:
                observed_relocations.add(plan_id)
    for plan_id in reviewed_relocations.keys() - observed_relocations:
        findings.append(f"{plan_id}: reviewed source relocation no longer matches the ledger and plan")
    return findings
