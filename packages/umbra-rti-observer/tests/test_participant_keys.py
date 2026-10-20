"""Internal cross-language attribution codec evidence, not RTI conformance."""
from __future__ import annotations

import json
from pathlib import Path

import pytest
from umbra_rti_observer.attribution import event_perspective

CASES = json.loads((Path(__file__).resolve().parents[1] / "conformance" / "participant-keys-v1.json").read_text(encoding="utf-8"))


@pytest.mark.parametrize("row", CASES, ids=[row["name"] for row in CASES])
def test_canonical_keys_match_portable_corpus(row):
    perspective = event_perspective(row["event"])
    actual = perspective.key if perspective is not None else None
    if perspective and not row["provenance"] and perspective.attribution is None:
        actual = json.dumps(["legacy-name", perspective.label], ensure_ascii=False, separators=(",", ":"))
    expected = json.dumps(row["components"], ensure_ascii=False, separators=(",", ":")) if row["components"] is not None else None
    assert actual == expected
