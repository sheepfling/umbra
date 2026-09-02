"""Build and stage a directly consumable Umbra Java/JNI product directory."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    from .umbra_jni_product import (
        ProductError,
        build_bridge,
        edition_config,
        required_file,
        stage_product,
        verify_product,
    )
except ImportError:
    from umbra_jni_product import (
        ProductError,
        build_bridge,
        edition_config,
        required_file,
        stage_product,
        verify_product,
    )


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Build the bounded Umbra C++ -> JNI -> Java bridge for IEEE 1516.1 "
            "2025 or IEEE 1516e-2010 and stage a movable product directory."
        )
    )
    parser.add_argument("--edition", choices=("2025", "2010"), default="2025")
    parser.add_argument(
        "--java-api-jar",
        required=True,
        type=Path,
        help="Path to the independently obtained standard Java API JAR.",
    )
    parser.add_argument(
        "--output-directory",
        type=Path,
        help="Product directory (default: out/umbra-jni-<edition>).",
    )
    parser.add_argument(
        "--build-directory",
        type=Path,
        help="Intermediate build directory (default: <output>/.build).",
    )
    parser.add_argument("--java-api-coordinate")
    parser.add_argument("--java-api-version")
    parser.add_argument(
        "--java-api-source",
        help="Optional URL or provenance string recorded in the dependency manifest.",
    )
    parser.add_argument(
        "--fom-path",
        type=Path,
        help="Optional 2025 FOM path passed to NativeSmokeTest.",
    )
    parser.add_argument(
        "--run-smoke-test",
        action="store_true",
        help="Run all staged Java surface/native/type smoke tests after packaging.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    root = Path(__file__).resolve().parents[1]
    config = edition_config(arguments.edition, root)
    output = (
        (arguments.output_directory or root / "out" / f"umbra-jni-{arguments.edition}")
        .expanduser()
        .resolve()
    )
    build_directory = (
        (arguments.build_directory or output / ".build").expanduser().resolve()
    )
    coordinate = arguments.java_api_coordinate or config.default_api_coordinate
    version = arguments.java_api_version or config.default_api_version

    try:
        if arguments.fom_path is not None and arguments.edition != "2025":
            raise ProductError(
                "--fom-path is available only for the IEEE 1516.1-2025 bundle"
            )
        api_path = required_file(arguments.java_api_jar, f"{config.label} Java API JAR")
        bridge = build_bridge(config, api_path, build_directory)
        product = stage_product(
            config,
            bridge,
            output,
            api_coordinate=coordinate,
            api_version=version,
            api_source=arguments.java_api_source,
        )
        verify_product(
            config,
            product.artifact_root,
            api_jar=product.api_jar,
            run_smoke=arguments.run_smoke_test,
            fom_path=arguments.fom_path,
        )
    except ProductError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(product.artifact_root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
