#!/usr/bin/env python3
"""Check line links in the IEEE 1516.1-2025 topic guides and roadmap."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DESIGN = ROOT / "docs" / "design"
LINK_PATTERN = re.compile(r"\]\(([^)\s]+)#L([1-9][0-9]*)(?:-[1-9][0-9]*)?(?:\s[^)]*)?\)")


def markdown_sources() -> list[Path]:
    return sorted(DESIGN.glob("HLA-2025-*.md"))


def collect_references(sources: list[Path]) -> set[tuple[str, int, str]]:
    references: set[tuple[str, int, str]] = set()
    for source in sources:
        content = source.read_text(encoding="utf-8")
        for target, line_text in LINK_PATTERN.findall(content):
            if "://" in target:
                continue
            target_path = (source.parent / target).resolve()
            try:
                relative_path = target_path.relative_to(ROOT).as_posix()
            except ValueError:
                continue
            references.add((relative_path, int(line_text), source.name))
    return references


def git_tree_line_counts(revision: str, target_paths: set[str]) -> dict[str, int | None]:
    tree = subprocess.run(
        ["git", "ls-tree", "-r", "-z", "--full-tree", revision],
        cwd=ROOT,
        check=True,
        capture_output=True,
    ).stdout
    object_ids: dict[str, str] = {}
    for entry in tree.split(b"\0"):
        if not entry:
            continue
        metadata, raw_path = entry.split(b"\t", 1)
        _, object_type, object_id = metadata.split()
        if object_type == b"blob":
            object_ids[raw_path.decode("utf-8", errors="surrogateescape")] = object_id.decode()

    result: dict[str, int | None] = {path: None for path in target_paths}
    present_paths = sorted(path for path in target_paths if path in object_ids)
    if not present_paths:
        return result

    process = subprocess.Popen(
        ["git", "cat-file", "--batch"],
        cwd=ROOT,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
    )
    object_input = "".join(f"{object_ids[path]}\n" for path in present_paths).encode()
    output, _ = process.communicate(object_input)
    position = 0
    for path in present_paths:
        header_end = output.find(b"\n", position)
        if header_end < 0:
            raise RuntimeError(f"git cat-file returned no object for {path}")
        fields = output[position:header_end].split()
        if len(fields) != 3 or fields[1] != b"blob":
            raise RuntimeError(f"git cat-file returned an invalid object header for {path}")
        size = int(fields[2])
        position = header_end + 1
        data = output[position : position + size]
        position += size
        if len(data) != size or output[position : position + 1] != b"\n":
            raise RuntimeError(f"git cat-file returned truncated content for {path}")
        position += 1
        result[path] = data.count(b"\n") + int(bool(data) and not data.endswith(b"\n"))

    if process.returncode != 0:
        raise RuntimeError("git cat-file failed while reading the selected revision")
    return result


def worktree_line_counts(target_paths: set[str]) -> dict[str, int | None]:
    result: dict[str, int | None] = {}
    for path in target_paths:
        target = ROOT / Path(path)
        if not target.is_file():
            result[path] = None
            continue
        data = target.read_bytes()
        result[path] = data.count(b"\n") + int(bool(data) and not data.endswith(b"\n"))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--ref", help="Git revision whose tracked files should be checked")
    group.add_argument(
        "--worktree",
        action="store_true",
        help="check current worktree files (including staged, unstaged, and untracked changes)",
    )
    args = parser.parse_args()

    sources = markdown_sources()
    if not sources:
        print(f"No IEEE 1516.1-2025 Markdown files found under {DESIGN}", file=sys.stderr)
        return 2

    references = collect_references(sources)
    target_paths = {path for path, _, _ in references}
    try:
        if args.worktree:
            revision_name = "worktree"
            line_counts = worktree_line_counts(target_paths)
        else:
            revision_name = args.ref or "HEAD"
            line_counts = git_tree_line_counts(revision_name, target_paths)
    except (OSError, subprocess.CalledProcessError, RuntimeError) as error:
        selected_revision = locals().get("revision_name", "selected revision")
        print(f"Could not inspect {selected_revision}: {error}", file=sys.stderr)
        return 2

    failures: dict[str, list[tuple[int, str]]] = defaultdict(list)
    for path, line, source in references:
        available = line_counts[path]
        if available is None or line > available:
            failures[path].append((line, source))

    print(
        f"Checked {len(references)} unique path/line/guide references across "
        f"{len(sources)} IEEE 1516.1-2025 Markdown files against {revision_name}: "
        f"{len(references) - sum(len(items) for items in failures.values())} resolved, "
        f"{sum(len(items) for items in failures.values())} unresolved in {len(failures)} target files."
    )
    for path in sorted(failures):
        available = line_counts[path]
        details = ", ".join(
            f"L{line} ({source})" for line, source in sorted(failures[path])
        )
        if available is None:
            reason = "target missing"
        else:
            reason = f"target has {available} lines"
        print(f"  {path}: {reason}; {details}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
