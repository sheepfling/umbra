"""Run Java TLS 1.3 transport scenarios against process-backed TLS fixtures."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from types import ModuleType

ROOT = Path(__file__).resolve().parents[1]
JAVA_RUNNER = ROOT / "tools" / "run_java_tck.py"
ADAPTER_DIRECTORY = (
    ROOT / "packages" / "hla-rti-cpp-tck" / "adapters" / "current-process"
)
TRANSPORT_SCENARIO = "java-tck.tls13-transport-support"
ENCRYPTION_ONLY_SCENARIO = "java-tck.tls13-encryption-only-mode"
SERVER_CERTIFICATE_VALIDATION_SCENARIO = (
    "java-tck.tls13-server-certificate-validation"
)
UNTRUSTED_SERVER_SCENARIO = (
    "java-tck.tls13-server-certificate-validation-rejects-untrusted"
)
DEFAULT_PORT_SCENARIO = "java-tck.tls13-default-port-support"
ALTERNATE_ADDRESS_PORT_SCENARIO = (
    "java-tck.tls13-alternate-address-port-configuration"
)
SECURE_WEBSOCKET_SCENARIO = "java-tck.secure-websocket-transport-support"
CALLBACK_MODELS = ("HLA_EVOKED", "HLA_IMMEDIATE")
TLS_SCENARIOS = {
    TRANSPORT_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_server_validation.py",
        "adapter_module": "cpp_tls_server_transport_adapter",
        "fixture_mode": "public-server-tls-server-validation-transport",
        "complete_marker": "tls-server-validation-tck-complete.ok",
        "additional_settings": "tlsCertificateValidation=server",
        "trust_certificate": "server-ip.crt",
        "server_certificate": "self-signed; subjectAltName=IP:127.0.0.1",
        "trust_store": "matching adapter-provided server certificate",
    },
    ENCRYPTION_ONLY_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_encryption_only.py",
        "adapter_module": "cpp_tls_encryption_only_adapter",
        "fixture_mode": "public-server-tls-encryption-only-transport",
        "complete_marker": "tls-tck-complete.ok",
        "additional_settings": "tlsCertificateValidation=none",
        "trust_certificate": None,
        "server_certificate": "self-signed; subjectAltName=DNS:localhost",
        "trust_store": "configured path deliberately does not exist",
        "endpoint_name_mismatch": True,
    },
    SERVER_CERTIFICATE_VALIDATION_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_server_validation.py",
        "adapter_module": "cpp_tls_server_transport_adapter",
        "fixture_mode": "public-server-tls-server-validation-transport",
        "complete_marker": "tls-server-validation-tck-complete.ok",
        "additional_settings": "tlsCertificateValidation=server",
        "trust_certificate": "server-ip.crt",
        "server_certificate": "self-signed; subjectAltName=IP:127.0.0.1",
        "trust_store": "matching adapter-provided server certificate",
        "certificate_validation_mode": "server",
    },
    UNTRUSTED_SERVER_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_server_validation.py",
        "adapter_module": "cpp_tls_server_transport_adapter",
        "fixture_mode": "public-server-tls-server-validation-transport",
        "complete_marker": "tls-server-validation-tck-complete.ok",
        "additional_settings": "tlsCertificateValidation=server",
        "trust_certificate": "untrusted-ip.crt",
        "server_certificate": "self-signed; subjectAltName=IP:127.0.0.1",
        "trust_store": "different self-signed certificate with matching endpoint identity",
        "certificate_validation_mode": "server",
        "expected_connection_failure": True,
    },
    DEFAULT_PORT_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_server_validation.py",
        "adapter_module": "cpp_tls_server_transport_adapter",
        "fixture_mode": "public-server-tls-default-port-transport",
        "complete_marker": "tls-default-port-tck-complete.ok",
        "additional_settings": "",
        "trust_certificate": "server-ip.crt",
        "server_certificate": "self-signed; subjectAltName=IP:127.0.0.1",
        "trust_store": "matching adapter-provided server certificate",
        "default_port": 15165,
    },
    ALTERNATE_ADDRESS_PORT_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_server_validation.py",
        "adapter_module": "cpp_tls_server_transport_adapter",
        "fixture_mode": "public-server-tls-alternate-port-transport",
        "complete_marker": "tls-alternate-port-tck-complete.ok",
        "additional_settings": "",
        "trust_certificate": "server-ip.crt",
        "server_certificate": "self-signed; subjectAltName=IP:127.0.0.1",
        "trust_store": "matching adapter-provided server certificate",
        "expected_port": 15166,
    },
    SECURE_WEBSOCKET_SCENARIO: {
        "adapter_path": ADAPTER_DIRECTORY / "run_tls_server_validation.py",
        "adapter_module": "cpp_tls_server_transport_adapter",
        "fixture_mode": "public-server-secure-websocket-transport",
        "complete_marker": "secure-websocket-tck-complete.ok",
        "additional_settings": "tlsCertificateValidation=server",
        "trust_certificate": "server-ip.crt",
        "server_certificate": "self-signed; subjectAltName=IP:127.0.0.1",
        "trust_store": "matching adapter-provided server certificate",
        "secure_websocket": True,
        "certificate_validation_mode": "server",
    },
}
DEFAULT_FOM = ROOT / "packages" / "hla-rti-java-tck" / "fom" / "p0-tck-without-switches.xml"
DEFAULT_MIM = (
    ROOT
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)


def _load_adapter(adapter_path: Path, module_name: str) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        module_name, adapter_path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"could not load TLS process-fixture helper {adapter_path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def _root_path(value: str | Path) -> Path:
    path = Path(value)
    return path.resolve() if path.is_absolute() else (ROOT / path).resolve()


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--scenario",
        choices=sorted(TLS_SCENARIOS),
        default=TRANSPORT_SCENARIO,
        help="Java TLS TCK scenario to run",
    )
    parser.add_argument("--api-jar", required=True, help="pinned official Java API JAR")
    parser.add_argument(
        "--provider-jar", action="append", required=True, help="provider JAR; repeat as needed"
    )
    parser.add_argument(
        "--dependency-jar", action="append", default=[], help="provider dependency JAR"
    )
    parser.add_argument("--process-fixture", required=True, help="trusted TLS process fixture")
    parser.add_argument("--fom-path", default=str(DEFAULT_FOM))
    parser.add_argument("--mim-path", default=str(DEFAULT_MIM))
    parser.add_argument("--provider-id", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--classes-directory")
    parser.add_argument("--java")
    parser.add_argument("--javac")
    parser.add_argument("--jni-library-path")
    parser.add_argument("--prepend-path", action="append", default=[])
    parser.add_argument("--jvm-argument", action="append", default=[])
    parser.add_argument("--timeout-s", type=float, default=60.0)
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    return parser.parse_args()


def _build_tck(arguments: argparse.Namespace, classes_directory: Path) -> None:
    command = [
        sys.executable,
        str(JAVA_RUNNER),
        "build",
        "--api-jar",
        str(_root_path(arguments.api_jar)),
        "--output-directory",
        str(classes_directory),
    ]
    if arguments.javac:
        command.extend(["--javac", arguments.javac])
    completed = subprocess.run(command, cwd=ROOT, check=False, timeout=180)
    if completed.returncode != 0:
        raise RuntimeError(f"Java TCK compilation failed with exit {completed.returncode}")


def _java_command(
    arguments: argparse.Namespace,
    callback_model: str,
    rti_address: str,
    classes_directory: Path,
    model_directory: Path,
) -> list[str]:
    command = [
        sys.executable,
        str(JAVA_RUNNER),
        "run",
        "--api-jar",
        str(_root_path(arguments.api_jar)),
        "--fom-path",
        str(_root_path(arguments.fom_path)),
        "--mim-path",
        str(_root_path(arguments.mim_path)),
        "--provider-id",
        arguments.provider_id,
        "--scenario",
        arguments.scenario,
        "--run-unverified",
        "--classes-directory",
        str(classes_directory),
        "--results-path",
        str(model_directory / "results.json"),
        "--junit-path",
        str(model_directory / "results.xml"),
        "--skip-build",
    ]
    for provider_jar in arguments.provider_jar:
        command.extend(["--provider-jar", str(_root_path(provider_jar))])
    for dependency_jar in arguments.dependency_jar:
        command.extend(["--dependency-jar", str(_root_path(dependency_jar))])
    if arguments.java:
        command.extend(["--java", arguments.java])
    if arguments.jni_library_path:
        command.append(
            "--jvm-argument=-Djava.library.path="
            + str(_root_path(arguments.jni_library_path))
        )
    command.extend(
        [
            "--jvm-argument=-Dhla.rti.tck.callbackModel=" + callback_model,
            "--jvm-argument=-Dhla.rti.tck.rtiAddress=" + rti_address,
            "--jvm-argument=-Dhla.rti.tck.tlsTransportTimeoutMillis="
            + str(int(arguments.timeout_s * 1000)),
            "--jvm-argument=-Dhla.rti.tck.webSocketTransportTimeoutMillis="
            + str(int(arguments.timeout_s * 1000)),
        ]
    )
    additional_settings = TLS_SCENARIOS[arguments.scenario]["additional_settings"]
    if additional_settings:
        command.append(
            "--jvm-argument=-Dhla.rti.tck.additionalSettings="
            + additional_settings
        )
    command.extend("--jvm-argument=" + value for value in arguments.jvm_argument)
    return command


def _terminate(process: subprocess.Popen[bytes]) -> None:
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def _run_model(
    arguments: argparse.Namespace,
    callback_model: str,
    classes_directory: Path,
    environment: dict[str, str],
) -> dict[str, object]:
    model_directory = _root_path(arguments.output_directory) / callback_model.lower()
    model_directory.mkdir(parents=True, exist_ok=True)
    scenario = TLS_SCENARIOS[arguments.scenario]
    adapter = _load_adapter(
        scenario["adapter_path"], scenario["adapter_module"]
    )
    stale_names = ("port.txt", scenario["complete_marker"])
    stale = [name for name in stale_names if (model_directory / name).exists()]
    if stale:
        raise RuntimeError("refusing stale TLS fixture markers: " + ", ".join(stale))
    for name in ("results.json", "results.xml", "tls-evidence.json"):
        (model_directory / name).unlink(missing_ok=True)

    fixture_path = _root_path(arguments.process_fixture)
    fixture = subprocess.Popen(
        [
            str(fixture_path),
            scenario["fixture_mode"],
            str(model_directory),
        ],
        cwd=ROOT,
        env=environment,
    )
    try:
        port = adapter.wait_for_port(
            model_directory,
            arguments.timeout_s,
            fixture,
            scenario["fixture_mode"],
        )
        expected_port = scenario.get("default_port", scenario.get("expected_port"))
        if expected_port and port != expected_port:
            raise RuntimeError(
                f"TLS fixture listened on {port}, expected {expected_port}"
            )
        if scenario.get("secure_websocket"):
            rti_address = f"wss://127.0.0.1:{port}/hla"
        elif scenario.get("default_port"):
            rti_address = "tls://127.0.0.1"
        else:
            rti_address = f"tls://127.0.0.1:{port}"
        client_environment = environment.copy()
        trust_certificate = scenario["trust_certificate"]
        trust_store = (
            model_directory / "tls-credentials" / trust_certificate
            if trust_certificate
            else model_directory / "unavailable-trust-store.pem"
        )
        client_environment["UMBRA_RTI_TLS_TRUST_STORE_FILE"] = str(trust_store)
        command = _java_command(
            arguments,
            callback_model,
            rti_address,
            classes_directory,
            model_directory,
        )
        try:
            completed = subprocess.run(
                command,
                cwd=ROOT,
                env=client_environment,
                check=False,
                timeout=arguments.timeout_s,
            )
        except subprocess.TimeoutExpired as error:
            raise TimeoutError(f"Java TLS TCK exceeded {arguments.timeout_s:g} seconds") from error
        if completed.returncode != 0:
            raise RuntimeError(f"Java TLS TCK exited with {completed.returncode}")

        (model_directory / scenario["complete_marker"]).write_text(
            "complete\n", encoding="ascii"
        )
        fixture_status = fixture.wait(timeout=arguments.timeout_s)
        if fixture_status != 0:
            raise RuntimeError(f"TLS process fixture exited with code {fixture_status}")

        java_results = json.loads(
            (model_directory / "results.json").read_text(encoding="utf-8")
        )
        if scenario.get("secure_websocket"):
            transport = "TLS 1.3 over WebSocket"
            endpoint = f"wss://127.0.0.1:{port}/hla"
        else:
            transport = "TLS 1.3 over TCP"
            endpoint = (
                "tls://127.0.0.1 (default port omitted)"
                if scenario.get("default_port")
                else f"tls://127.0.0.1:{port} (explicit alternate port)"
                if scenario.get("expected_port")
                else "127.0.0.1:<ephemeral-port>"
            )
        tls_evidence: dict[str, object] = {
            "scenario": arguments.scenario,
            "callback_model": callback_model,
            "transport": transport,
            "endpoint": endpoint,
            "listener_port": port,
            "server_certificate": scenario["server_certificate"],
            "trust_store": scenario["trust_store"],
        }
        if scenario.get("secure_websocket"):
            tls_evidence["websocket_resource"] = "/hla"
            tls_evidence["tls_protocol"] = "TLS 1.3"
        if scenario["additional_settings"]:
            tls_evidence["tls_mode_selector"] = scenario["additional_settings"]
            tls_evidence["tls_mode_selector_is_provider_specific"] = True
        if scenario.get("default_port"):
            tls_evidence["normative_default_port"] = scenario["default_port"]
            tls_evidence["address_contains_explicit_port"] = False
        if scenario.get("expected_port"):
            tls_evidence["configured_port"] = scenario["expected_port"]
            tls_evidence["address_contains_explicit_port"] = True
            tls_evidence["address_port_matches_listener"] = True
            tls_evidence["address_port_is_non_default"] = True
        if scenario.get("expected_connection_failure"):
            tls_evidence["expected_connect_failure"] = "ConnectionFailed"
            tls_evidence["connection_rejected"] = True
        else:
            tls_evidence["connect_address_used"] = True
            tls_evidence["list_federation_executions_reported"] = True
        if scenario.get("endpoint_name_mismatch"):
            tls_evidence["endpoint_name_mismatch"] = True
        if scenario.get("certificate_validation_mode"):
            tls_evidence["certificate_validation_mode"] = scenario[
                "certificate_validation_mode"
            ]
        (model_directory / "tls-evidence.json").write_text(
            json.dumps(tls_evidence, indent=2) + "\n", encoding="utf-8"
        )
        return {"java_results": java_results, "tls_evidence": tls_evidence}
    finally:
        _terminate(fixture)


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout_s <= 0:
        raise ValueError("timeout must be positive")
    output_directory = _root_path(arguments.output_directory)
    output_directory.mkdir(parents=True, exist_ok=True)
    classes_directory = (
        _root_path(arguments.classes_directory)
        if arguments.classes_directory
        else output_directory / "classes"
    )
    _build_tck(arguments, classes_directory)

    fixture_path = _root_path(arguments.process_fixture)
    environment = os.environ.copy()
    prepend = [str(_root_path(path)) for path in arguments.prepend_path]
    prepend.append(str(fixture_path.parent))
    environment["PATH"] = os.pathsep.join([*prepend, environment.get("PATH", "")])
    scenario = TLS_SCENARIOS[arguments.scenario]
    adapter = _load_adapter(scenario["adapter_path"], scenario["adapter_module"])
    adapter.add_configured_openssl_runtime(environment, fixture_path)

    models = (
        CALLBACK_MODELS
        if arguments.callback_model == "both"
        else ("HLA_EVOKED",)
        if arguments.callback_model == "evoked"
        else ("HLA_IMMEDIATE",)
    )
    results: dict[str, object] = {}
    junit_cases: list[ET.Element] = []
    for model in models:
        results[model] = _run_model(arguments, model, classes_directory, environment)
        junit_path = output_directory / model.lower() / "results.xml"
        junit_cases.extend(list(ET.parse(junit_path).getroot()))

    aggregate = {
        "kind": "hla-rti-java-tck-process-adapter-evidence",
        "standard": "IEEE 1516.1-2025",
        "scenario": arguments.scenario,
        "callback_models": list(models),
        "results": results,
    }
    (output_directory / "results.json").write_text(
        json.dumps(aggregate, indent=2) + "\n", encoding="utf-8"
    )
    suite = ET.Element("testsuite", tests=str(len(junit_cases)))
    for case in junit_cases:
        suite.append(case)
    ET.ElementTree(suite).write(
        output_directory / "results.xml", encoding="utf-8", xml_declaration=True
    )
    print(f"{arguments.scenario}: passed in {', '.join(models)} callback model(s)")
    print(f"Evidence: {output_directory}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, TimeoutError, ValueError, subprocess.SubprocessError) as error:
        print(f"Java TLS TCK runner failed: {error}", file=sys.stderr)
        raise SystemExit(1)
