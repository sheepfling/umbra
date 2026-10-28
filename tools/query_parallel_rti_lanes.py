"""Print the small, file-owned 2025 RTI implementation queue.

Run ``query_rti_work.py resume`` first for the ordinary handoff, then this
command only when coordinating concurrent lanes. It reads the roadmap index;
it never synchronizes or rewrites the pinned Requirements Lab export.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

INDEX = Path(__file__).resolve().parents[1] / "docs" / "planning" / "ROADMAP-INDEX.json"


def main() -> int:
    # Windows PowerShell may expose a legacy code page that cannot encode the
    # arrows and section symbols stored in the roadmap's human-readable notes.
    sys.stdout.reconfigure(errors="backslashreplace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--id", help="show one exact parallel lane id")
    parser.add_argument(
        "--state",
        choices=("ready", "active", "in-progress", "blocked", "verified", "all"),
        help="filter lanes (default: active, or all with --id)",
    )
    parser.add_argument(
        "--json", action="store_true", help="emit machine-readable records"
    )
    args = parser.parse_args()

    roadmap = json.loads(INDEX.read_text(encoding="utf-8"))
    active_handoff = roadmap.get("active_handoff", {})
    lanes = roadmap.get("parallel_implementation_lanes")
    if lanes is None and isinstance(active_handoff, dict):
        lanes = active_handoff.get("parallel_implementation_lanes", [])
    if lanes is None:
        lanes = []
    if not isinstance(lanes, list):
        parser.error("parallel_implementation_lanes must be an array")
    if args.id:
        lanes = [lane for lane in lanes if lane.get("id") == args.id]
        if not lanes:
            parser.error(f"unknown lane id: {args.id}")
    state = args.state or ("all" if args.id else "active")
    if state != "all":
        normalized_state = "in-progress" if state == "active" else state
        if normalized_state == "in-progress":
            lanes = [
                lane for lane in lanes if lane.get("state") in {"active", "in-progress"}
            ]
        else:
            lanes = [lane for lane in lanes if lane.get("state") == normalized_state]

    if args.json:
        print(json.dumps(lanes, indent=2, ensure_ascii=True))
        return 0

    if not lanes:
        print(f"No {state} parallel implementation lanes are queued.")
        return 0

    def display(value: object) -> str:
        return str(value).encode("ascii", "backslashreplace").decode("ascii")

    for lane in lanes:
        print(
            f"{display(lane['id'])} [{display(lane['state'])}] -> "
            f"{display(lane['canonical_section'])}"
        )
        requirement_id = lane.get("requirement_id") or lane.get("target_requirement_id")
        print(f"  requirement: {display(requirement_id or '<none>')}")
        print(f"  source: {display(lane['source_target'])}")
        print(f"  file owner: {display(lane['file_owner'])}")
        print(f"  next: {display(lane['next_check'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
