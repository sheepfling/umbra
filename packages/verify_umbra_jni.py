"""Verify a staged Umbra Java/JNI product directory."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    from .umbra_jni_product import ProductError, edition_config, verify_product
except ImportError:
    from umbra_jni_product import ProductError, edition_config, verify_product


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Verify API ownership, ServiceLoader registrations, provider classes, "
            "manifests, hashes, and optional Java smoke tests."
        )
    )
    parser.add_argument("--edition", choices=("2025", "2010"), default="2025")
    parser.add_argument(
        "--artifact-directory",
        required=True,
        type=Path,
        help="Staged product directory to verify.",
    )
    parser.add_argument(
        "--java-api-jar",
        type=Path,
        help="API JAR to verify; otherwise discover the only non-provider JAR in the bundle.",
    )
    parser.add_argument("--expected-api-sha256")
    parser.add_argument("--expected-bridge-sha256")
    parser.add_argument("--expected-native-sha256")
    parser.add_argument("--fom-path", type=Path)
    parser.add_argument(
        "--skip-smoke-test",
        action="store_true",
        help="Only inspect the product; do not launch Java smoke tests.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    config = edition_config(arguments.edition)
    try:
        product = verify_product(
            config,
            arguments.artifact_directory,
            api_jar=arguments.java_api_jar,
            expected_api_sha256=arguments.expected_api_sha256,
            expected_bridge_sha256=arguments.expected_bridge_sha256,
            expected_native_sha256=arguments.expected_native_sha256,
            run_smoke=not arguments.skip_smoke_test,
            fom_path=arguments.fom_path,
        )
    except ProductError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(
        f"Verified {config.label} JNI product: {product.artifact_root} "
        f"({product.bridge_jar.name}, {product.native_library.name})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
