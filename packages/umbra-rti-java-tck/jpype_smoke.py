"""Compatibility entry point for the vendor-neutral Java TCK JPype smoke."""

import runpy
from pathlib import Path

runpy.run_path(
    str(Path(__file__).resolve().parents[1] / "hla-rti-java-tck" / "jpype_smoke.py"),
    run_name="__main__",
)
