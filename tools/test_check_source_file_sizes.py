"""Internal source-policy utility checks, not RTI/IEEE behavioral evidence."""
from pathlib import Path

import pytest

from tools import check_source_file_sizes as checker


@pytest.mark.parametrize("suffix", [".ts", ".cjs", ".schema.json"])
def test_explicit_generated_ratchets_are_enforced_without_false_missing_reports(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, suffix: str,
) -> None:
    source = tmp_path / f"generated{suffix}"
    source.write_text("one\ntwo\n", encoding="utf-8")
    monkeypatch.setattr(checker, "REPOSITORY_ROOT", tmp_path)
    monkeypatch.setattr(checker, "_tracked_paths", lambda: [source])
    baseline = {source.name: {"max_lines": 2, "reason": "Explicit generated artifact"}}
    assert checker._scan({}, baseline) == ([], [])
    source.write_text("one\ntwo\nthree\n", encoding="utf-8")
    _, errors = checker._scan({}, baseline)
    assert len(errors) == 1 and "legacy file grew" in errors[0] and "missing" not in errors[0]
    assert source.read_text(encoding="utf-8") == "one\ntwo\nthree\n"


@pytest.mark.parametrize("listed", [False, True])
def test_real_missing_registered_artifacts_still_fail(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, listed: bool,
) -> None:
    missing = tmp_path / "absent.ts"
    monkeypatch.setattr(checker, "REPOSITORY_ROOT", tmp_path)
    monkeypatch.setattr(checker, "_tracked_paths", lambda: [missing] if listed else [])
    _, errors = checker._scan({}, {missing.name: {"max_lines": 2}})
    assert errors == ["size-policy baseline names a missing source file: absent.ts"]


def test_unregistered_unknown_formats_are_not_silently_added_to_the_policy(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    source = tmp_path / "unregistered.ts"
    source.write_text("one\ntwo\n", encoding="utf-8")
    monkeypatch.setattr(checker, "REPOSITORY_ROOT", tmp_path)
    monkeypatch.setattr(checker, "_tracked_paths", lambda: [source])
    assert checker._scan({}, {}) == ([], [])


def test_cpp_fragment_expansion_and_existing_ratchets_remain_enforced(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    source = tmp_path / "cpp/src/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text('one\n#include "part.inc"\nthree\n', encoding="utf-8")
    fragment = source.with_name("part.inc")
    fragment.write_text("four\nfive\n", encoding="utf-8")
    monkeypatch.setattr(checker, "REPOSITORY_ROOT", tmp_path)
    monkeypatch.setattr(checker, "_tracked_paths", lambda: [source, fragment])
    _, errors = checker._scan({"production_cpp": checker.Policy(soft=1, hard=4)},
        {"cpp/src/example.cpp": {"max_lines": 3, "max_expanded_lines": 4}})
    assert len(errors) == 1 and "source plus included fragments exceeds its ratchet" in errors[0]
