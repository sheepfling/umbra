"""Audit the checked-in IEEE 1516e Python contract and optional adapter."""

from __future__ import annotations

import argparse
import ast
import importlib
import sys
from collections.abc import Iterable, Mapping
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NATIVE_SOURCE = (
    ROOT
    / "packages"
    / "umbra-rti-native"
    / "src"
    / "umbra"
    / "_native"
    / "rti1516e"
    / "__init__.py"
)


def _missing(owner: type[object], names: Iterable[str]) -> list[str]:
    return [name for name in names if not callable(getattr(owner, name, None))]


def _metadata_findings(
    owner: type[object],
    method_names: Iterable[str],
    expected_counts: Mapping[str, int],
    expected_parameter_types: Mapping[str, tuple[tuple[str, ...], ...]],
    expected_return_types: Mapping[str, tuple[str, ...]],
    label: str,
) -> list[str]:
    """Ensure an adapter retains the generated Java overload contract."""

    findings: list[str] = []
    actual_counts = getattr(owner, "__overload_counts__", None)
    if dict(actual_counts or {}) != dict(expected_counts):
        findings.append(
            f"{label} overload-count metadata differs from the 2010 contract"
        )
    if set(expected_parameter_types) != set(expected_counts):
        findings.append(f"{label} parameter metadata is missing an overload entry")
    if set(expected_return_types) != set(expected_counts):
        findings.append(f"{label} return metadata is missing an overload entry")
    for name in method_names:
        count = expected_counts[name]
        parameter_overloads = expected_parameter_types.get(name, ())
        return_overloads = expected_return_types.get(name, ())
        if len(parameter_overloads) != count:
            findings.append(f"{label}.{name} parameter overload count is not {count}")
        if len(return_overloads) != count:
            findings.append(f"{label}.{name} return overload count is not {count}")
    return findings


def _verify_native_source() -> list[str]:
    """Check the source-only dynamic façade without loading the extension."""

    try:
        tree = ast.parse(
            NATIVE_SOURCE.read_text(encoding="utf-8"), filename=str(NATIVE_SOURCE)
        )
    except (OSError, SyntaxError, UnicodeDecodeError) as error:
        return [f"native 2010 façade source could not be parsed: {error}"]

    findings: list[str] = []
    classes = {node.name: node for node in tree.body if isinstance(node, ast.ClassDef)}
    ambassador = classes.get("Native2010RTIambassador")
    if ambassador is None:
        findings.append("native source is missing Native2010RTIambassador")
    else:
        bases = {base.id for base in ambassador.bases if isinstance(base, ast.Name)}
        if "RTIambassador" not in bases:
            findings.append("Native2010RTIambassador does not inherit RTIambassador")
        explicit_methods = {
            node.name
            for node in ambassador.body
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
        }
        if not {"connect", "disconnect", "getTimeFactory"}.issubset(explicit_methods):
            findings.append("native source is missing its implemented façade methods")

    if not any(
        isinstance(node, ast.For)
        and isinstance(node.target, ast.Name)
        and node.target.id == "_name"
        and isinstance(node.iter, ast.Name)
        and node.iter.id == "RTIAMBASSADOR_METHODS"
        and any(
            isinstance(child, ast.Call)
            and isinstance(child.func, ast.Name)
            and child.func.id == "setattr"
            for child in ast.walk(node)
        )
        for node in tree.body
    ):
        findings.append(
            "native source does not dynamically bind the complete RTIAMBASSADOR_METHODS contract"
        )
    if not any(
        isinstance(node, ast.ClassDef) and node.name == "Native2010RtiFactory"
        for node in tree.body
    ):
        findings.append("native source is missing Native2010RtiFactory")
    if "update_abstractmethods(Native2010RTIambassador)" not in NATIVE_SOURCE.read_text(
        encoding="utf-8"
    ):
        findings.append("native source does not refresh abstract-method metadata")
    return findings


