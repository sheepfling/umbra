"""Run public TLS 1.3 transport and address/port configuration cases."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import tempfile
import time
from pathlib import Path


SCENARIOS = {
    "cpp-tck.tls13-transport-support",
    "cpp-tck.tls13-server-certificate-validation",
    "cpp-tck.tls13-server-certificate-validation-rejects-untrusted",
    "cpp-tck.tls13-default-port-support",
    "cpp-tck.tls13-alternate-address-port-configuration",
    "cpp-tck.secure-websocket-transport-support",
}
DEFAULT_PORT_SCENARIO = "cpp-tck.tls13-default-port-support"
ALTERNATE_PORT_SCENARIO = "cpp-tck.tls13-alternate-address-port-configuration"
SECURE_WEBSOCKET_SCENARIO = "cpp-tck.secure-websocket-transport-support"
ADAPTER_SETTINGS = "tlsCertificateValidation=server"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, required=True)
    parser.add_argument("--mim-fom", type=Path, required=True)
    parser.add_argument("--time-implementation", required=True)
    parser.add_argument("--scenario", choices=sorted(SCENARIOS), required=True)
    parser.add_argument(
        "--callback-model", choices=("evoked", "immediate"), required=True
    )
    parser.add_argument("--provider-id", default="installed-package")
    parser.add_argument("--timeout-s", type=float, default=30.0)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--junit", type=Path, required=True)
    return parser.parse_args()


def wait_for_port(
    directory: Path,
    timeout_s: float,
    fixture: subprocess.Popen[bytes],
    fixture_mode: str,
) -> int:
    port_path = directory / "port.txt"
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            port = int(port_path.read_text(encoding="ascii").strip())
        except (FileNotFoundError, OSError, ValueError):
            port = 0
        if 0 < port <= 65535:
            return port
        if fixture.poll() is not None:
            error_path = directory / f"{fixture_mode}.error"
            detail = (
                error_path.read_text(encoding="utf-8").strip()
                if error_path.exists()
                else "no fixture error marker was written"
            )
            raise RuntimeError(
                "TLS process fixture exited before publishing its port "
                f"(exit={fixture.returncode}; {detail})"
            )
        time.sleep(0.025)
    raise RuntimeError("TLS process fixture did not publish a listening port")


def add_configured_openssl_runtime(
    environment: dict[str, str], process_fixture: Path
) -> None:
    cache_path = process_fixture.resolve().parent / "CMakeCache.txt"
    if not cache_path.exists():
        return
    for line in cache_path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("OPENSSL_ROOT_DIR:") or "=" not in line:
            continue
        openssl_root = Path(line.split("=", 1)[1].strip())
        runtime_directory = openssl_root / "bin"
        if runtime_directory.is_dir():
            current_path = environment.get("PATH", "")
            environment["PATH"] = os.pathsep.join(
                value
                for value in (str(runtime_directory), current_path)
                if value
            )
        return


def run() -> int:
    arguments = parse_arguments()
    arguments.results.parent.mkdir(parents=True, exist_ok=True)
    arguments.junit.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="cpp-tck-tls-transport-",
        dir=arguments.results.parent,
    ) as temporary_directory:
        directory = Path(temporary_directory)
        model_results = directory / "results.json"
        model_junit = directory / "results.xml"
        fixture: subprocess.Popen[bytes] | None = None
        environment = os.environ.copy()
        add_configured_openssl_runtime(environment, arguments.process_fixture)
        if arguments.scenario == DEFAULT_PORT_SCENARIO:
            fixture_mode = "public-server-tls-default-port-transport"
            completion_marker = "tls-default-port-tck-complete.ok"
        elif arguments.scenario == ALTERNATE_PORT_SCENARIO:
            fixture_mode = "public-server-tls-alternate-port-transport"
            completion_marker = "tls-alternate-port-tck-complete.ok"
        elif arguments.scenario == SECURE_WEBSOCKET_SCENARIO:
            fixture_mode = "public-server-secure-websocket-transport"
            completion_marker = "secure-websocket-tck-complete.ok"
        else:
            fixture_mode = "public-server-tls-server-validation-transport"
            completion_marker = "tls-server-validation-tck-complete.ok"
        try:
            fixture = subprocess.Popen(
                [
                    str(arguments.process_fixture),
                    fixture_mode,
                    str(directory),
                ],
                env=environment,
            )
            port = wait_for_port(
                directory, arguments.timeout_s, fixture, fixture_mode
            )
            if arguments.scenario == DEFAULT_PORT_SCENARIO and port != 15165:
                raise RuntimeError(
                    f"TLS default-port fixture listened on {port}, expected 15165"
                )
            if arguments.scenario == ALTERNATE_PORT_SCENARIO and port != 15166:
                raise RuntimeError(
                    f"TLS alternate-port fixture listened on {port}, expected 15166"
                )
            if arguments.scenario == DEFAULT_PORT_SCENARIO:
                rti_address = "tls://127.0.0.1"
            elif arguments.scenario == SECURE_WEBSOCKET_SCENARIO:
                rti_address = f"wss://127.0.0.1:{port}/hla"
            else:
                rti_address = f"tls://127.0.0.1:{port}"
            command = [
                str(arguments.tck_executable),
                "--fom",
                str(arguments.fom_path),
                "--mim-fom",
                str(arguments.mim_fom),
                "--time-implementation",
                arguments.time_implementation,
                "--scenario",
                arguments.scenario,
                "--callback-model",
                arguments.callback_model,
                "--provider-id",
                arguments.provider_id,
                "--rti-address",
                rti_address,
                "--timeout-ms",
                str(max(int(arguments.timeout_s * 1000), 3000)),
                "--results",
                str(model_results),
                "--junit",
                str(model_junit),
            ]
            if arguments.scenario not in {
                DEFAULT_PORT_SCENARIO,
                ALTERNATE_PORT_SCENARIO,
            }:
                command.extend(["--additional-settings", ADAPTER_SETTINGS])
            client_environment = environment.copy()
            trust_certificate = (
                "untrusted-ip.crt"
                if arguments.scenario.endswith("rejects-untrusted")
                else "server-ip.crt"
            )
            client_environment["UMBRA_RTI_TLS_TRUST_STORE_FILE"] = str(
                directory / "tls-credentials" / trust_certificate
            )
            runtime_directory = str(arguments.process_fixture.resolve().parent)
            existing_path = client_environment.get("PATH", "")
            client_environment["PATH"] = os.pathsep.join(
                value for value in (existing_path, runtime_directory) if value
            )
            subprocess.run(
                command,
                check=True,
                timeout=arguments.timeout_s,
                env=client_environment,
            )

            (directory / completion_marker).write_text(
                "complete\n", encoding="ascii"
            )
            if fixture.wait(timeout=arguments.timeout_s) != 0:
                raise RuntimeError("The TLS process fixture failed")

            evidence = json.loads(model_results.read_text(encoding="utf-8"))
            matched = [
                result
                for result in evidence.get("results", [])
                if result.get("id") == arguments.scenario
            ]
            if len(matched) != 1:
                raise RuntimeError(
                    "The TLS TCK result did not contain exactly one scenario record"
                )
            matched[0]["adapter_evidence"] = {
                "transport": (
                    "TLS 1.3 over WebSocket"
                    if arguments.scenario == SECURE_WEBSOCKET_SCENARIO
                    else "TLS 1.3 over TCP"
                ),
                "endpoint": (
                    "tls://127.0.0.1 (default port omitted)"
                    if arguments.scenario == DEFAULT_PORT_SCENARIO
                    else f"wss://127.0.0.1:{port}/hla"
                    if arguments.scenario == SECURE_WEBSOCKET_SCENARIO
                    else f"tls://127.0.0.1:{port} (explicit alternate port)"
                    if arguments.scenario == ALTERNATE_PORT_SCENARIO
                    else "127.0.0.1:<ephemeral-port>"
                ),
                "listener_port": port,
                "server_certificate": (
                    "self-signed; subjectAltName=IP:127.0.0.1"
                ),
                "trust_store": (
                    "matching server certificate"
                    if trust_certificate == "server-ip.crt"
                    else "different self-signed certificate with matching endpoint identity"
                ),
                "tls_mode_selector": (
                    None
                    if arguments.scenario
                    in {DEFAULT_PORT_SCENARIO, ALTERNATE_PORT_SCENARIO}
                    else ADAPTER_SETTINGS
                ),
                "tls_mode_selector_is_provider_specific": (
                    arguments.scenario
                    not in {DEFAULT_PORT_SCENARIO, ALTERNATE_PORT_SCENARIO}
                ),
            }
            if arguments.scenario == DEFAULT_PORT_SCENARIO:
                matched[0]["adapter_evidence"]["normative_default_port"] = 15165
                matched[0]["adapter_evidence"]["address_contains_explicit_port"] = False
            if arguments.scenario == ALTERNATE_PORT_SCENARIO:
                matched[0]["adapter_evidence"]["configured_port"] = 15166
                matched[0]["adapter_evidence"]["address_contains_explicit_port"] = True
                matched[0]["adapter_evidence"]["address_port_matches_listener"] = True
                matched[0]["adapter_evidence"]["address_port_is_non_default"] = True
            arguments.results.write_text(
                json.dumps(evidence, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            arguments.junit.write_bytes(model_junit.read_bytes())
        finally:
            if fixture is not None and fixture.poll() is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait()
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
