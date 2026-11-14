"""Run the Connect authorization TCK case with a private Umbra RID profile."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

from rid_security import protect_directory


PASSWORD = "tck-authorizer-password"
PROFILE = "tck-authorizer"


def main() -> int:
    if len(sys.argv) < 4 or sys.argv[2] != "--":
        print(
            "usage: run_authorizer_connect_decision.py <tck-executable> -- <tck-arguments>",
            file=sys.stderr,
        )
        return 2

    executable = sys.argv[1]
    arguments = sys.argv[3:]
    with tempfile.TemporaryDirectory(prefix="umbra-tck-authorizer-") as raw_dir:
        directory = Path(raw_dir)
        protect_directory(directory)
        password_path = directory / "global-password.txt"
        password_path.write_text(PASSWORD, encoding="ascii")
        rid_path = directory / "umbra.rid"
        rid_path.write_text(
            f"[authorization.{PROFILE}]\n"
            "service=HLAauthorizer\n"
            "globalPasswordFile=global-password.txt\n",
            encoding="ascii",
        )
        if os.name != "nt":
            os.chmod(password_path, 0o600)
            os.chmod(rid_path, 0o600)

        environment = os.environ.copy()
        environment["UMBRA_RTI_RID_FILE"] = str(rid_path)
        completed = subprocess.run(
            [executable, *arguments],
            cwd=Path.cwd(),
            env=environment,
            check=False,
        )
        return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