def verify(
    check_jpype: bool = False,
    check_native: bool = False,
    check_native_source: bool = False,
) -> list[str]:
    import hla.rti1516e as rti
    from hla.rti1516e import contracts
    from hla.rti1516e.encoding import EncoderFactory
    from hla.rti1516e.time import HLAfloat64TimeFactory, HLAinteger64TimeFactory

    findings: list[str] = []
    if rti.STANDARD_EDITION != "IEEE 1516.1-2010":
        findings.append("wrong standard edition")
    if rti.JAVA_PACKAGE != "hla.rti1516e":
        findings.append("wrong Java package")
    if rti.CPP_NAMESPACE != "rti1516e":
        findings.append("wrong C++ namespace")
    findings.extend(
        f"RTIambassador missing {name}"
        for name in _missing(rti.RTIambassador, contracts.RTIAMBASSADOR_METHODS)
    )
    findings.extend(
        f"FederateAmbassador missing {name}"
        for name in _missing(
            rti.FederateAmbassador, contracts.FEDERATE_AMBASSADOR_METHODS
        )
    )
    findings.extend(
        _metadata_findings(
            rti.RTIambassador,
            contracts.RTIAMBASSADOR_METHODS,
            contracts.RTIAMBASSADOR_OVERLOAD_COUNTS,
            contracts.RTIAMBASSADOR_PARAMETER_TYPES,
            contracts.RTIAMBASSADOR_RETURN_TYPES,
            "RTIambassador",
        )
    )
    findings.extend(
        _metadata_findings(
            rti.FederateAmbassador,
            contracts.FEDERATE_AMBASSADOR_METHODS,
            contracts.FEDERATE_AMBASSADOR_OVERLOAD_COUNTS,
            contracts.FEDERATE_AMBASSADOR_PARAMETER_TYPES,
            contracts.FEDERATE_AMBASSADOR_RETURN_TYPES,
            "FederateAmbassador",
        )
    )
    if not hasattr(rti.RtiFactoryFactory, "getAvailableRtiFactories"):
        findings.append("RtiFactoryFactory missing getAvailableRtiFactories")
    if not hasattr(rti.LogicalTimeFactoryFactory, "getAvailableLogicalTimeFactories"):
        findings.append(
            "LogicalTimeFactoryFactory missing getAvailableLogicalTimeFactories"
        )
    for name in EncoderFactory.__abstractmethods__:
        if not hasattr(EncoderFactory, name):
            findings.append(f"EncoderFactory missing {name}")
    for factory_type in (HLAfloat64TimeFactory, HLAinteger64TimeFactory):
        for name in (
            "decodeTime",
            "decodeInterval",
            "makeInitial",
            "makeFinal",
            "makeZero",
            "makeEpsilon",
            "makeTime",
            "makeInterval",
            "getName",
        ):
            if not hasattr(factory_type, name):
                findings.append(f"{factory_type.__name__} missing {name}")
    if not hasattr(rti.ParameterHandleValueMap, "getValueReference"):
        findings.append("ParameterHandleValueMap missing getValueReference")
    if not (
        rti.FederateAmbassador.SupplementalReflectInfo is rti.SupplementalReflectInfo
        and rti.FederateAmbassador.SupplementalReceiveInfo
        is rti.SupplementalReceiveInfo
        and rti.FederateAmbassador.SupplementalRemoveInfo is rti.SupplementalRemoveInfo
    ):
        findings.append("nested callback record aliases are not Java-shaped")

    if check_jpype:
        try:
            provider = importlib.import_module("umbra._java.rti1516e.provider")
        except ImportError as error:
            findings.append(f"JPype adapter could not be imported: {error}")
        else:
            owner = provider.Java2010RTIambassador
            findings.extend(
                f"Java2010RTIambassador missing {name}"
                for name in _missing(owner, contracts.RTIAMBASSADOR_METHODS)
            )
            findings.extend(
                _metadata_findings(
                    owner,
                    contracts.RTIAMBASSADOR_METHODS,
                    contracts.RTIAMBASSADOR_OVERLOAD_COUNTS,
                    contracts.RTIAMBASSADOR_PARAMETER_TYPES,
                    contracts.RTIAMBASSADOR_RETURN_TYPES,
                    "Java2010RTIambassador",
                )
            )
            if getattr(owner, "__abstractmethods__", ()):
                findings.append(
                    "Java2010RTIambassador remains abstract: "
                    + ", ".join(sorted(owner.__abstractmethods__))
                )
    if check_native:
        try:
            native = importlib.import_module("umbra._native.rti1516e")
        except ImportError as error:
            findings.append(f"native 2010 adapter could not be imported: {error}")
        else:
            owner = native.Native2010RTIambassador
            findings.extend(
                f"Native2010RTIambassador missing {name}"
                for name in _missing(owner, contracts.RTIAMBASSADOR_METHODS)
            )
            findings.extend(
                _metadata_findings(
                    owner,
                    contracts.RTIAMBASSADOR_METHODS,
                    contracts.RTIAMBASSADOR_OVERLOAD_COUNTS,
                    contracts.RTIAMBASSADOR_PARAMETER_TYPES,
                    contracts.RTIAMBASSADOR_RETURN_TYPES,
                    "Native2010RTIambassador",
                )
            )
            if getattr(owner, "__abstractmethods__", ()):
                findings.append(
                    "Native2010RTIambassador remains abstract: "
                    + ", ".join(sorted(owner.__abstractmethods__))
                )
            factory = native.Native2010RtiFactory()
            if not str(factory.rtiName()) or not str(factory.rtiVersion()):
                findings.append("Native2010RtiFactory did not provide a name/version")
            for accessor in ("getRtiAmbassador", "getEncoderFactory"):
                if not callable(getattr(factory, accessor, None)):
                    findings.append(f"Native2010RtiFactory missing {accessor}")
    if check_native_source:
        findings.extend(_verify_native_source())
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-jpype", action="store_true")
    parser.add_argument("--check-native", action="store_true")
    parser.add_argument("--check-native-source", action="store_true")
    args = parser.parse_args()
    try:
        findings = verify(
            args.check_jpype,
            args.check_native,
            args.check_native_source,
        )
    except (
        AttributeError,
        ImportError,
        OSError,
        RuntimeError,
        TypeError,
        ValueError,
    ) as error:
        print(f"IEEE 1516e Python surface verification: FAIL: {error}", file=sys.stderr)
        return 1
    if findings:
        print("IEEE 1516e Python surface verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e Python surface verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
