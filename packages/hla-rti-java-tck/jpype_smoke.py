"""Run a small JPype handoff against a provider that passed the Java TCK."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--direct-results", type=Path, required=True)
    parser.add_argument("--api-jar", type=Path, required=True)
    parser.add_argument("--provider-jar", type=Path, action="append", required=True)
    parser.add_argument("--dependency-jar", type=Path, action="append", default=[])
    parser.add_argument("--factory-name", required=True)
    parser.add_argument("--jvm-arg", action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    direct = json.loads(args.direct_results.read_text(encoding="utf-8"))
    direct_failures = [
        item for item in direct.get("scenarios", []) if item.get("status") == "fail"
    ]
    result: dict[str, Any] = {
        "schema_version": 1,
        "kind": "java-rti-tck-jpype-evidence",
        "provider": direct.get("provider", args.factory_name),
        "direct_java_tck_results": str(args.direct_results),
        "status": "blocked" if direct_failures else "pass",
        "steps": [],
    }
    if direct_failures:
        result["steps"].append(
            {"id": "direct-java-tck-gate", "status": "blocked", "message": "direct Java TCK has failures"}
        )
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        return 2

    jpype_module = None
    try:
        import jpype

        jpype_module = jpype

        jvm_args: list[str] = list(args.jvm_arg)
        classpath = [
            str(args.api_jar.resolve()),
            *(str(item.resolve()) for item in args.provider_jar),
            *(str(item.resolve()) for item in args.dependency_jar),
        ]
        jpype.startJVM(*jvm_args, classpath=classpath, convertStrings=True)
        factory_factory = jpype.JClass("hla.rti1516_2025.RtiFactoryFactory")
        factory = factory_factory.getRtiFactory(args.factory_name)
        factory_name = str(factory.rtiName())
        factory_version = str(factory.rtiVersion())
        encoder = factory.getEncoderFactory()
        value = encoder.createHLAinteger32BE(0x01020304)
        if int(value.getValue()) != 0x01020304:
            raise AssertionError("JPype encoder value did not round-trip")
        ambassador = factory.getRtiAmbassador()
        result["provider_name"] = factory_name
        result["provider_version"] = factory_version
        result["steps"].extend(
            [
                {"id": "direct-java-tck-gate", "status": "pass"},
                {"id": "jpype-service-loader-factory", "status": "pass"},
                {"id": "jpype-standard-encoder", "status": "pass"},
                {"id": "jpype-standard-ambassador", "status": "pass", "type": str(type(ambassador))},
            ]
        )
    except Exception as error:  # pragma: no cover - exercised by configured JVM lanes
        result["status"] = "fail"
        result["steps"].append({"id": "jpype-provider-handoff", "status": "fail", "message": repr(error)})
        result["error"] = repr(error)
        exit_code = 2
    else:
        exit_code = 0
    finally:
        if jpype_module is not None and getattr(jpype_module, "isJVMStarted", lambda: False)():
            jpype_module.shutdownJVM()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
