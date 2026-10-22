#!/usr/bin/env python3
"""Build only the Java JNI provider JAR, without rebuilding the native RTI.

This is an adapter build helper. The portable Java TCK itself compiles against
the separately supplied official IEEE API JAR, never against provider sources.
"""

from __future__ import annotations

import argparse
import subprocess
import tempfile
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SOURCES = ROOT / "packages/umbra-rti-jni/src/main/java"
RESOURCES = ROOT / "packages/umbra-rti-jni/src/main/resources"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--output-jar", type=Path, required=True)
    args = parser.parse_args()

    api_jar = args.api_jar.resolve()
    output_jar = args.output_jar.resolve()
    if not api_jar.is_file():
        parser.error(f"official IEEE API JAR not found: {api_jar}")
    if output_jar.exists():
        parser.error(f"refusing to overwrite existing provider JAR: {output_jar}")
    sources = sorted(SOURCES.rglob("*.java"))
    if not sources or not RESOURCES.is_dir():
        parser.error("JNI Java sources or service resources are missing")

    output_jar.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="jni-java-provider-", dir=output_jar.parent) as work:
        work_dir = Path(work)
        classes = work_dir / "classes"
        classes.mkdir()
        subprocess.run(
            ["javac", "-encoding", "UTF-8", "-cp", str(api_jar), "-d", str(classes),
             *(str(source) for source in sources)],
            cwd=ROOT,
            check=True,
        )
        subprocess.run(
            ["java", f"-Djava.io.tmpdir={work_dir}", "-m",
             "jdk.jartool/sun.tools.jar.Main", "--create", "--file", str(output_jar),
             "-C", str(classes), ".", "-C", str(RESOURCES), "."],
            cwd=ROOT,
            check=True,
        )

    with zipfile.ZipFile(output_jar) as provider:
        names = provider.namelist()
        if any(name.startswith("hla/rti1516_2025/") for name in names):
            raise RuntimeError("provider JAR must not bundle the official API")
        if "META-INF/services/hla.rti1516_2025.RtiFactory" not in names:
            raise RuntimeError("provider JAR is missing the RTI factory service descriptor")
    print(f"Java JNI provider JAR: {output_jar}")
    print(f"Java sources: {len(sources)}; native library: unchanged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
