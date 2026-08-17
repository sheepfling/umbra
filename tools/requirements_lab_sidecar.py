"""Run the HLA Requirements Lab's external compliance sidecar from Umbra.

The wrapper deliberately keeps the Lab outside Umbra's runtime dependencies.
It only invokes the Lab's documented command-line modules from a pinned local
checkout and keeps generated requests, raw manifests, and JUnit artifacts in
the ignored .compliance directory.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LAB_ROOT = REPOSITORY_ROOT.parent / "Document-Recreation"
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_REQUEST = REPOSITORY_ROOT / ".compliance" / "adapter-request.json"
DEFAULT_MANIFEST = REPOSITORY_ROOT / ".compliance" / "implementation-manifest.raw.json"


def _lab_root(value: Path | None) -> Path:
    root = value or Path(os.environ.get("HLA_REQUIREMENTS_LAB_ROOT", DEFAULT_LAB_ROOT))
    root = root.resolve()
    sidecar = root / "hla_lab" / "traceability" / "compliance_sidecar.py"
    adapter = root / "hla_lab" / "traceability" / "junit_adapter.py"
    if not sidecar.is_file() or not adapter.is_file():
        raise ValueError(
            f"{root} is not an HLA Requirements Lab checkout with the compliance sidecar; "
            "set HLA_REQUIREMENTS_LAB_ROOT or pass --lab-root"
        )
    return root


def _run(command: tuple[str, ...], lab_root: Path) -> None:
    subprocess.run(command, cwd=lab_root, check=True)


def _common_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--lab-root", type=Path)
    parser.add_argument("--python", default=sys.executable, help="Python environment for the Lab")
    parser.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    parser.add_argument("--request-id", default="umbra-compliance-request")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    prepare = subparsers.add_parser("prepare", help="Create a portable Lab AdapterRequest")
    _common_arguments(prepare)
    prepare.add_argument("--output", type=Path, default=DEFAULT_REQUEST)

    raw = subparsers.add_parser("raw", help="Turn existing Catch2 JUnit output into a raw manifest")
    _common_arguments(raw)
    raw.add_argument("--catalog", type=Path, required=True)
    raw.add_argument("--results", type=Path, action="append", required=True)
    raw.add_argument("--repository", default="sheepfling/umbra")
    raw.add_argument("--output", type=Path, default=DEFAULT_MANIFEST)

    verify = subparsers.add_parser("verify", help="Verify an externally reviewed manifest")
    _common_arguments(verify)
    verify.add_argument("--manifest", type=Path, required=True)

    args = parser.parse_args()
    try:
        lab_root = _lab_root(args.lab_root)
        bundle = args.bundle.resolve()
        if not bundle.is_file():
            raise ValueError(f"bundle is absent: {bundle}; run requirements_lab.py export first")

        if args.command == "prepare":
            output = args.output.resolve()
            output.parent.mkdir(parents=True, exist_ok=True)
            _run(
                (
                    args.python,
                    "-m",
                    "hla_lab.traceability.compliance_sidecar",
                    "prepare",
                    "--bundle",
                    str(bundle),
                    "--request-id",
                    args.request_id,
                    "--output",
                    str(output),
                ),
                lab_root,
            )
            return 0

        if args.command == "raw":
            catalog = args.catalog.resolve()
            if not catalog.is_file():
                raise ValueError(f"test catalog is absent: {catalog}")
            results = tuple(path.resolve() for path in args.results)
            missing = [str(path) for path in results if not path.is_file()]
            if missing:
                raise ValueError(f"JUnit result artifacts are absent: {', '.join(missing)}")
            output = args.output.resolve()
            output.parent.mkdir(parents=True, exist_ok=True)
            adapter_arguments: list[str] = []
            for result in results:
                adapter_arguments.extend(("--results", str(result)))
            adapter_command = (
                args.python,
                "-m",
                "hla_lab.traceability.junit_adapter",
                "--catalog",
                str(catalog),
                "--repository",
                args.repository,
                *adapter_arguments,
            )
            _run(
                (
                    args.python,
                    "-m",
                    "hla_lab.traceability.compliance_sidecar",
                    "check",
                    "--bundle",
                    str(bundle),
                    "--request-id",
                    args.request_id,
                    "--manifest-output",
                    str(output),
                    "--json",
                    "--adapter-command",
                    *adapter_command,
                ),
                lab_root,
            )
            return 0

        manifest = args.manifest.resolve()
        if not manifest.is_file():
            raise ValueError(f"manifest is absent: {manifest}")
        _run(
            (
                args.python,
                "-m",
                "hla_lab.traceability.compliance_sidecar",
                "check",
                "--bundle",
                str(bundle),
                "--request-id",
                args.request_id,
                "--manifest",
                str(manifest),
                "--json",
            ),
            lab_root,
        )
        return 0
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"requirements-lab-sidecar: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
