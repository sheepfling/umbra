"""Run Umbra's repeatable, pipeline-agnostic CI profiles locally.

CI services should only prepare a supported environment and invoke this file.
The same profile commands are intentionally usable from a developer checkout,
which keeps the build, test, and failure behavior identical on a laptop and in
GitHub Actions, Azure DevOps, or another runner.
"""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
STAGES = ("configure", "build", "test")


@dataclass(frozen=True)
class Profile:
    """The matching configure, build, and test presets for one CI lane."""

    description: str
    configure_preset: str
    build_preset: str
    test_preset: str


PROFILES: dict[str, Profile] = {
    "native": Profile(
        "Core native build, smoke tests, contracts, and package smoke.",
        "windows-default",
        "windows-default-debug",
        "windows-default-debug",
    ),
    "native-catch2": Profile(
        "Focused native Catch2 behavior tests; may fetch Catch2.",
        "windows-catch2",
        "windows-catch2-debug",
        "windows-catch2-debug",
    ),
    "native-fom": Profile(
        "FOM validation and XML/XSD tests; may fetch Catch2 and libxml2.",
        "windows-fom",
        "windows-fom-debug",
        "windows-fom-debug",
    ),
    "native-fom-services": Profile(
        "Non-installable embedded federation-management development profile.",
        "windows-fom-services",
        "windows-fom-services-debug",
        "windows-fom-services-debug",
    ),
}


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "profile",
        nargs="?",
        choices=sorted(PROFILES),
        help="CI profile to run; use --list-profiles to inspect choices.",
    )
    parser.add_argument(
        "--stage",
        choices=("all", *STAGES),
        default="all",
        help="Run all stages (default) or rerun one completed stage.",
    )
    parser.add_argument(
        "--cmake",
        default="cmake",
        help="CMake executable used for configure and build stages (default: cmake).",
    )
    parser.add_argument(
        "--ctest",
        default="ctest",
        help="CTest executable used for the test stage (default: ctest).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the commands without running them.",
    )
    parser.add_argument(
        "--list-profiles",
        action="store_true",
        help="List available profiles and exit.",
    )
    arguments = parser.parse_args()
    if not arguments.list_profiles and arguments.profile is None:
        parser.error("profile is required unless --list-profiles is used")
    return arguments


def command_display(command: Sequence[str]) -> str:
    return subprocess.list2cmdline(command) if os.name == "nt" else shlex.join(command)


def run_command(command: Sequence[str], *, dry_run: bool) -> None:
    print(f"+ {command_display(command)}", flush=True)
    if not dry_run:
        subprocess.run(command, cwd=REPOSITORY_ROOT, check=True)


def list_profiles() -> None:
    for name, profile in PROFILES.items():
        print(f"{name}: {profile.description}")
        print(
            f"  configure={profile.configure_preset} "
            f"build={profile.build_preset} test={profile.test_preset}"
        )


def stage_commands(arguments: argparse.Namespace, profile: Profile) -> dict[str, list[str]]:
    return {
        "configure": [arguments.cmake, "--preset", profile.configure_preset],
        "build": [arguments.cmake, "--build", "--preset", profile.build_preset],
        "test": [arguments.ctest, "--preset", profile.test_preset],
    }


def main() -> int:
    arguments = parse_arguments()
    if sys.version_info < (3, 11):
        print("Umbra CI profiles require Python 3.11 or newer.", file=sys.stderr)
        return 2
    if arguments.list_profiles:
        list_profiles()
        return 0

    profile = PROFILES[arguments.profile]
    commands = stage_commands(arguments, profile)
    stages = STAGES if arguments.stage == "all" else (arguments.stage,)
    print(f"Running CI profile '{arguments.profile}' from {REPOSITORY_ROOT}", flush=True)
    try:
        for stage in stages:
            run_command(commands[stage], dry_run=arguments.dry_run)
    except FileNotFoundError as error:
        print(f"Cannot run {error.filename!r}; install CMake and ensure it is on PATH.", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as error:
        print(
            f"CI profile '{arguments.profile}' failed during the {stage} stage "
            f"with exit code {error.returncode}.",
            file=sys.stderr,
        )
        return error.returncode or 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
