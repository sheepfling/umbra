"""Run the federation-authorization TCK case with a private custom RID profile."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

from rid_security import protect_directory


PROFILES = {
    "TckFederationGate": "tck-authorizer-federation",
    "TckDestroyGate": "tck-authorizer-destroy",
    "TckJoinGate": "tck-authorizer-join",
    "TckNoCredentialsGate": "tck-authorizer-no-credentials",
}


def main() -> int:
    if len(sys.argv) < 5 or sys.argv[3] != "--":
        print(
            "usage: run_authorizer_federation_decision.py <tck-executable> <service-name> -- <tck-arguments>",
            file=sys.stderr,
        )
        return 2

    service_name = sys.argv[2]
    profile = PROFILES.get(service_name)
    if profile is None:
        print(f"unsupported TCK Authorizer service: {service_name}", file=sys.stderr)
        return 2

    provider_library = os.environ.get("UMBRA_AUTHORIZER_PROVIDER_LIBRARY", "")
    if not provider_library:
        print(
            "UMBRA_AUTHORIZER_PROVIDER_LIBRARY must name the adapter's custom authorizer library",
            file=sys.stderr,
        )
        return 2

    executable = sys.argv[1]
    arguments = sys.argv[4:]
    with tempfile.TemporaryDirectory(
        prefix="umbra-tck-authorizer-federation-"
    ) as raw_dir:
        directory = Path(raw_dir)
        protect_directory(directory)
        rid_path = directory / "umbra.rid"
        rid_path.write_text(
            f"[authorization.{profile}]\n"
            f"service={service_name}\n",
            encoding="ascii",
        )
        if os.name != "nt":
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
