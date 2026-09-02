"""Run a metadata-only preflight for an IEEE 1516e Java provider JAR.

This check answers the narrow onboarding question: does a supplied JAR expose
the *2010* standard ``ServiceLoader`` entry point, with its provider class
present in the supplied provider/dependency class path, without accidentally
selecting the 2025 namespace?  It never starts a JVM, loads provider classes,
or claims RTI conformance.  The runtime probe/TCK remain responsible for those
checks.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Iterable
from zipfile import BadZipFile, ZipFile


SERVICE_PATH = "META-INF/services/hla.rti1516e.RtiFactory"
SERVICE_2025_PATH = "META-INF/services/hla.rti1516_2025.RtiFactory"
REQUIRED_API_ENTRIES = (
    "hla/rti1516e/RtiFactory.class",
    "hla/rti1516e/RtiFactoryFactory.class",
    "hla/rti1516e/RTIambassador.class",
    "hla/rti1516e/FederateAmbassador.class",
    "hla/rti1516e/encoding/EncoderFactory.class",
)
_JAVA_CLASS_NAME = re.compile(r"^[A-Za-z_$][A-Za-z0-9_$]*(?:\.[A-Za-z_$][A-Za-z0-9_$]*)*$")


@dataclass(frozen=True, slots=True)
class JarPreflight:
    path: str
    kind: str
    status: str
    service_entries: tuple[str, ...]
    has_2010_api: bool
    has_2025_namespace: bool
    findings: tuple[str, ...]

    def as_dict(self) -> dict[str, object]:
        return {
            "path": self.path,
            "kind": self.kind,
            "status": self.status,
            "service_path": SERVICE_PATH,
            "service_entries": list(self.service_entries),
            "has_2010_api": self.has_2010_api,
            "has_2025_namespace": self.has_2025_namespace,
            "findings": list(self.findings),
        }


def _service_entries(archive: ZipFile) -> tuple[str, ...]:
    if SERVICE_PATH not in archive.namelist():
        return ()
    entries: list[str] = []
    for raw in archive.read(SERVICE_PATH).decode("utf-8-sig").splitlines():
        value = raw.split("#", 1)[0].strip()
        if value:
            entries.append(value)
    return tuple(entries)


def _preflight(
    path: Path,
    *,
    kind: str,
    require_service: bool,
    classpath_entries: set[str] | None = None,
) -> JarPreflight:
    findings: list[str] = []
    service_entries: tuple[str, ...] = ()
    try:
        with ZipFile(path) as archive:
            names = set(archive.namelist())
            service_entries = _service_entries(archive)
            has_2010_api = all(entry in names for entry in REQUIRED_API_ENTRIES)
            has_2025_namespace = (
                SERVICE_2025_PATH in names
                or any(name.startswith("hla/rti1516_2025/") for name in names)
            )
            if require_service and not service_entries:
                findings.append(
                    f"missing or empty {SERVICE_PATH} ServiceLoader descriptor"
                )
            for provider in service_entries:
                if not _JAVA_CLASS_NAME.fullmatch(provider):
                    findings.append(f"invalid Java provider class name: {provider!r}")
                elif kind == "provider" and classpath_entries is not None:
                    provider_entry = provider.replace(".", "/") + ".class"
                    if provider_entry not in classpath_entries:
                        findings.append(
                            "ServiceLoader provider class is not present in the supplied "
                            f"provider/dependency class path: {provider_entry}"
                        )
                if "rti1516_2025" in provider or "1516_2025" in provider:
                    findings.append(
                        f"2010 service descriptor advertises a 2025 provider: {provider}"
                    )
            if has_2025_namespace:
                findings.append(
                    "JAR contains the hla.rti1516_2025 namespace; keep 2010 and 2025 "
                    "provider artifacts separate"
                )
            if kind == "api" and not has_2010_api:
                missing = [entry for entry in REQUIRED_API_ENTRIES if entry not in names]
                findings.append("API JAR is missing: " + ", ".join(missing))
    except FileNotFoundError:
        has_2010_api = False
        has_2025_namespace = False
        findings.append("file does not exist")
    except (BadZipFile, OSError, UnicodeDecodeError) as error:
        has_2010_api = False
        has_2025_namespace = False
        findings.append(f"cannot inspect JAR metadata: {error}")

    return JarPreflight(
        path=str(path),
        kind=kind,
        status="fail" if findings else "pass",
        service_entries=service_entries,
        has_2010_api=has_2010_api,
        has_2025_namespace=has_2025_namespace,
        findings=tuple(findings),
    )


def verify(
    provider_jars: Iterable[Path],
    api_jar: Path | None = None,
    dependency_jars: Iterable[Path] = (),
) -> list[JarPreflight]:
    provider_paths = tuple(provider_jars)
    dependency_paths = tuple(dependency_jars)
    classpath_entries: set[str] = set()
    for path in (*provider_paths, *dependency_paths):
        try:
            with ZipFile(path) as archive:
                classpath_entries.update(archive.namelist())
        except (FileNotFoundError, BadZipFile, OSError):
            # The per-archive preflight below reports the actionable error.
            continue
    results = [
        _preflight(
            path,
            kind="provider",
            require_service=True,
            classpath_entries=classpath_entries,
        )
        for path in provider_paths
    ]
    results.extend(
        _preflight(path, kind="dependency", require_service=False)
        for path in dependency_paths
    )
    if api_jar is not None:
        results.append(_preflight(api_jar, kind="api", require_service=False))
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--provider-jar",
        action="append",
        type=Path,
        required=True,
        help="2010 provider JAR; repeat only when multiple providers are intentional",
    )
    parser.add_argument(
        "--api-jar",
        type=Path,
        help="optional 2010 API JAR to check for the canonical interface entries",
    )
    parser.add_argument(
        "--dependency-jar",
        action="append",
        type=Path,
        default=[],
        help="optional provider dependency JAR; repeat as needed",
    )
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()
    results = verify(args.provider_jar, args.api_jar, args.dependency_jar)
    payload = {
        "kind": "ieee1516e-provider-jar-preflight",
        "status": "fail" if any(item.status == "fail" for item in results) else "pass",
        "conformance_claim": False,
        "results": [item.as_dict() for item in results],
    }
    if args.json:
        print(json.dumps(payload, indent=2))
    else:
        print(
            "IEEE 1516e provider-JAR metadata preflight: "
            f"{payload['status'].upper()} (metadata only; no conformance claim)"
        )
        for item in results:
            print(f"  {item.kind}: {item.path}: {item.status.upper()}")
            if item.service_entries:
                print("    providers: " + ", ".join(item.service_entries))
            for finding in item.findings:
                print(f"    {finding}")
    return 1 if payload["status"] == "fail" else 0


if __name__ == "__main__":
    raise SystemExit(main())
