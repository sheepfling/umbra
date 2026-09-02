"""Offline regression checks for the Requirements Lab observation ledger.

This intentionally uses the real observation file and immutable historical
baseline, then evaluates temporary in-memory variants.  It proves that a
reproduced unresolved/not-fixed issue receives a new post-RL-157 identifier
with an earlier citation, while historical-ID reuse, historical edits, and
skipped identifiers remain errors.  It does not modify the checked-in ledger.
"""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path


SCRIPT_ROOT = Path(__file__).resolve().parent
REPOSITORY_ROOT = SCRIPT_ROOT.parent
sys.path.insert(0, str(SCRIPT_ROOT))

import requirements_lab  # noqa: E402  (repository tool import after path setup)


OBSERVATIONS_PATH = REPOSITORY_ROOT / "docs" / "testing" / "REQUIREMENTS-LAB-OBSERVATIONS.md"
BASELINE_PATH = (
    REPOSITORY_ROOT
    / "compliance"
    / "requirements-lab"
    / "observations-historical-baseline.json"
)


def _with_appended_observation(
    text: str,
    body: str,
    identifier: str | None = None,
) -> str:
    marker = "\n## Recording rules"
    if marker not in text:
        raise AssertionError("observation log has no recording-rules boundary")
    if identifier is None:
        identifier = requirements_lab._next_post_cutoff_identifier(text)
    section = f"\n### {identifier} — Synthetic recurrence guard\n\n{body.strip()}\n"
    return text.replace(marker, section + marker, 1)


def _check_text(text: str) -> tuple[str, ...]:
    """Run the production checker against a temporary, untracked ledger."""
    with tempfile.TemporaryDirectory(prefix="umbra-observation-regression-") as directory:
        path = Path(directory) / "observations.md"
        path.write_text(text, encoding="utf-8")
        return requirements_lab._check_observations(path, baseline_path=BASELINE_PATH)


def main() -> int:
    text = OBSERVATIONS_PATH.read_text(encoding="utf-8")

    findings = requirements_lab._check_observations(
        OBSERVATIONS_PATH, baseline_path=BASELINE_PATH
    )
    if findings:
        raise AssertionError("live observation ledger is not valid: " + "; ".join(findings))

    next_identifier = requirements_lab._next_post_cutoff_identifier(text)
    cited_recurrence = _with_appended_observation(
        text,
        "The earlier RL-176 issue remains unresolved after its expected mitigation; "
        "this synthetic current reproduction cites RL-176.",
    )
    findings = _check_text(cited_recurrence)
    if findings:
        raise AssertionError(
            "a cited post-RL-157 unresolved reproduction should be accepted: "
            + "; ".join(findings)
        )

    cited_not_fixed = _with_appended_observation(
        text,
        "The prior RL-156 issue was not fixed after its expected mitigation; "
        "this synthetic reproduction cites RL-156.",
    )
    findings = _check_text(cited_not_fixed)
    if findings:
        raise AssertionError(
            "a cited post-RL-157 not-fixed reproduction should be accepted: "
            + "; ".join(findings)
        )

    uncited_recurrence = _with_appended_observation(
        text,
        "The earlier issue remains unresolved after its expected mitigation, "
        "but this synthetic reproduction intentionally omits its earlier RL citation.",
    )
    findings = _check_text(uncited_recurrence)
    expected_uncited = (
        f"post-RL-157 recurrence {next_identifier} must cite an earlier RL-### observation"
    )
    if expected_uncited not in findings:
        raise AssertionError(
            "an uncited post-RL-157 unresolved reproduction was not rejected: "
            + "; ".join(findings)
        )

    uncited_not_fixed = _with_appended_observation(
        text,
        "The earlier issue was not actually fixed after the expected mitigation; "
        "this synthetic reproduction intentionally omits its earlier RL citation.",
    )
    findings = _check_text(uncited_not_fixed)
    if expected_uncited not in findings:
        raise AssertionError(
            "an uncited post-RL-157 not-fixed reproduction was not rejected: "
            + "; ".join(findings)
        )

    reused_historical = _with_appended_observation(
        text,
        "This synthetic entry deliberately reuses an immutable historical ID.",
        identifier="RL-156",
    )
    findings = _check_text(reused_historical)
    if not any("duplicate identifier RL-156" in finding for finding in findings):
        raise AssertionError(
            "reuse of an RL-001..RL-157 identifier was not rejected: "
            + "; ".join(findings)
        )

    skipped_identifier = _with_appended_observation(
        text,
        "This synthetic audit entry deliberately skips the next identifier.",
        identifier=f"RL-{int(next_identifier[3:]) + 1:03d}",
    )
    findings = _check_text(skipped_identifier)
    if not any("must use each next RL-### identifier" in finding for finding in findings):
        raise AssertionError(
            "a skipped post-RL-157 identifier was not rejected: " + "; ".join(findings)
        )

    historical_edit = text.replace(
        "### RL-001 —",
        "### RL-001 — Synthetic historical edit",
        1,
    )
    findings = _check_text(historical_edit)
    if not any("historical RL-001..RL-157 observation content/order differs" in finding for finding in findings):
        raise AssertionError(
            "a historical observation edit was not rejected: " + "; ".join(findings)
        )

    print("requirements-lab: observation recurrence regression guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
