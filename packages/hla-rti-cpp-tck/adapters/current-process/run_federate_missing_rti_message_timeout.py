"""Run the provider-specific missing-RTI-message timeout TCK case."""

from __future__ import annotations

import argparse
import os
import subprocess
import time
from pathlib import Path


SCENARIO = "cpp-tck.federate-missing-rti-message-timeout-recovers"
ADDITIONAL_SETTINGS = (
    "FED_INT_HEART=0.05;FED_TIMEOUT_HEART=0.4;FED_TIMEOUT_RECONNECT=4"
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-fom", type=Path, required=True)
    parser.add_argument("--time-implementation", required=True)
    parser.add_argument("--callback-model", choices=("evoked",), required=True)
    parser.add_argument("--provider-id", default="current-process")
    parser.add_argument("--timeout-s", type=float, default=30.0)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--junit", type=Path, required=True)
    return parser.parse_args()


def wait_for_port(
    directory: Path,
    timeout_s: float,
    fixture: subprocess.Popen[bytes],
) -> int:
    port_path = directory / "port.txt"
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(port_path.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if fixture.poll() is not None:
            raise RuntimeError(
                "The missing-RTI-message process fixture exited before publishing its port"
            )
        time.sleep(0.025)
    raise RuntimeError(
        "The missing-RTI-message process fixture did not publish its port"
    )


def run() -> int:
    arguments = parse_arguments()
    if arguments.timeout_s <= 0:
        raise ValueError("--timeout-s must be positive")
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    directory = arguments.results.parent
    fixture: subprocess.Popen[bytes] | None = None
    try:
        fixture = subprocess.Popen(
            [
                str(arguments.process_fixture),
                "public-server-federate-missing-rti-message-timeout",
                str(directory),
            ]
        )
        port = wait_for_port(directory, arguments.timeout_s, fixture)
        command = [
            str(arguments.tck_executable),
            "--fom",
            str(arguments.fom_path),
            "--mim-fom",
            str(arguments.mim_fom),
            "--time-implementation",
            arguments.time_implementation,
            "--scenario",
            SCENARIO,
            "--callback-model",
            arguments.callback_model,
            "--provider-id",
            arguments.provider_id,
            "--rti-address",
            f"tcp://127.0.0.1:{port}",
            "--additional-settings",
            ADDITIONAL_SETTINGS,
            "--federate-missing-rti-message-timeout-server-managed",
            "--timeout-ms",
            "10000",
            "--results",
            str(arguments.results),
            "--junit",
            str(arguments.junit),
        ]
        environment = os.environ.copy()
        runtime_directory = str(arguments.process_fixture.resolve().parent)
        environment["PATH"] = runtime_directory + os.pathsep + environment.get(
            "PATH", ""
        )
        subprocess.run(
            command,
            check=True,
            timeout=arguments.timeout_s,
            env=environment,
        )
        required_markers = (
            ("federate-missing-rti-message-timeout-reply-suppressed.ok", "suppressed"),
            ("federate-missing-rti-message-timeout-resumed.ok", "resumed"),
        )
        for marker_name, expected in required_markers:
            marker = directory / marker_name
            if (
                not marker.is_file()
                or marker.read_text(encoding="utf-8").strip() != expected
            ):
                raise RuntimeError(
                    f"The missing-RTI-message fixture did not publish {expected!r}"
                )
        (directory / "federate-missing-rti-message-timeout-complete.ok").write_text(
            "complete\n", encoding="utf-8"
        )
        if fixture.wait(timeout=arguments.timeout_s) != 0:
            raise RuntimeError("The missing-RTI-message process fixture failed")
        return 0
    finally:
        if fixture is not None and fixture.poll() is None:
            fixture.terminate()
            try:
                fixture.wait(timeout=5)
            except subprocess.TimeoutExpired:
                fixture.kill()
                fixture.wait()


if __name__ == "__main__":
    raise SystemExit(run())
