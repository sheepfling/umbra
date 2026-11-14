"""Minimal private-directory permissions for RID-backed TCK adapters."""

from __future__ import annotations

import csv
import os
import shutil
import subprocess
from pathlib import Path


def protect_directory(directory: Path) -> None:
    if os.name != "nt":
        os.chmod(directory, 0o700)
        return

    whoami = shutil.which("whoami.exe") or shutil.which("whoami")
    icacls = shutil.which("icacls.exe") or shutil.which("icacls")
    if not whoami or not icacls:
        raise RuntimeError("Windows RID adapter requires whoami and icacls.")
    identity = subprocess.run(
        [whoami, "/user", "/fo", "csv", "/nh"],
        check=True,
        capture_output=True,
        text=True,
    )
    rows = list(csv.reader(identity.stdout.splitlines()))
    current_sid = next(
        (row[1] for row in rows if len(row) > 1 and row[1].startswith("S-1-")),
        None,
    )
    if current_sid is None:
        raise RuntimeError("Could not determine the current Windows account SID.")
    subprocess.run(
        [
            icacls,
            str(directory),
            "/inheritance:r",
            "/grant:r",
            f"*{current_sid}:(OI)(CI)F",
            "*S-1-5-18:(OI)(CI)F",
            "*S-1-5-32-544:(OI)(CI)F",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        [icacls, str(directory), "/remove", "*S-1-3-4"],
        check=True,
        capture_output=True,
        text=True,
    )
