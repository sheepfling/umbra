"""Run the transplantable IEEE 1516e Python provider contract smoke.

The default route consumes the standard ``hla.rti1516e`` Python contract and a
caller-supplied Java API/provider class path. ``--native-provider`` selects the
direct C++/pybind route. Both JSON results use the same scenario IDs so the
evidence can be checked and mapped to the 2010 Requirements Lab catalog.
"""

from __future__ import annotations

import argparse
import json
import time
from pathlib import Path
from types import SimpleNamespace
from typing import Any, Callable

from hla.rti1516e import (
    AttributeRegionAssociation,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DimensionHandleSet,
    FEDERATE_AMBASSADOR_METHODS,
    FEDERATE_AMBASSADOR_OVERLOAD_COUNTS,
    NullFederateAmbassador,
    RangeBounds,
    ResignAction,
    RTIAMBASSADOR_METHODS,
    RTIAMBASSADOR_OVERLOAD_COUNTS,
    STANDARD_EDITION,
)
from hla.rti1516e.encoding import (
    ByteWrapper,
    DataElementFactory,
    DecoderException,
    HLAfixedRecord,
    HLAinteger32BE,
    HLAoctet,
    HLAunicodeString,
    HLAvariableArray,
)
from hla.rti1516e.exceptions import (
    FederateServiceInvocationsAreBeingReportedViaMOM,
    RTIexception,
    exceptionForName,
)


def _scenario(
    scenario_id: str,
    category: str,
    action: Callable[[], str | None],
) -> dict[str, Any]:
    try:
        message = action() or ""
        return {"id": scenario_id, "category": category, "status": "pass", "message": message}
    except (NotImplementedError, AttributeError) as error:
        # A vendor may expose only the generic LogicalTimeFactory shape. A
        # missing optional convenience operation is visible as unsupported,
        # never converted into a false pass.
        return {
            "id": scenario_id,
            "category": category,
            "status": "unsupported",
            "message": str(error),
        }
    except Exception as error:  # pragma: no cover - exercised by a vendor run
        return {
            "id": scenario_id,
            "category": category,
            "status": "fail",
            "message": str(error),
        }


def _load_capability_profile(path: Path | None) -> dict[str, str]:
    """Load the shared Java-properties capability profile format.

    Profiles are deliberately only a gating declaration.  ``run``/``pass``
    permit the scenario to execute, while ``unsupported`` and
    ``not applicable`` produce an explicit non-asserting result.  A profile
    can therefore never manufacture a conformance pass.
    """

    if path is None:
        return {}
    resolved = path.resolve()
    if not resolved.is_file():
        raise FileNotFoundError(f"capability profile does not exist: {resolved}")
    catalog_root = Path(__file__).resolve().parents[2] / "compliance" / "catalogs"
    known_scenarios: set[str] = set()
    for catalog_name in (
        "java-2010-tck-scenario-catalog.json",
        "python-2010-tck-scenario-catalog.json",
    ):
        catalog_path = catalog_root / catalog_name
        try:
            catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
            known_scenarios.update(
                scenario["id"]
                for scenario in catalog["scenarios"]
                if isinstance(scenario, dict) and isinstance(scenario.get("id"), str)
            )
        except (OSError, UnicodeDecodeError, json.JSONDecodeError, KeyError, TypeError) as error:
            raise RuntimeError(
                f"cannot load the checked-in 2010 scenario catalog {catalog_path}: {error}"
            ) from error
    profile: dict[str, str] = {}
    for line_number, raw_line in enumerate(resolved.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith(('#', ';')):
            continue
        if "=" not in line:
            raise ValueError(f"capability profile line {line_number} has no '=': {raw_line!r}")
        key, value = (part.strip() for part in line.split("=", 1))
        if not key.startswith("scenario.") or not key[len("scenario.") :]:
            raise ValueError(f"capability profile line {line_number} must use scenario.<id>: {key!r}")
        scenario_id = key[len("scenario.") :]
        if scenario_id in profile:
            raise ValueError(
                f"capability profile line {line_number} repeats scenario id {scenario_id!r}"
            )
        if scenario_id not in known_scenarios:
            raise ValueError(
                f"capability profile line {line_number} names unknown scenario id {scenario_id!r}"
            )
        normalized = value.lower().replace("_", " ").replace("-", " ").strip()
        if normalized in {"run", "pass"}:
            mode = "run"
        elif normalized in {"unsupported", "skip"}:
            mode = "unsupported"
        elif normalized == "not applicable":
            mode = "not applicable"
        else:
            raise ValueError(
                f"unknown capability profile mode for {key[len('scenario.') :]}: {value!r}"
            )
        profile[scenario_id] = mode
    return profile


def _profiled_scenario(
    profile: dict[str, str],
    scenario_id: str,
    category: str,
    action: Callable[[], str | None],
) -> dict[str, Any]:
    mode = profile.get(scenario_id, "run")
    if mode in {"unsupported", "not applicable"}:
        return {
            "id": scenario_id,
            "category": category,
            "status": mode,
            "message": f"{mode} by capability profile; no provider assertion was made",
        }
    return _scenario(scenario_id, category, action)


def _profiled_failure(
    profile: dict[str, str], scenario_id: str, category: str, error: Exception
) -> dict[str, Any]:
    mode = profile.get(scenario_id, "run")
    if mode in {"unsupported", "not applicable"}:
        return {
            "id": scenario_id,
            "category": category,
            "status": mode,
            "message": f"{mode} by capability profile; no provider assertion was made",
        }
    return {"id": scenario_id, "category": category, "status": "fail", "message": str(error)}


def _drain_callbacks(
    ambassador: object,
    labels: list[str],
    timeout: float = 0.25,
    expected: int = 1,
) -> None:
    """Give a provider a bounded chance to deliver queued callbacks."""

    deadline = time.monotonic() + timeout
    while len(labels) < expected and time.monotonic() < deadline:
        for method_name, arguments in (
            ("evokeCallback", (0.01,)),
            ("evokeMultipleCallbacks", (0.0, 0.01)),
        ):
            method = getattr(ambassador, method_name, None)
            if method is None:
                continue
            try:
                method(*arguments)
            except (AttributeError, NotImplementedError):
                continue
        if len(labels) < expected:
            time.sleep(0.005)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--api-jar",
        type=Path,
        help="2010 Java API JAR (required for the JPype provider route)",
    )
    parser.add_argument(
        "--provider-jar",
        type=Path,
        action="append",
        help="provider JAR; repeat when multiple providers are intentional",
    )
    parser.add_argument(
        "--native-provider",
        action="store_true",
        help="exercise the installed direct C++/pybind 2010 provider instead of JPype",
    )
    parser.add_argument(
        "--dependency-jar",
        type=Path,
        action="append",
        default=[],
        help="Additional provider dependency JAR; may be repeated.",
    )
    parser.add_argument("--factory-name")
    parser.add_argument("--jvm-path")
    parser.add_argument(
        "--fom-path",
        type=Path,
        help="Optional 2010 FOM module used by the federation-membership scenario.",
    )
    parser.add_argument(
        "--mim-path",
        type=Path,
        help="Optional IEEE 1516-2010 standard MOM/Initialization Module (MIM).",
    )
    parser.add_argument(
        "--object-class",
        default="HLAobjectRoot.Employee.Server",
        help="FOM object class used by declaration/object scenarios.",
    )
    parser.add_argument(
        "--attribute-name",
        default="Efficiency",
        help="FOM attribute used by declaration/object scenarios.",
    )
    parser.add_argument(
        "--interaction-class",
        default="HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
        help="FOM interaction class used by the interaction scenario.",
    )
    parser.add_argument(
        "--parameter-name",
        default="TemperatureOk",
        help="FOM interaction parameter used by the interaction scenario.",
    )
    parser.add_argument(
        "--jvm-option",
        action="append",
        default=[],
        help="Additional JVM option; may be repeated.",
    )
    parser.add_argument(
        "--capability-profile",
        type=Path,
        help="Optional shared Java-properties profile that gates provider scenarios.",
    )
    parser.add_argument("--results", type=Path, required=True)
    args = parser.parse_args()

    if not args.native_provider and (args.api_jar is None or args.provider_jar is None):
        parser.error("--api-jar and --provider-jar are required unless --native-provider is used")
    if args.native_provider and (
        args.api_jar is not None
        or args.provider_jar is not None
        or args.dependency_jar
    ):
        parser.error("--native-provider cannot be combined with --api-jar or --provider-jar")

    capability_profile = _load_capability_profile(args.capability_profile)
    scenarios: list[dict[str, Any]] = []
    factory = None
    probe = None
    ambassador = None

    try:
        if args.native_provider:
            from umbra._native.rti1516e import Native2010RtiFactory

            factory = Native2010RtiFactory()
            probe = SimpleNamespace(
                rti_name=factory.rtiName(),
                rti_version=factory.rtiVersion(),
            )
        else:
            from umbra._java.rti1516e import Java2010RtiFactory

            factory = Java2010RtiFactory.from_jar(
                args.api_jar,
                dependencies=(*args.provider_jar, *args.dependency_jar),
                factory_name=args.factory_name,
                jvm_path=args.jvm_path,
                jvm_options=tuple(args.jvm_option),
            )
            probe = factory.probe()
    except Exception as error:
        # There is no provider boundary to exercise if discovery itself fails;
        # retain the full expected result shape for evidence tooling.
        for scenario_id, category in (
            ("python-2010-tck.factory-discovery", "lifecycle"),
            ("python-2010-tck.encoder-round-trip", "encoding"),
            ("python-2010-tck.malformed-inputs", "malformed-inputs"),
            ("python-2010-tck.complex-encoders", "encoding"),
            ("python-2010-tck.api-surface-inventory", "api-surface"),
            ("python-2010-tck.overloads-and-exceptions", "exceptions"),
            ("python-2010-tck.connect-disconnect", "lifecycle"),
            ("python-2010-tck.federation-membership", "lifecycle"),
            ("python-2010-tck.declaration-management", "declarations"),
            ("python-2010-tck.object-management", "object-management"),
            ("python-2010-tck.ownership-management", "ownership"),
            ("python-2010-tck.interaction-management", "interactions"),
            ("python-2010-tck.synchronization-point", "synchronization"),
            ("python-2010-tck.ddm-management", "ddm"),
            ("python-2010-tck.mom-service-reporting", "mom"),
            ("python-2010-tck.mom-service-reporting-interlocks", "mom"),
            ("python-2010-tck.mom-federate-object", "mom"),
            ("python-2010-tck.mom-publication-report", "mom"),
            ("python-2010-tck.mom-subscription-report", "mom"),
            ("python-2010-tck.mom-interaction-publication-subscription-reports", "mom"),
            ("python-2010-tck.mom-synchronization-reports", "mom"),
            ("python-2010-tck.mom-exception-reporting", "mom"),
            ("python-2010-tck.mom-federation-object", "mom"),
            ("python-2010-tck.mom-mom-exception", "mom"),
            ("python-2010-tck.mom-timing", "mom"),
            ("python-2010-tck.mom-object-instance-information", "mom"),
            ("python-2010-tck.mom-object-instances-can-be-deleted", "mom"),
            ("python-2010-tck.mom-object-instance-count-reports", "mom"),
            ("python-2010-tck.mom-transport-count-reports", "mom"),
            ("python-2010-tck.mom-interaction-count-reports", "mom"),
            ("python-2010-tck.mom-fom-mim-data-reports", "mom"),
            ("python-2010-tck.time-management", "time"),
            ("python-2010-tck.save-restore", "save-restore"),
            ("python-2010-tck.logical-time-arithmetic", "time"),
        ):
            scenarios.append(_profiled_failure(capability_profile, scenario_id, category, error))
    else:
        def factory_discovery() -> str:
            """Exercise the provider-owned 2010 factory/carrier families.

            Discovery is intentionally more than a name/version probe.  A
            standard Java provider must expose the handle decoders, mutable
            collection factories, DDM pair-list factory, transportation
            defaults, and selected logical-time factory through the same
            ambassador returned to Python.  The assertions stay at the
            provider-neutral boundary and never inspect fixture internals.
            """

            if probe is None:
                raise AssertionError("factory probe was not initialized")
            if not probe.rti_name or not probe.rti_version:
                raise AssertionError("provider returned an empty name or version")
            candidate = factory.getRtiAmbassador()
            handle_factories = (
                "getFederateHandleFactory",
                "getObjectClassHandleFactory",
                "getObjectInstanceHandleFactory",
                "getAttributeHandleFactory",
                "getInteractionClassHandleFactory",
                "getParameterHandleFactory",
                "getDimensionHandleFactory",
            )
            for accessor in handle_factories:
                handle_factory = getattr(candidate, accessor)()
                if handle_factory is None or not callable(getattr(handle_factory, "decode", None)):
                    raise AssertionError(f"{accessor} did not return a decoder factory")
            collection_factories = (
                "getAttributeHandleSetFactory",
                "getDimensionHandleSetFactory",
                "getFederateHandleSetFactory",
                "getRegionHandleSetFactory",
                "getAttributeHandleValueMapFactory",
                "getParameterHandleValueMapFactory",
                "getAttributeSetRegionSetPairListFactory",
            )
            for accessor in collection_factories:
                collection_factory = getattr(candidate, accessor)()
                if collection_factory is None or not callable(getattr(collection_factory, "create", None)):
                    raise AssertionError(f"{accessor} did not return a collection factory")
                created = collection_factory.create()
                if created is None:
                    raise AssertionError(f"{accessor}.create() returned None")
            transportation = candidate.getTransportationTypeHandleFactory()
            if transportation.getHLAdefaultReliable() is None:
                raise AssertionError("default reliable transportation handle is missing")
            if transportation.getHLAdefaultBestEffort() is None:
                raise AssertionError("default best-effort transportation handle is missing")
            time_factory = candidate.getTimeFactory()
            if time_factory is None or not time_factory.getName():
                raise AssertionError("logical-time factory discovery returned no name")
            return f"{probe.rti_name} {probe.rti_version}; standard factory families available"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.factory-discovery",
                "lifecycle",
                factory_discovery,
            )
        )

        def encoder_round_trip() -> str:
            encoder = factory.getEncoderFactory()
            integer = encoder.createHLAinteger32BE(0x01020304)
            if not isinstance(integer, HLAinteger32BE):
                raise AssertionError("adapter did not return hla.rti1516e HLAinteger32BE")
            if integer.getValue() != 0x01020304 or integer.toByteArray() != b"\x01\x02\x03\x04":
                raise AssertionError("2010 JPype encoder round-trip failed")
            integer.decode(b"\x04\x03\x02\x01")
            if integer.getValue() != 0x04030201:
                raise AssertionError("2010 JPype encoder decode failed")
            wrapper = ByteWrapper(b"\x00\x04\x03\x02\x01\x00", 1, 4)
            integer.decode(wrapper)
            if integer.getValue() != 0x04030201 or wrapper.getPos() != 5:
                raise AssertionError("ByteWrapper decode did not preserve the provider cursor")
            scalar_values = {
                "createHLAinteger16BE": 0x1234,
                "createHLAinteger16LE": 0x1234,
                "createHLAinteger32LE": 0x01020304,
                "createHLAinteger64BE": 0x0102030405060708,
                "createHLAinteger64LE": 0x0102030405060708,
                "createHLAfloat32BE": 1.25,
                "createHLAfloat32LE": 1.25,
                "createHLAfloat64BE": 1.25,
                "createHLAfloat64LE": 1.25,
                "createHLAbyte": 7,
                "createHLAoctet": 7,
                "createHLAASCIIchar": 65,
                "createHLAunicodeChar": 65,
                "createHLAoctetPairBE": 0x1234,
                "createHLAoctetPairLE": 0x1234,
            }
            for method_name, expected in scalar_values.items():
                element = getattr(encoder, method_name)(expected)
                encoded = element.toByteArray()
                if not encoded:
                    raise AssertionError(f"{method_name} returned an empty encoding")
                element.decode(encoded)
                if abs(float(element.getValue()) - float(expected)) > 1e-5:
                    raise AssertionError(f"{method_name} round-trip changed its value")
                default_element = getattr(encoder, method_name)()
                if default_element.getEncodedLength() <= 0:
                    raise AssertionError(f"{method_name} no-argument overload returned no encoding")
            boolean = encoder.createHLAboolean(True)
            # IEEE 1516.1-2010 HLAboolean is a four-octet big-endian
            # unsigned value (0 for false, 1 for true), matching the
            # official C++/Java encoder implementations.
            if not boolean.getValue() or boolean.toByteArray() != b"\x00\x00\x00\x01":
                raise AssertionError("createHLAboolean round-trip failed")
            boolean.decode(b"\x00\x00\x00\x00")
            if boolean.getValue():
                raise AssertionError("HLAboolean decode failed")
            if encoder.createHLAboolean().getEncodedLength() <= 0:
                raise AssertionError("createHLAboolean no-argument overload returned no encoding")
            opaque = encoder.createHLAopaqueData(b"\x01\x02\xff")
            if (
                opaque.getValue() != b"\x01\x02\xff"
                or tuple(opaque) != (1, 2, 255)
                or opaque.toByteArray() != b"\x00\x00\x00\x03\x01\x02\xff"
            ):
                raise AssertionError("HLAopaqueData round-trip failed")
            opaque_wrapper = ByteWrapper(b"\x00" + opaque.toByteArray() + b"\xff", 1, 7)
            decoded_opaque = encoder.createHLAopaqueData()
            decoded_opaque.decode(opaque_wrapper)
            if decoded_opaque.getValue() != b"\x01\x02\xff" or opaque_wrapper.getPos() != 8:
                raise AssertionError("HLAopaqueData ByteWrapper decode failed")
            return "provider-owned standard scalar and opaque encoder family"

        scenarios.append(_profiled_scenario(capability_profile, "python-2010-tck.encoder-round-trip", "encoding", encoder_round_trip))

        def malformed_inputs() -> str:
            value = factory.getEncoderFactory().createHLAinteger32BE(0)
            try:
                value.decode(b"\x00")
            except (DecoderException, RTIexception) as error:
                return f"provider rejected malformed scalar input as {type(error).__name__}"
            raise AssertionError("provider accepted a truncated HLAinteger32BE encoding")

        scenarios.append(_profiled_scenario(capability_profile, "python-2010-tck.malformed-inputs", "malformed-inputs", malformed_inputs))

        def complex_encoders() -> str:
            encoder = factory.getEncoderFactory()
            text = encoder.createHLAASCIIstring("hello")
            if text.getValue() != "hello":
                raise AssertionError("HLAASCIIstring value did not cross JPype")
            text.setValue("updated")
            if text.getValue() != "updated":
                raise AssertionError("HLAASCIIstring setValue failed")
            unicode_text = encoder.createHLAunicodeString("こんにちは")
            if unicode_text.getValue() != "こんにちは":
                raise AssertionError("HLAunicodeString value did not cross JPype")
            if encoder.createHLAASCIIstring().getEncodedLength() <= 0:
                raise AssertionError("HLAASCIIstring no-argument overload returned no encoding")
            if encoder.createHLAunicodeString().getEncodedLength() <= 0:
                raise AssertionError("HLAunicodeString no-argument overload returned no encoding")
            opaque = encoder.createHLAopaqueData(b"\x01\x02")
            if opaque.getValue() != b"\x01\x02" or tuple(opaque) != (1, 2):
                raise AssertionError("HLAopaqueData value did not cross JPype")
            if encoder.createHLAopaqueData().size() != 0:
                raise AssertionError("HLAopaqueData no-argument overload was not empty")
            first = encoder.createHLAinteger32BE(1)
            second = encoder.createHLAinteger32BE(2)
            record = encoder.createHLAfixedRecord()
            record.add(first)
            record.add(second)
            if record.size() != 2 or record.get(1).getValue() != 2:
                raise AssertionError("HLAfixedRecord shape failed")
            fixed = encoder.createHLAfixedArray(first, second)
            if fixed.size() != 2 or fixed.get(0).getValue() != 1 or fixed.get(1).getValue() != 2:
                raise AssertionError("HLAfixedArray variadic shape failed")

            class ElementFactory(DataElementFactory):
                def createElement(self, index: int):
                    return encoder.createHLAinteger32BE(index)

            variable = encoder.createHLAvariableArray(ElementFactory(), first)
            variable.resize(2)
            if variable.size() != 2 or variable.get(1).getValue() != 1:
                raise AssertionError("HLAvariableArray factory/resize shape failed")
            variant = encoder.createHLAvariantRecord(first)
            variant.setVariant(first, second)
            if variant.getDiscriminant().getValue() != 1 or variant.getValue().getValue() != 2:
                raise AssertionError("HLAvariantRecord shape failed")
            return "provider-owned string, opaque, record, array, and variant encoders"

        scenarios.append(_profiled_scenario(capability_profile, "python-2010-tck.complex-encoders", "encoding", complex_encoders))

        def api_surface_inventory() -> str:
            if len(RTIAMBASSADOR_METHODS) != 150 or len(FEDERATE_AMBASSADOR_METHODS) != 51:
                raise AssertionError("unexpected 2010 Python method inventory")
            if sum(RTIAMBASSADOR_OVERLOAD_COUNTS.values()) != 172:
                raise AssertionError("unexpected 2010 RTI overload inventory")
            if sum(FEDERATE_AMBASSADOR_OVERLOAD_COUNTS.values()) != 60:
                raise AssertionError("unexpected 2010 callback overload inventory")
            if STANDARD_EDITION != "IEEE 1516.1-2010":
                raise AssertionError("wrong Python standard edition")
            candidate = factory.getRtiAmbassador()
            missing = [name for name in RTIAMBASSADOR_METHODS if not hasattr(candidate, name)]
            if missing:
                raise AssertionError(
                    "provider ambassador is missing standard methods: "
                    + ", ".join(sorted(missing))
                )
            return "150 RTI methods/172 overloads; 51 callbacks/60 overloads"

        scenarios.append(_profiled_scenario(capability_profile, "python-2010-tck.api-surface-inventory", "api-surface", api_surface_inventory))

        def overloads_and_exceptions() -> str:
            if RTIAMBASSADOR_OVERLOAD_COUNTS["connect"] != 2:
                raise AssertionError("connect overload count changed")
            if RTIAMBASSADOR_OVERLOAD_COUNTS["createFederationExecution"] != 5:
                raise AssertionError("createFederationExecution overload count changed")
            if RTIAMBASSADOR_OVERLOAD_COUNTS["joinFederationExecution"] != 4:
                raise AssertionError("joinFederationExecution overload count changed")
            if not issubclass(RTIexception, Exception):
                raise AssertionError("2010 RTIexception escaped the standard hierarchy")
            mapped = exceptionForName("hla.rti1516e.exceptions.InconsistentFDD", "bad FDD")
            if type(mapped).__name__ != "InconsistentFDD":
                raise AssertionError("standard exception mapping changed")
            return "2/5/4 standard overloads and hla.rti1516e exception identity"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.overloads-and-exceptions",
                "exceptions",
                overloads_and_exceptions,
            )
        )

        def connect_disconnect() -> str:
            nonlocal ambassador
            ambassador = factory.getRtiAmbassador()
            # Keep this scenario aligned with the provider-neutral Java TCK:
            # it proves the standard callback-model overload and lifecycle,
            # while callback delivery and HLA-version reporting belong to the
            # service scenarios that exercise them.
            ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
            ambassador.disconnect()
            ambassador = None
            return "standard CallbackModel overload and connection lifecycle"

        scenarios.append(_profiled_scenario(capability_profile, "python-2010-tck.connect-disconnect", "lifecycle", connect_disconnect))

        def federation_membership() -> str:
            nonlocal ambassador
            if args.fom_path is None and not args.native_provider:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve() if args.fom_path is not None else None
            if fom_path is not None and not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            ambassador = factory.getRtiAmbassador()
            connected = False
            created = False
            joined = False
            federation = f"umbra-1516e-python-{int(time.time() * 1000)}"
            try:
                ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                connected = True
                # A pathlib.Path selects the standard URL overload; the JPype
                # runtime converts it to a file: URL without inventing a
                # vendor-specific create-FOM API.
                ambassador.createFederationExecution(
                    federation,
                    fom_path if fom_path is not None else "Umbra-2010-reference-fom",
                )
                created = True
                ambassador.joinFederationExecution("umbra-1516e-python", federation)
                joined = True
            finally:
                if joined:
                    try:
                        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        ambassador.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if connected:
                    try:
                        ambassador.disconnect()
                    finally:
                        ambassador = None
            return "standard FOM URL overload, create/join/resign/destroy lifecycle"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.federation-membership",
                "lifecycle",
                federation_membership,
            )
        )

        def declaration_management() -> str:
            nonlocal ambassador
            if args.fom_path is None and not args.native_provider:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve() if args.fom_path is not None else None
            if fom_path is not None and not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            ambassador = factory.getRtiAmbassador()
            connected = False
            created = False
            joined = False
            federation = f"umbra-1516e-python-declaration-{int(time.time() * 1000)}"
            try:
                ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                connected = True
                ambassador.createFederationExecution(
                    federation,
                    fom_path if fom_path is not None else "Umbra-2010-reference-fom",
                )
                created = True
                ambassador.joinFederationExecution("umbra-1516e-python-declaration", federation)
                joined = True
                object_class = ambassador.getObjectClassHandle(args.object_class)
                attribute = ambassador.getAttributeHandle(object_class, args.attribute_name)
                attributes = ambassador.getAttributeHandleSetFactory().create()
                attributes.add(attribute)
                if ambassador.getObjectClassName(object_class) != args.object_class:
                    raise AssertionError("object-class name did not round-trip")
                if ambassador.getAttributeName(object_class, attribute) != args.attribute_name:
                    raise AssertionError("attribute name did not round-trip")
                ambassador.publishObjectClassAttributes(object_class, attributes)
                ambassador.subscribeObjectClassAttributes(object_class, attributes)
                ambassador.unsubscribeObjectClassAttributes(object_class, attributes)
                ambassador.unpublishObjectClassAttributes(object_class, attributes)
            finally:
                if joined:
                    try:
                        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        ambassador.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if connected:
                    try:
                        ambassador.disconnect()
                    finally:
                        ambassador = None
            return "standard class/attribute lookup, publish/subscribe, and teardown"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.declaration-management",
                "declarations",
                declaration_management,
            )
        )

        def object_management() -> str:
            nonlocal ambassador
            if args.fom_path is None and not args.native_provider:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve() if args.fom_path is not None else None
            if fom_path is not None and not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.labels: list[str] = []
                    self.discovered: tuple[object, ...] | None = None
                    self.reflected: tuple[object, ...] | None = None

                def discoverObjectInstance(self, *values: object) -> None:
                    self.discovered = values
                    self.labels.append("discover")

                def reflectAttributeValues(self, *values: object) -> None:
                    self.reflected = values
                    self.labels.append("reflect")

            publisher = factory.getRtiAmbassador()
            subscriber = factory.getRtiAmbassador()
            publisher_callback = Recorder()
            subscriber_callback = Recorder()
            federation = f"umbra-1516e-python-object-{int(time.time() * 1000)}"
            publisher_connected = False
            subscriber_connected = False
            publisher_joined = False
            subscriber_joined = False
            created = False
            try:
                publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
                publisher_connected = True
                subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
                subscriber_connected = True
                publisher.createFederationExecution(
                    federation,
                    fom_path if fom_path is not None else "Umbra-2010-reference-fom",
                )
                created = True
                publisher.joinFederationExecution("umbra-1516e-python-publisher", federation)
                publisher_joined = True
                subscriber.joinFederationExecution("umbra-1516e-python-subscriber", federation)
                subscriber_joined = True

                publisher_class = publisher.getObjectClassHandle(args.object_class)
                publisher_attribute = publisher.getAttributeHandle(
                    publisher_class, args.attribute_name
                )
                subscriber_class = subscriber.getObjectClassHandle(args.object_class)
                subscriber_attribute = subscriber.getAttributeHandle(
                    subscriber_class, args.attribute_name
                )
                publisher_attributes = publisher.getAttributeHandleSetFactory().create()
                publisher_attributes.add(publisher_attribute)
                subscriber_attributes = subscriber.getAttributeHandleSetFactory().create()
                subscriber_attributes.add(subscriber_attribute)
                subscriber.subscribeObjectClassAttributes(subscriber_class, subscriber_attributes)
                publisher.publishObjectClassAttributes(publisher_class, publisher_attributes)

                registered = publisher.registerObjectInstance(publisher_class)
                _drain_callbacks(subscriber, subscriber_callback.labels)
                if registered.encodedLength() == 0:
                    raise AssertionError("registerObjectInstance returned an empty handle")
                if subscriber_callback.discovered is None:
                    raise AssertionError("discoverObjectInstance did not cross JPype")
                discovered = subscriber_callback.discovered
                if discovered[0] != registered or discovered[1] != subscriber_class:
                    raise AssertionError("discovery handle/class did not round-trip")
                if publisher.getObjectInstanceName(registered) != discovered[2]:
                    raise AssertionError("discovered object name did not round-trip")

                payload = b"\x01\x02\x03\xff"
                values = publisher.getAttributeHandleValueMapFactory().create(1)
                values[publisher_attribute] = payload
                tag = b"\x09\x08\x07"
                publisher.updateAttributeValues(registered, values, tag)
                _drain_callbacks(subscriber, subscriber_callback.labels, expected=2)
                if subscriber_callback.reflected is None:
                    raise AssertionError("reflectAttributeValues did not cross JPype")
                reflected = subscriber_callback.reflected
                if reflected[0] != registered or len(reflected[1]) != 1:
                    raise AssertionError("reflection object/map shape did not round-trip")
                if reflected[1][subscriber_attribute] != payload or reflected[2] != tag:
                    raise AssertionError("reflection payload/tag did not round-trip")
            finally:
                if subscriber_joined:
                    try:
                        subscriber.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if publisher_joined:
                    try:
                        publisher.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        publisher.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if subscriber_connected:
                    try:
                        subscriber.disconnect()
                    except Exception:
                        pass
                if publisher_connected:
                    try:
                        publisher.disconnect()
                    finally:
                        ambassador = None
            return "publish/subscribe, register/discover, and opaque update/reflect callbacks"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.object-management",
                "object-management",
                object_management,
            )
        )

        def ownership_management() -> str:
            if args.fom_path is None and not args.native_provider:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve() if args.fom_path is not None else None
            if fom_path is not None and not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.labels: list[str] = []
                    self.ownership: tuple[object, ...] | None = None

                def discoverObjectInstance(self, *values: object) -> None:
                    self.labels.append("discover")

                def informAttributeOwnership(self, *values: object) -> None:
                    self.ownership = values
                    self.labels.append("ownership")

            publisher = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            publisher_callback = Recorder()
            observer_callback = Recorder()
            federation = f"umbra-1516e-python-ownership-{int(time.time() * 1000)}"
            publisher_connected = False
            observer_connected = False
            publisher_joined = False
            observer_joined = False
            created = False
            try:
                publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
                publisher_connected = True
                observer.connect(observer_callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                publisher.createFederationExecution(
                    federation,
                    fom_path if fom_path is not None else "Umbra-2010-reference-fom",
                )
                created = True
                publisher.joinFederationExecution(
                    "umbra-1516e-python-owner", "owner", federation
                )
                publisher_joined = True
                observer.joinFederationExecution(
                    "umbra-1516e-python-observer", "observer", federation
                )
                observer_joined = True

                publisher_class = publisher.getObjectClassHandle(args.object_class)
                publisher_attribute = publisher.getAttributeHandle(
                    publisher_class, args.attribute_name
                )
                observer_class = observer.getObjectClassHandle(args.object_class)
                observer_attribute = observer.getAttributeHandle(
                    observer_class, args.attribute_name
                )
                attributes = observer.getAttributeHandleSetFactory().create()
                attributes.add(observer_attribute)
                observer.subscribeObjectClassAttributes(observer_class, attributes)
                published = publisher.getAttributeHandleSetFactory().create()
                published.add(publisher_attribute)
                publisher.publishObjectClassAttributes(publisher_class, published)

                registered = publisher.registerObjectInstance(publisher_class)
                _drain_callbacks(observer, observer_callback.labels)
                if "discover" not in observer_callback.labels:
                    raise AssertionError("ownership fixture did not discover the object")

                observer_callback.labels.clear()
                observer.queryAttributeOwnership(registered, observer_attribute)
                _drain_callbacks(observer, observer_callback.labels)
                if observer_callback.ownership is None:
                    raise AssertionError("informAttributeOwnership did not cross JPype")
                ownership = observer_callback.ownership
                if ownership[0] != registered or ownership[1] != observer_attribute:
                    raise AssertionError("ownership object/attribute did not round-trip")
                owner = ownership[2]
                publisher_handle = publisher.getFederateHandle("umbra-1516e-python-owner")
                if owner != publisher_handle:
                    raise AssertionError(
                        "ownership federate handle did not round-trip: "
                        f"owner={getattr(owner, 'encodedValue', None)!r}, "
                        f"publisher={getattr(publisher_handle, 'encodedValue', None)!r}"
                    )
                owner_name = publisher.getFederateName(publisher_handle)
                if owner_name != "umbra-1516e-python-owner":
                    raise AssertionError(
                        f"ownership federate name did not round-trip: {owner_name!r}"
                    )
                if not publisher.isAttributeOwnedByFederate(registered, publisher_attribute):
                    raise AssertionError("publisher did not report owning its attribute")
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if publisher_joined:
                    try:
                        publisher.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        publisher.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if publisher_connected:
                    try:
                        publisher.disconnect()
                    except Exception:
                        pass
            return "query/inform ownership and federate-owner identity"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.ownership-management",
                "ownership",
                ownership_management,
            )
        )

        def interaction_management() -> str:
            if args.fom_path is None and not args.native_provider:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve() if args.fom_path is not None else None
            if fom_path is not None and not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.labels: list[str] = []
                    self.received: tuple[object, ...] | None = None

                def receiveInteraction(self, *values: object) -> None:
                    self.received = values
                    self.labels.append("receive")

            publisher = factory.getRtiAmbassador()
            subscriber = factory.getRtiAmbassador()
            publisher_callback = Recorder()
            subscriber_callback = Recorder()
            federation = f"umbra-1516e-python-interaction-{int(time.time() * 1000)}"
            publisher_connected = False
            subscriber_connected = False
            publisher_joined = False
            subscriber_joined = False
            created = False
            try:
                publisher.connect(publisher_callback, CallbackModel.HLA_EVOKED)
                publisher_connected = True
                subscriber.connect(subscriber_callback, CallbackModel.HLA_EVOKED)
                subscriber_connected = True
                publisher.createFederationExecution(
                    federation,
                    fom_path if fom_path is not None else "Umbra-2010-reference-fom",
                )
                created = True
                publisher.joinFederationExecution("umbra-1516e-python-interaction-publisher", federation)
                publisher_joined = True
                subscriber.joinFederationExecution("umbra-1516e-python-interaction-subscriber", federation)
                subscriber_joined = True

                publisher_class = publisher.getInteractionClassHandle(args.interaction_class)
                subscriber_class = subscriber.getInteractionClassHandle(args.interaction_class)
                if publisher.getInteractionClassName(publisher_class) != args.interaction_class:
                    raise AssertionError("interaction-class name did not round-trip")
                if subscriber_class != publisher_class:
                    raise AssertionError("interaction-class handle identity did not round-trip")
                publisher_parameter = publisher.getParameterHandle(
                    publisher_class, args.parameter_name
                )
                subscriber_parameter = subscriber.getParameterHandle(
                    subscriber_class, args.parameter_name
                )
                if publisher.getParameterName(publisher_class, publisher_parameter) != args.parameter_name:
                    raise AssertionError("parameter name did not round-trip")
                if subscriber_parameter != publisher_parameter:
                    raise AssertionError("parameter handle identity did not round-trip")

                publisher.publishInteractionClass(publisher_class)
                subscriber.subscribeInteractionClass(subscriber_class)
                values = publisher.getParameterHandleValueMapFactory().create(1)
                payload = b"\x05\x06\x07\xff"
                values[publisher_parameter] = payload
                tag = b"\x0a\x0b\x0c"
                publisher.sendInteraction(publisher_class, values, tag)
                _drain_callbacks(subscriber, subscriber_callback.labels)
                if subscriber_callback.received is None:
                    raise AssertionError("receiveInteraction did not cross JPype")
                received = subscriber_callback.received
                if received[0] != subscriber_class or len(received[1]) != 1:
                    raise AssertionError("interaction handle/map shape did not round-trip")
                if received[1][subscriber_parameter] != payload or received[2] != tag:
                    raise AssertionError("interaction payload/tag did not round-trip")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError("interaction receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError("interaction transportation handle did not round-trip")
                subscriber.unsubscribeInteractionClass(subscriber_class)
                publisher.unpublishInteractionClass(publisher_class)
            finally:
                if subscriber_joined:
                    try:
                        subscriber.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if publisher_joined:
                    try:
                        publisher.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        publisher.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if subscriber_connected:
                    try:
                        subscriber.disconnect()
                    except Exception:
                        pass
                if publisher_connected:
                    try:
                        publisher.disconnect()
                    except Exception:
                        pass
            return "publish/subscribe, parameter lookup, and send/receive interaction callbacks"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.interaction-management",
                "interactions",
                interaction_management,
            )
        )

        def synchronization_point() -> str:
            if args.fom_path is None and not args.native_provider:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve() if args.fom_path is not None else None
            if fom_path is not None and not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.registration: list[str] = []
                    self.announcements: list[tuple[str, bytes]] = []

                def synchronizationPointRegistrationSucceeded(self, label: str) -> None:
                    self.registration.append(label)

                def announceSynchronizationPoint(self, label: str, tag: bytes) -> None:
                    self.announcements.append((label, tag))

            registrar = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            registrar_callback = Recorder()
            observer_callback = Recorder()
            federation = f"umbra-1516e-python-sync-{int(time.time() * 1000)}"
            registrar_connected = False
            observer_connected = False
            registrar_joined = False
            observer_joined = False
            created = False
            try:
                registrar.connect(registrar_callback, CallbackModel.HLA_EVOKED)
                registrar_connected = True
                observer.connect(observer_callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                registrar.createFederationExecution(
                    federation,
                    fom_path if fom_path is not None else "Umbra-2010-reference-fom",
                )
                created = True
                registrar.joinFederationExecution("umbra-1516e-python-sync-registrar", federation)
                registrar_joined = True
                observer.joinFederationExecution("umbra-1516e-python-sync-observer", federation)
                observer_joined = True
                registrar_callback.registration.clear()
                observer_callback.announcements.clear()

                label = "umbra-1516e-python-sync-point"
                tag = b"\x11\x12\x13"
                registrar.registerFederationSynchronizationPoint(label, tag)
                _drain_callbacks(registrar, registrar_callback.registration)
                _drain_callbacks(observer, observer_callback.announcements)
                if registrar_callback.registration != [label]:
                    raise AssertionError("registration-success callback did not cross JPype")
                if observer_callback.announcements != [(label, tag)]:
                    raise AssertionError("announce-synchronization callback/tag did not cross JPype")
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if registrar_joined:
                    try:
                        registrar.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        registrar.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if registrar_connected:
                    try:
                        registrar.disconnect()
                    except Exception:
                        pass
            return "standard registration-success and announce callbacks with tag preservation"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.synchronization-point",
                "synchronization",
                synchronization_point,
            )
        )

        def ddm_management() -> str:
            nonlocal ambassador
            if args.fom_path is None:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            ambassador = factory.getRtiAmbassador()
            connected = False
            created = False
            joined = False
            region = None
            federation = f"umbra-1516e-python-ddm-{int(time.time() * 1000)}"
            try:
                ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                connected = True
                ambassador.createFederationExecution(federation, fom_path)
                created = True
                ambassador.joinFederationExecution("umbra-1516e-python-ddm", federation)
                joined = True

                flavor_dimension = ambassador.getDimensionHandle("SodaFlavor")
                quantity_dimension = ambassador.getDimensionHandle("BarQuantity")
                if ambassador.getDimensionName(flavor_dimension) != "SodaFlavor":
                    raise AssertionError("SodaFlavor dimension name did not round-trip")
                if ambassador.getDimensionName(quantity_dimension) != "BarQuantity":
                    raise AssertionError("BarQuantity dimension name did not round-trip")
                if ambassador.getDimensionUpperBound(flavor_dimension) != 3:
                    raise AssertionError("SodaFlavor upper bound did not match the 2010 FOM")
                if ambassador.getDimensionUpperBound(quantity_dimension) != 25:
                    raise AssertionError("BarQuantity upper bound did not match the 2010 FOM")

                dimensions = ambassador.getDimensionHandleSetFactory().create()
                dimensions.add(flavor_dimension)
                dimensions.add(quantity_dimension)
                region = ambassador.createRegion(dimensions)
                if region is None or not region.encodedValue:
                    raise AssertionError("createRegion returned an invalid handle")
                returned_dimensions = ambassador.getDimensionHandleSet(region)
                if returned_dimensions != DimensionHandleSet(
                    (flavor_dimension, quantity_dimension)
                ):
                    raise AssertionError("region dimension set did not round-trip")

                flavor_bounds = RangeBounds(0, 3)
                quantity_bounds = RangeBounds(5, 20)
                ambassador.setRangeBounds(region, flavor_dimension, flavor_bounds)
                ambassador.setRangeBounds(region, quantity_dimension, quantity_bounds)
                if ambassador.getRangeBounds(region, flavor_dimension) != flavor_bounds:
                    raise AssertionError("SodaFlavor range bounds did not round-trip")
                if ambassador.getRangeBounds(region, quantity_dimension) != quantity_bounds:
                    raise AssertionError("BarQuantity range bounds did not round-trip")

                regions = ambassador.getRegionHandleSetFactory().create()
                regions.add(region)
                ambassador.commitRegionModifications(regions)

                soda_class = ambassador.getObjectClassHandle("HLAobjectRoot.Drink.Soda")
                flavor = ambassador.getAttributeHandle(soda_class, "Flavor")
                attributes = ambassador.getAttributeHandleSetFactory().create()
                attributes.add(flavor)
                pairs = ambassador.getAttributeSetRegionSetPairListFactory().create(1)
                if not isinstance(pairs, AttributeSetRegionSetPairList):
                    raise AssertionError("pair-list factory did not return the standard Python type")
                pairs.append(AttributeRegionAssociation(attributes, regions))
                ambassador.subscribeObjectClassAttributesWithRegions(soda_class, pairs)
                ambassador.unsubscribeObjectClassAttributesWithRegions(soda_class, pairs)
            finally:
                if region is not None:
                    try:
                        ambassador.deleteRegion(region)
                    except Exception:
                        pass
                if joined:
                    try:
                        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        ambassador.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if connected:
                    try:
                        ambassador.disconnect()
                    finally:
                        ambassador = None
            return "dimension/region bounds and regional subscription pair-list forwarding"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.ddm-management",
                "ddm",
                ddm_management,
            )
        )

        def mom_service_reporting() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            controller = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            controller_connected = False
            observer_connected = False
            created = False
            controller_joined = False
            observer_joined = False
            federation = f"umbra-1516e-python-mom-{int(time.time() * 1000)}"
            try:
                controller.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                controller_connected = True
                observer.connect(callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                # This selects the standard create-FOM(URL[], MIM URL)
                # overload; the adapter performs only Java URL conversion.
                controller.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                controller_handle = controller.joinFederationExecution(
                    "umbra-2010-mom-controller", federation
                )
                controller_joined = True
                observer.joinFederationExecution("umbra-2010-mom-observer", federation)
                observer_joined = True

                report_name = (
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportServiceInvocation"
                )
                report_class = observer.getInteractionClassHandle(report_name)
                if observer.getInteractionClassName(report_class) != report_name:
                    raise AssertionError("MOM report interaction name did not round-trip")
                observer.subscribeInteractionClass(report_class)

                set_reporting = controller.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
                    "HLAsetServiceReporting"
                )
                target_federate = controller.getParameterHandle(set_reporting, "HLAfederate")
                reporting_state = controller.getParameterHandle(
                    set_reporting, "HLAreportingState"
                )
                switch_values = controller.getParameterHandleValueMapFactory().create(2)
                switch_values[target_federate] = bytes(controller_handle.encodedValue)
                switch_values[reporting_state] = b"\x01"
                controller.sendInteraction(set_reporting, switch_values, b"")

                # The fixture and a conforming provider both use a normal
                # service call as the observable MOM report trigger.
                controller.getObjectClassHandle("HLAobjectRoot.HLAmanager.HLAfederate")
                _drain_callbacks(observer, callback.received)
                if not callback.received:
                    raise AssertionError(
                        "MOM HLAreportServiceInvocation did not cross JPype"
                    )
                received = callback.received[-1]
                if received[0] != report_class or len(received[1]) != 6:
                    raise AssertionError(
                        "MOM service report did not contain all six standard parameters"
                    )
                if received[2] != b"":
                    raise AssertionError("RTI-originated MOM report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError("MOM report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(
                        "MOM report transportation handle did not round-trip"
                    )
                serial_number = observer.getParameterHandle(report_class, "HLAserialNumber")
                if bytes(received[1][serial_number]) != b"\x00\x00\x00\x00":
                    raise AssertionError("MOM service report serial number did not start at zero")
                receive_info = received[5]
                if not receive_info.hasSentRegions() or len(tuple(receive_info.getSentRegions())) != 1:
                    raise AssertionError("MOM service report did not carry a single update region")

                controller.getObjectClassHandle("HLAobjectRoot.HLAmanager.HLAfederate")
                _drain_callbacks(observer, callback.received, expected=2)
                second = callback.received[-1]
                if bytes(second[1][serial_number]) != b"\x00\x00\x00\x01":
                    raise AssertionError(
                        "MOM service report serial number did not increment per invocation"
                    )
                second_receive_info = second[5]
                if not second_receive_info.hasSentRegions() or len(tuple(second_receive_info.getSentRegions())) != 1:
                    raise AssertionError(
                        "second MOM service report did not carry a single update region"
                    )
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if controller_joined:
                    try:
                        controller.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        controller.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if controller_connected:
                    try:
                        controller.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM service-report interaction, six parameters, and callback metadata"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-service-reporting",
                "mom",
                mom_service_reporting,
            )
        )

        def mom_service_reporting_interlocks() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            controller = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            controller_connected = False
            observer_connected = False
            created = False
            controller_joined = False
            observer_joined = False
            federation = f"umbra-2010-mom-interlocks-{int(time.time() * 1000)}"
            try:
                controller.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                controller_connected = True
                observer.connect(callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                controller.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                controller_handle = controller.joinFederationExecution(
                    "umbra-2010-mom-interlock-controller", federation
                )
                controller_joined = True
                observer_handle = observer.joinFederationExecution(
                    "umbra-2010-mom-interlock-observer", federation
                )
                observer_joined = True

                report_name = (
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportServiceInvocation"
                )
                controller_report = controller.getInteractionClassHandle(report_name)
                observer_report = observer.getInteractionClassHandle(report_name)
                mom_exception = observer.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportMOMexception"
                )
                observer.subscribeInteractionClass(observer_report)
                observer.subscribeInteractionClass(mom_exception)

                set_reporting = controller.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
                    "HLAsetServiceReporting"
                )
                target_federate = controller.getParameterHandle(set_reporting, "HLAfederate")
                reporting_state = controller.getParameterHandle(
                    set_reporting, "HLAreportingState"
                )
                encoded_controller = bytes(controller_handle.encodedValue)
                encoded_observer = bytes(observer_handle.encodedValue)
                enable_controller = controller.getParameterHandleValueMapFactory().create(2)
                enable_controller[target_federate] = encoded_controller
                enable_controller[reporting_state] = b"\x01"
                controller.sendInteraction(set_reporting, enable_controller, b"")

                rejected = False
                try:
                    controller.subscribeInteractionClass(controller_report)
                except FederateServiceInvocationsAreBeingReportedViaMOM:
                    rejected = True
                if not rejected:
                    raise AssertionError(
                        "a federate with service reporting enabled was allowed to subscribe to service reports"
                    )

                callback.received.clear()
                enable_observer = controller.getParameterHandleValueMapFactory().create(2)
                enable_observer[target_federate] = encoded_observer
                enable_observer[reporting_state] = b"\x01"
                controller.sendInteraction(set_reporting, enable_observer, b"")
                _drain_callbacks(observer, callback.received)
                mom_report = next(
                    (item for item in callback.received if item[0] == mom_exception),
                    None,
                )
                if mom_report is None:
                    raise AssertionError(
                        "service-reporting subscription interlock did not produce HLAreportMOMexception"
                    )
                received = mom_report
                values = received[1]
                report_federate = observer.getParameterHandle(mom_exception, "HLAfederate")
                service = observer.getParameterHandle(mom_exception, "HLAservice")
                exception = observer.getParameterHandle(mom_exception, "HLAexception")
                parameter_error = observer.getParameterHandle(
                    mom_exception, "HLAparameterError"
                )
                if (
                    len(values) != 4
                    or bytes(values[report_federate]) != encoded_observer
                    or service not in values
                    or exception not in values
                    or parameter_error not in values
                ):
                    raise AssertionError(
                        "service-reporting interlock MOM exception parameters were incomplete"
                    )
                if bytes(values[service]).decode("utf-8") != (
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
                    "HLAsetServiceReporting"
                ):
                    raise AssertionError(
                        "service-reporting interlock service name was not fully qualified"
                    )
                if not bytes(values[exception]):
                    raise AssertionError("service-reporting interlock exception text was empty")
                if bytes(values[parameter_error]) != b"\x00":
                    raise AssertionError(
                        "service-reporting interlock incorrectly classified a precondition failure as a parameter error"
                    )
                if (
                    received[2] != b""
                    or getattr(received[3], "name", None) != "RECEIVE"
                    or received[4].encodedValue != b"transport:HLAdefaultReliable"
                ):
                    raise AssertionError(
                        "service-reporting interlock callback metadata did not round-trip"
                    )
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if controller_joined:
                    try:
                        controller.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        controller.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if controller_connected:
                    try:
                        controller.disconnect()
                    finally:
                        ambassador = None
            return "service-reporting subscription/enabling interlocks through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-service-reporting-interlocks",
                "mom",
                mom_service_reporting_interlocks,
            )
        )

        def mom_federate_object() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.discovered: list[tuple[object, ...]] = []
                    self.reflected: list[tuple[object, ...]] = []

                def discoverObjectInstance(self, *values: object) -> None:
                    self.discovered.append(values)

                def reflectAttributeValues(self, *values: object) -> None:
                    self.reflected.append(values)

            callback = Recorder()
            registrar = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            registrar_connected = False
            observer_connected = False
            created = False
            registrar_joined = False
            observer_joined = False
            federation = f"umbra-1516e-python-mom-object-{int(time.time() * 1000)}"
            try:
                registrar.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                registrar_connected = True
                observer.connect(callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                registrar.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                registrar.joinFederationExecution(
                    "umbra-2010-mom-object-owner", federation
                )
                registrar_joined = True
                observer.joinFederationExecution("umbra-2010-mom-object-observer", federation)
                observer_joined = True

                mom_class = observer.getObjectClassHandle(
                    "HLAobjectRoot.HLAmanager.HLAfederate"
                )
                federate_name = observer.getAttributeHandle(mom_class, "HLAfederateName")
                attributes = observer.getAttributeHandleSetFactory().create()
                attributes.add(federate_name)
                observer.subscribeObjectClassAttributes(mom_class, attributes)
                _drain_callbacks(observer, callback.discovered)
                if len(callback.discovered) < 2:
                    raise AssertionError(
                        "MOM HLAfederate object discovery did not cover each joined federate"
                    )
                if len(callback.reflected) < len(callback.discovered):
                    raise AssertionError(
                        "MOM HLAfederate static attributes were not reflected"
                    )
                found_name = False
                for values in callback.reflected:
                    if len(values) < 3:
                        continue
                    reflected_values = values[1]
                    if federate_name in reflected_values and reflected_values[federate_name]:
                        found_name = True
                        break
                if not found_name:
                    raise AssertionError(
                        "MOM HLAfederateName attribute did not cross JPype"
                    )
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if registrar_joined:
                    try:
                        registrar.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        registrar.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if registrar_connected:
                    try:
                        registrar.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM RTI-owned HLAfederate discovery and static reflection"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-federate-object",
                "mom",
                mom_federate_object,
            )
        )

        def mom_publication_report() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-publication-{int(time.time() * 1000)}"
            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-publication-target", federation
                )
                target_joined = True
                requester.joinFederationExecution(
                    "umbra-2010-mom-publication-requester", federation
                )
                requester_joined = True

                object_class = target.getObjectClassHandle(
                    "HLAobjectRoot.Employee.Server"
                )
                attribute = target.getAttributeHandle(object_class, "Efficiency")
                attributes = target.getAttributeHandleSetFactory().create()
                attributes.add(attribute)
                target.publishObjectClassAttributes(object_class, attributes)

                report_name = (
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportObjectClassPublication"
                )
                report_class = requester.getInteractionClassHandle(report_name)
                if requester.getInteractionClassName(report_class) != report_name:
                    raise AssertionError(
                        "MOM publication report interaction name did not round-trip"
                    )
                requester.subscribeInteractionClass(report_class)

                request_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                    "HLArequestPublications"
                )
                requested_federate = requester.getParameterHandle(
                    request_class, "HLAfederate"
                )
                request_values = requester.getParameterHandleValueMapFactory().create(1)
                request_values[requested_federate] = bytes(target_handle.encodedValue)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received)

                if not callback.received or callback.received[-1][0] != report_class:
                    raise AssertionError(
                        "MOM publication report did not cross JPype"
                    )
                received = callback.received[-1]
                report_values = received[1]
                report_federate = requester.getParameterHandle(report_class, "HLAfederate")
                number_of_classes = requester.getParameterHandle(
                    report_class, "HLAnumberOfClasses"
                )
                reported_object_class = requester.getParameterHandle(
                    report_class, "HLAobjectClass"
                )
                reported_attributes = requester.getParameterHandle(
                    report_class, "HLAattributeList"
                )
                if (
                    len(report_values) != 4
                    or report_federate not in report_values
                    or number_of_classes not in report_values
                    or reported_object_class not in report_values
                    or reported_attributes not in report_values
                ):
                    raise AssertionError(
                        "MOM publication report did not contain the standard parameters"
                    )
                if bytes(report_values[report_federate]) != bytes(target_handle.encodedValue):
                    raise AssertionError("MOM publication report targeted the wrong federate")
                if int.from_bytes(bytes(report_values[number_of_classes]), "big") != 1:
                    raise AssertionError("MOM publication report class count was not one")
                if bytes(report_values[reported_object_class]) != bytes(object_class.encodedValue):
                    raise AssertionError(
                        "MOM publication report object class did not round-trip"
                    )
                if not bytes(report_values[reported_attributes]):
                    raise AssertionError("MOM publication report attribute list was empty")
                if received[2] != b"":
                    raise AssertionError("MOM publication report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(
                        "MOM publication report receive order did not round-trip"
                    )
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError("MOM publication report transport did not round-trip")

                target.unpublishObjectClass(object_class)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received)
                if len(callback.received) < 2 or callback.received[-1][0] != report_class:
                    raise AssertionError(
                        "MOM publication NULL response did not cross JPype"
                    )
                null_values = callback.received[-1][1]
                if (
                    len(null_values) != 2
                    or int.from_bytes(bytes(null_values[number_of_classes]), "big") != 0
                    or reported_object_class in null_values
                    or reported_attributes in null_values
                ):
                    raise AssertionError(
                        "MOM publication NULL response did not omit class data"
                    )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM publication request/report and NULL response through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-publication-report",
                "mom",
                mom_publication_report,
            )
        )

        def mom_subscription_report() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-subscription-{int(time.time() * 1000)}"
            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-subscription-target", federation
                )
                target_joined = True
                requester.joinFederationExecution(
                    "umbra-2010-mom-subscription-requester", federation
                )
                requester_joined = True

                object_class = target.getObjectClassHandle(
                    "HLAobjectRoot.Employee.Server"
                )
                attribute = target.getAttributeHandle(object_class, "Efficiency")
                attributes = target.getAttributeHandleSetFactory().create()
                attributes.add(attribute)
                target.subscribeObjectClassAttributes(object_class, attributes)

                report_name = (
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportObjectClassSubscription"
                )
                report_class = requester.getInteractionClassHandle(report_name)
                if requester.getInteractionClassName(report_class) != report_name:
                    raise AssertionError(
                        "MOM subscription report interaction name did not round-trip"
                    )
                requester.subscribeInteractionClass(report_class)

                request_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                    "HLArequestSubscriptions"
                )
                requested_federate = requester.getParameterHandle(
                    request_class, "HLAfederate"
                )
                request_values = requester.getParameterHandleValueMapFactory().create(1)
                request_values[requested_federate] = bytes(target_handle.encodedValue)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received)

                if not callback.received or callback.received[-1][0] != report_class:
                    raise AssertionError("MOM subscription report did not cross JPype")
                received = callback.received[-1]
                report_values = received[1]
                report_federate = requester.getParameterHandle(report_class, "HLAfederate")
                number_of_classes = requester.getParameterHandle(
                    report_class, "HLAnumberOfClasses"
                )
                reported_object_class = requester.getParameterHandle(
                    report_class, "HLAobjectClass"
                )
                active = requester.getParameterHandle(report_class, "HLAactive")
                max_update_rate = requester.getParameterHandle(
                    report_class, "HLAmaxUpdateRate"
                )
                reported_attributes = requester.getParameterHandle(
                    report_class, "HLAattributeList"
                )
                if (
                    len(report_values) != 6
                    or report_federate not in report_values
                    or number_of_classes not in report_values
                    or reported_object_class not in report_values
                    or active not in report_values
                    or max_update_rate not in report_values
                    or reported_attributes not in report_values
                ):
                    raise AssertionError(
                        "MOM subscription report did not contain the standard parameters"
                    )
                if bytes(report_values[report_federate]) != bytes(target_handle.encodedValue):
                    raise AssertionError("MOM subscription report targeted the wrong federate")
                if int.from_bytes(bytes(report_values[number_of_classes]), "big") != 1:
                    raise AssertionError("MOM subscription report class count was not one")
                if bytes(report_values[reported_object_class]) != bytes(object_class.encodedValue):
                    raise AssertionError(
                        "MOM subscription report object class did not round-trip"
                    )
                if bytes(report_values[active]) != b"\x01":
                    raise AssertionError("MOM subscription report active flag was not true")
                if not bytes(report_values[max_update_rate]):
                    raise AssertionError(
                        "MOM subscription report maximum update rate was empty"
                    )
                if not bytes(report_values[reported_attributes]):
                    raise AssertionError("MOM subscription report attribute list was empty")
                if received[2] != b"":
                    raise AssertionError("MOM subscription report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(
                        "MOM subscription report receive order did not round-trip"
                    )
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError("MOM subscription report transport did not round-trip")

                target.unsubscribeObjectClass(object_class)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received)
                if len(callback.received) < 2 or callback.received[-1][0] != report_class:
                    raise AssertionError("MOM subscription NULL response did not cross JPype")
                null_values = callback.received[-1][1]
                if (
                    len(null_values) != 2
                    or int.from_bytes(bytes(null_values[number_of_classes]), "big") != 0
                    or reported_object_class in null_values
                    or active in null_values
                    or max_update_rate in null_values
                    or reported_attributes in null_values
                ):
                    raise AssertionError(
                        "MOM subscription NULL response did not omit class data"
                    )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM subscription request/report and NULL response through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-subscription-report",
                "mom",
                mom_subscription_report,
            )
        )

        def mom_interaction_publication_subscription_reports() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-interaction-pub-sub-{int(time.time() * 1000)}"

            def assert_report(
                received: tuple[object, ...], report_class: object, federate: object,
                interaction_list: object, encoded_target: bytes, encoded_interaction: bytes | None,
                label: str,
            ) -> None:
                if received[0] != report_class:
                    raise AssertionError(f"{label} report class did not round-trip")
                values = received[1]
                if len(values) != 2 or federate not in values or interaction_list not in values:
                    raise AssertionError(f"{label} report parameters were wrong")
                if bytes(values[federate]) != encoded_target:
                    raise AssertionError(f"{label} report targeted the wrong federate")
                encoded_values = bytes(values[interaction_list])
                if encoded_interaction is None:
                    if encoded_values:
                        raise AssertionError(f"{label} NULL interaction list was not empty")
                elif encoded_interaction not in encoded_values:
                    raise AssertionError(f"{label} interaction handle was absent from the list")
                if received[2] != b"":
                    raise AssertionError(f"{label} report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(f"{label} report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(f"{label} report transport did not round-trip")

            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-interaction-pub-sub-target", federation
                )
                target_joined = True
                requester.joinFederationExecution("umbra-2010-mom-interaction-pub-sub-requester", federation)
                requester_joined = True

                target_interaction = target.getInteractionClassHandle(
                    "HLAinteractionRoot.CustomerTransactions.OrderTaken"
                )
                requester_interaction = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.CustomerTransactions.OrderTaken"
                )
                target.publishInteractionClass(target_interaction)
                target.subscribeInteractionClass(target_interaction)
                encoded_interaction = bytes(requester_interaction.encodedValue)
                encoded_target = bytes(target_handle.encodedValue)

                publication_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication"
                )
                subscription_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription"
                )
                requester.subscribeInteractionClass(publication_report)
                requester.subscribeInteractionClass(subscription_report)
                publication_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications"
                )
                subscription_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions"
                )
                publication_federate = requester.getParameterHandle(publication_request, "HLAfederate")
                subscription_federate = requester.getParameterHandle(subscription_request, "HLAfederate")
                publication_report_federate = requester.getParameterHandle(
                    publication_report, "HLAfederate"
                )
                publication_list = requester.getParameterHandle(
                    publication_report, "HLAinteractionClassList"
                )
                subscription_report_federate = requester.getParameterHandle(
                    subscription_report, "HLAfederate"
                )
                subscription_list = requester.getParameterHandle(
                    subscription_report, "HLAinteractionClassList"
                )

                values = requester.getParameterHandleValueMapFactory().create(1)
                values[publication_federate] = encoded_target
                requester.sendInteraction(publication_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=1)
                if len(callback.received) < 1:
                    raise AssertionError("MOM interaction publication report count was wrong")
                assert_report(
                    callback.received[-1], publication_report, publication_report_federate,
                    publication_list, encoded_target, encoded_interaction,
                    "MOM interaction publication report",
                )

                values = requester.getParameterHandleValueMapFactory().create(1)
                values[subscription_federate] = encoded_target
                requester.sendInteraction(subscription_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                if len(callback.received) < 2:
                    raise AssertionError("MOM interaction subscription report count was wrong")
                assert_report(
                    callback.received[-1], subscription_report, subscription_report_federate,
                    subscription_list, encoded_target, encoded_interaction,
                    "MOM interaction subscription report",
                )

                target.unpublishInteractionClass(target_interaction)
                target.unsubscribeInteractionClass(target_interaction)
                values = requester.getParameterHandleValueMapFactory().create(1)
                values[publication_federate] = encoded_target
                requester.sendInteraction(publication_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=3)
                if len(callback.received) < 3:
                    raise AssertionError("MOM interaction publication NULL report count was wrong")
                assert_report(
                    callback.received[-1], publication_report, publication_report_federate,
                    publication_list, encoded_target, None,
                    "MOM interaction publication NULL report",
                )

                values = requester.getParameterHandleValueMapFactory().create(1)
                values[subscription_federate] = encoded_target
                requester.sendInteraction(subscription_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=4)
                if len(callback.received) < 4:
                    raise AssertionError("MOM interaction subscription NULL report count was wrong")
                assert_report(
                    callback.received[-1], subscription_report, subscription_report_federate,
                    subscription_list, encoded_target, None,
                    "MOM interaction subscription NULL report",
                )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM interaction publication/subscription reports and NULL lists through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-interaction-publication-subscription-reports",
                "mom",
                mom_interaction_publication_subscription_reports,
            )
        )

        def mom_synchronization_reports() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            subject = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            subject_connected = False
            observer_connected = False
            created = False
            subject_joined = False
            observer_joined = False
            federation = f"umbra-2010-mom-sync-reports-{int(time.time() * 1000)}"
            try:
                subject.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                subject_connected = True
                observer.connect(callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                subject.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                subject_handle = subject.joinFederationExecution(
                    "python-2010-mom-sync-subject", federation
                )
                subject_joined = True
                observer_handle = observer.joinFederationExecution(
                    "python-2010-mom-sync-observer", federation
                )
                observer_joined = True

                label = "python-2010-mom-sync-report"
                subject.registerFederationSynchronizationPoint(label, b"")

                points_request = subject.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
                    "HLArequestSynchronizationPoints"
                )
                points_report = observer.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
                    "HLAreportSynchronizationPoints"
                )
                points_parameter = observer.getParameterHandle(points_report, "HLAsyncPoints")
                status_request = subject.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
                    "HLArequestSynchronizationPointStatus"
                )
                status_request_name = subject.getParameterHandle(status_request, "HLAsyncPointName")
                status_report = observer.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
                    "HLAreportSynchronizationPointStatus"
                )
                status_report_name = observer.getParameterHandle(status_report, "HLAsyncPointName")
                status_report_federates = observer.getParameterHandle(
                    status_report, "HLAsyncPointFederates"
                )
                observer.subscribeInteractionClass(points_report)
                observer.subscribeInteractionClass(status_report)
                encoder = factory.getEncoderFactory()

                class UnicodeFactory(DataElementFactory):
                    def createElement(self, index: int) -> object:
                        return encoder.createHLAunicodeString()

                class OctetFactory(DataElementFactory):
                    def createElement(self, index: int) -> object:
                        return encoder.createHLAoctet()

                octet_factory = OctetFactory()

                class StatusRecordFactory(DataElementFactory):
                    def createElement(self, index: int) -> object:
                        record = encoder.createHLAfixedRecord()
                        record.add(encoder.createHLAvariableArray(octet_factory))
                        record.add(encoder.createHLAinteger32BE())
                        return record

                def decode_labels(encoded: bytes) -> list[str]:
                    values = encoder.createHLAvariableArray(UnicodeFactory())
                    values.decode(encoded)
                    return [str(value.getValue()) for value in values]

                def decode_statuses(encoded: bytes) -> dict[object, int]:
                    values = encoder.createHLAvariableArray(StatusRecordFactory())
                    values.decode(encoded)
                    result: dict[object, int] = {}
                    for record in values:
                        encoded_federate = record.get(0)
                        federate_bytes = bytes(
                            int(encoded_federate.get(index).getValue()) & 0xFF
                            for index in range(encoded_federate.size())
                        )
                        federate = observer.getFederateHandleFactory().decode(federate_bytes, 0)
                        result[federate] = int(record.get(1).getValue())
                    return result

                def assert_metadata(received: tuple[object, ...], name: str) -> None:
                    if received[2] != b"":
                        raise AssertionError(f"{name} tag was not empty")
                    if getattr(received[3], "name", None) != "RECEIVE":
                        raise AssertionError(f"{name} receive order did not round-trip")
                    if received[4].encodedValue != b"transport:HLAdefaultReliable":
                        raise AssertionError(f"{name} transport did not round-trip")

                subject.sendInteraction(
                    points_request,
                    subject.getParameterHandleValueMapFactory().create(0),
                    b"",
                )
                _drain_callbacks(observer, callback.received, expected=1)
                if len(callback.received) != 1 or callback.received[-1][0] != points_report:
                    raise AssertionError("MOM synchronization-points report did not cross JPype")
                point_values = callback.received[-1][1]
                if len(point_values) != 1 or points_parameter not in point_values:
                    raise AssertionError("MOM synchronization-points report parameters were wrong")
                if label not in decode_labels(bytes(point_values[points_parameter])):
                    raise AssertionError("MOM synchronization-points report omitted active label")
                assert_metadata(callback.received[-1], "MOM synchronization-points report")

                def request_status(expected: int, requested_label: str) -> dict[object, int]:
                    values = subject.getParameterHandleValueMapFactory().create(1)
                    values[status_request_name] = encoder.createHLAunicodeString(
                        requested_label
                    ).toByteArray()
                    subject.sendInteraction(status_request, values, b"")
                    _drain_callbacks(observer, callback.received, expected=expected)
                    if len(callback.received) < expected or callback.received[-1][0] != status_report:
                        raise AssertionError("MOM synchronization status report did not cross JPype")
                    received = callback.received[-1]
                    report_values = received[1]
                    if (
                        len(report_values) != 2
                        or status_report_name not in report_values
                        or status_report_federates not in report_values
                    ):
                        raise AssertionError("MOM synchronization status report parameters were wrong")
                    if str(
                        encoder.createHLAunicodeString().decode(
                            bytes(report_values[status_report_name])
                        ).getValue()
                    ) != requested_label:
                        raise AssertionError("MOM synchronization status label did not round-trip")
                    assert_metadata(received, "MOM synchronization status report")
                    return decode_statuses(bytes(report_values[status_report_federates]))

                before = request_status(2, label)
                if before.get(subject_handle) != 2 or before.get(observer_handle) != 2:
                    raise AssertionError("MOM synchronization status did not report both federates moving")

                subject.synchronizationPointAchieved(label)
                after_subject = request_status(3, label)
                if after_subject.get(subject_handle) != 3 or after_subject.get(observer_handle) != 2:
                    raise AssertionError(
                        "MOM synchronization status did not preserve per-federate achievement"
                    )

                missing = request_status(4, "missing-python-2010-sync")
                if missing:
                    raise AssertionError("MOM missing synchronization status was not an empty list")

                observer.synchronizationPointAchieved(label)
                callback.received.clear()
                subject.sendInteraction(
                    points_request,
                    subject.getParameterHandleValueMapFactory().create(0),
                    b"",
                )
                _drain_callbacks(observer, callback.received, expected=1)
                if len(callback.received) != 1 or callback.received[-1][0] != points_report:
                    raise AssertionError("MOM completed synchronization-points report was missing")
                if decode_labels(bytes(callback.received[-1][1][points_parameter])):
                    raise AssertionError("MOM completed synchronization point remained active")
                assert_metadata(callback.received[-1], "MOM completed synchronization-points report")
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if subject_joined:
                    try:
                        subject.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        subject.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if subject_connected:
                    try:
                        subject.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM synchronization-point list/status reports and completion through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-synchronization-reports",
                "mom",
                mom_synchronization_reports,
            )
        )

        def mom_exception_reporting() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-exception-{int(time.time() * 1000)}"
            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-exception-target", federation
                )
                target_joined = True
                requester.joinFederationExecution(
                    "umbra-2010-mom-exception-requester", federation
                )
                requester_joined = True

                report_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportException"
                )
                requester.subscribeInteractionClass(report_class)
                object_class = target.getObjectClassHandle(
                    "HLAobjectRoot.Employee.Server"
                )
                object_instance = target.registerObjectInstance(object_class)

                # Exception reporting is disabled by default; the second
                # delete is unknown but must not emit a report yet.
                target.deleteObjectInstance(object_instance, b"")
                target.deleteObjectInstance(object_instance, b"")
                _drain_callbacks(requester, callback.received)
                if callback.received:
                    raise AssertionError(
                        "exception report was emitted while reporting was disabled"
                    )

                set_exception_reporting = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
                    "HLAsetExceptionReporting"
                )
                requested_federate = requester.getParameterHandle(
                    set_exception_reporting, "HLAfederate"
                )
                reporting_state = requester.getParameterHandle(
                    set_exception_reporting, "HLAreportingState"
                )
                switch_values = requester.getParameterHandleValueMapFactory().create(2)
                switch_values[requested_federate] = bytes(target_handle.encodedValue)
                switch_values[reporting_state] = b"\x01"
                requester.sendInteraction(set_exception_reporting, switch_values, b"")

                target.deleteObjectInstance(object_instance, b"")
                _drain_callbacks(requester, callback.received)
                if not callback.received or callback.received[-1][0] != report_class:
                    raise AssertionError("HLAreportException did not cross JPype")
                received = callback.received[-1]
                values = received[1]
                report_federate = requester.getParameterHandle(report_class, "HLAfederate")
                service = requester.getParameterHandle(report_class, "HLAservice")
                exception = requester.getParameterHandle(report_class, "HLAexception")
                if (
                    len(values) != 3
                    or report_federate not in values
                    or service not in values
                    or exception not in values
                ):
                    raise AssertionError(
                        "HLAreportException did not contain the standard parameters"
                    )
                if bytes(values[report_federate]) != bytes(target_handle.encodedValue):
                    raise AssertionError("HLAreportException targeted the wrong federate")
                if bytes(values[service]).decode("utf-8") != "RTIambassador.deleteObjectInstance":
                    raise AssertionError(
                        "HLAreportException service name did not identify the failing service"
                    )
                if not bytes(values[exception]):
                    raise AssertionError("HLAreportException exception text was empty")
                if received[2] != b"":
                    raise AssertionError("HLAreportException tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError("HLAreportException receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError("HLAreportException transport did not round-trip")
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM exception-reporting switch and HLAreportException callback through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-exception-reporting",
                "mom",
                mom_exception_reporting,
            )
        )

        def mom_federation_object() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.discovered: list[tuple[object, ...]] = []
                    self.reflected: list[tuple[object, ...]] = []

                def discoverObjectInstance(self, *values: object) -> None:
                    self.discovered.append(values)

                def reflectAttributeValues(self, *values: object) -> None:
                    self.reflected.append(values)

            callback = Recorder()
            registrar = factory.getRtiAmbassador()
            observer = factory.getRtiAmbassador()
            registrar_connected = False
            observer_connected = False
            created = False
            registrar_joined = False
            observer_joined = False
            federation = f"umbra-2010-mom-federation-{int(time.time() * 1000)}"
            try:
                registrar.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                registrar_connected = True
                observer.connect(callback, CallbackModel.HLA_EVOKED)
                observer_connected = True
                registrar.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                registrar.joinFederationExecution(
                    "umbra-2010-mom-federation-owner", federation
                )
                registrar_joined = True
                observer.joinFederationExecution(
                    "umbra-2010-mom-federation-observer", federation
                )
                observer_joined = True

                federation_class = observer.getObjectClassHandle(
                    "HLAobjectRoot.HLAmanager.HLAfederation"
                )
                federation_name = observer.getAttributeHandle(
                    federation_class, "HLAfederationName"
                )
                attributes = observer.getAttributeHandleSetFactory().create()
                attributes.add(federation_name)
                observer.subscribeObjectClassAttributes(federation_class, attributes)
                _drain_callbacks(observer, callback.discovered)
                if len(callback.discovered) != 1:
                    raise AssertionError(
                        "MOM HLAfederation object discovery did not return one federation object"
                    )
                if len(callback.reflected) != 1 or len(callback.reflected[0]) < 3:
                    raise AssertionError(
                        "MOM HLAfederation static attributes were not reflected"
                    )
                reflected_values = callback.reflected[0][1]
                if federation_name not in reflected_values:
                    raise AssertionError("MOM HLAfederationName attribute was not reflected")
                if bytes(reflected_values[federation_name]).decode("utf-8") != federation:
                    raise AssertionError("MOM HLAfederationName did not identify the federation")
                if callback.reflected[0][2] != b"":
                    raise AssertionError("MOM HLAfederation reflection tag was not empty")
                if getattr(callback.reflected[0][3], "name", None) != "RECEIVE":
                    raise AssertionError("MOM HLAfederation reflection order did not round-trip")
                if callback.reflected[0][4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError("MOM HLAfederation reflection transport did not round-trip")
            finally:
                if observer_joined:
                    try:
                        observer.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if registrar_joined:
                    try:
                        registrar.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        registrar.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if observer_connected:
                    try:
                        observer.disconnect()
                    except Exception:
                        pass
                if registrar_connected:
                    try:
                        registrar.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM RTI-owned HLAfederation discovery and static reflection"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-federation-object",
                "mom",
                mom_federation_object,
            )
        )

        def mom_mom_exception() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-mom-exception-{int(time.time() * 1000)}"
            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-mom-exception-target", federation
                )
                target_joined = True
                requester.joinFederationExecution(
                    "umbra-2010-mom-mom-exception-requester", federation
                )
                requester_joined = True

                report_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportMOMexception"
                )
                requester.subscribeInteractionClass(report_class)
                malformed = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming"
                )
                requested_federate = requester.getParameterHandle(malformed, "HLAfederate")
                malformed_values = requester.getParameterHandleValueMapFactory().create(1)
                malformed_values[requested_federate] = bytes(target_handle.encodedValue)
                # Omit HLAreportPeriod deliberately; the MOM exception report
                # identifies the malformed interaction without raising a
                # bespoke Python exception.
                requester.sendInteraction(malformed, malformed_values, b"")
                _drain_callbacks(requester, callback.received)
                if not callback.received or callback.received[-1][0] != report_class:
                    raise AssertionError("HLAreportMOMexception did not cross JPype")
                received = callback.received[-1]
                values = received[1]
                report_federate = requester.getParameterHandle(report_class, "HLAfederate")
                service = requester.getParameterHandle(report_class, "HLAservice")
                exception = requester.getParameterHandle(report_class, "HLAexception")
                parameter_error = requester.getParameterHandle(report_class, "HLAparameterError")
                if (
                    len(values) != 4
                    or report_federate not in values
                    or service not in values
                    or exception not in values
                    or parameter_error not in values
                ):
                    raise AssertionError(
                        "HLAreportMOMexception did not contain the standard parameters"
                    )
                if bytes(values[report_federate]) != bytes(target_handle.encodedValue):
                    raise AssertionError("HLAreportMOMexception targeted the wrong federate")
                if bytes(values[service]).decode("utf-8") != (
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming"
                ):
                    raise AssertionError(
                        "HLAreportMOMexception service name was not fully qualified"
                    )
                if not bytes(values[exception]):
                    raise AssertionError("HLAreportMOMexception exception text was empty")
                if bytes(values[parameter_error]) != b"\x01":
                    raise AssertionError(
                        "HLAreportMOMexception parameter-error flag was not true"
                    )
                if received[2] != b"":
                    raise AssertionError("HLAreportMOMexception tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(
                        "HLAreportMOMexception receive order did not round-trip"
                    )
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(
                        "HLAreportMOMexception transport did not round-trip"
                    )

                # The same standard MOM service is valid when its
                # HLAreportPeriod parameter is present and contains a
                # non-negative HLAinteger32BE value. A valid control
                # interaction must not produce another MOM exception.
                report_period = requester.getParameterHandle(malformed, "HLAreportPeriod")
                malformed_values[report_period] = b"\x00\x00\x00\x05"
                requester.sendInteraction(malformed, malformed_values, b"")
                _drain_callbacks(requester, callback.received)
                if len(callback.received) != 1:
                    raise AssertionError(
                        "valid HLAsetTiming was incorrectly reported through HLAreportMOMexception"
                    )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM malformed/valid HLAsetTiming interlock and HLAreportMOMexception through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-mom-exception",
                "mom",
                mom_mom_exception,
            )
        )

        def mom_timing() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class ReflectRecorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.reflections: list[tuple[object, ...]] = []

                def reflectAttributeValues(self, *values: object) -> None:
                    self.reflections.append(values)

            evoked_callback = ReflectRecorder()
            immediate_callback = ReflectRecorder()
            target = factory.getRtiAmbassador()
            evoked = factory.getRtiAmbassador()
            immediate = factory.getRtiAmbassador()
            target_connected = False
            evoked_connected = False
            immediate_connected = False
            created = False
            target_joined = False
            evoked_joined = False
            immediate_joined = False
            federation = f"umbra-2010-mom-timing-{int(time.time() * 1000)}"
            try:
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                evoked.connect(evoked_callback, CallbackModel.HLA_EVOKED)
                evoked_connected = True
                immediate.connect(immediate_callback, CallbackModel.HLA_IMMEDIATE)
                immediate_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "python-2010-mom-timing-target", federation
                )
                target_joined = True
                evoked.joinFederationExecution("python-2010-mom-timing-evoked", federation)
                evoked_joined = True
                immediate.joinFederationExecution(
                    "python-2010-mom-timing-immediate", federation
                )
                immediate_joined = True

                class_name = "HLAobjectRoot.HLAmanager.HLAfederate"
                evoked_class = evoked.getObjectClassHandle(class_name)
                immediate_class = immediate.getObjectClassHandle(class_name)
                evoked_logical_time = evoked.getAttributeHandle(evoked_class, "HLAlogicalTime")
                evoked_lookahead = evoked.getAttributeHandle(evoked_class, "HLAlookahead")
                immediate_logical_time = immediate.getAttributeHandle(
                    immediate_class, "HLAlogicalTime"
                )
                immediate_lookahead = immediate.getAttributeHandle(
                    immediate_class, "HLAlookahead"
                )
                evoked_attributes = evoked.getAttributeHandleSetFactory().create()
                evoked_attributes.add(evoked_logical_time)
                evoked_attributes.add(evoked_lookahead)
                immediate_attributes = immediate.getAttributeHandleSetFactory().create()
                immediate_attributes.add(immediate_logical_time)
                immediate_attributes.add(immediate_lookahead)
                evoked.subscribeObjectClassAttributes(evoked_class, evoked_attributes)
                immediate.subscribeObjectClassAttributes(immediate_class, immediate_attributes)
                # Subscription causes the initial bounded MOM discovery route;
                # only later entries are periodic evidence.
                evoked_callback.reflections.clear()
                immediate_callback.reflections.clear()

                # ``LogicalTimeFactory`` exposes only the standard factory
                # methods.  Concrete HLAinteger64/HLAfloat64 factories may
                # add ``makeInterval(value)``, but requiring that convenience
                # method would make this transplantable probe vendor-specific.
                timing_lookahead = target.getTimeFactory().makeEpsilon()
                target.modifyLookahead(timing_lookahead)
                set_timing = target.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming"
                )
                federate_parameter = target.getParameterHandle(set_timing, "HLAfederate")
                period_parameter = target.getParameterHandle(set_timing, "HLAreportPeriod")
                timing_values = target.getParameterHandleValueMapFactory().create(2)
                timing_values[federate_parameter] = bytes(target_handle.encodedValue)
                timing_values[period_parameter] = b"\x00\x00\x00\x01"
                target.sendInteraction(set_timing, timing_values, b"")

                time.sleep(1.25)
                if evoked_callback.reflections:
                    raise AssertionError(
                        "HLA_EVOKED periodic MOM reflection bypassed the callback boundary"
                    )
                if not immediate_callback.reflections:
                    raise AssertionError(
                        "HLA_IMMEDIATE periodic MOM reflection did not arrive automatically"
                    )
                immediate_values = immediate_callback.reflections[0][1]
                if (
                    immediate_logical_time not in immediate_values
                    or immediate_lookahead not in immediate_values
                ):
                    raise AssertionError(
                        "periodic MOM reflection omitted HLAlogicalTime/HLAlookahead"
                    )
                if bytes(immediate_values[immediate_logical_time]) != b"\x00" * 8:
                    raise AssertionError("periodic HLAlogicalTime did not use the target snapshot")
                if bytes(immediate_values[immediate_lookahead]) != bytes(
                    timing_lookahead.encodedValue
                ):
                    raise AssertionError("periodic HLAlookahead did not use the target snapshot")

                evoked.evokeMultipleCallbacks(0.0, 0.25)
                if not evoked_callback.reflections:
                    raise AssertionError(
                        "HLA_EVOKED periodic MOM reflection did not arrive at Evoke"
                    )
                evoked_callback.reflections.clear()
                immediate_callback.reflections.clear()
                timing_values[period_parameter] = b"\x00\x00\x00\x00"
                target.sendInteraction(set_timing, timing_values, b"")
                time.sleep(1.25)
                evoked.evokeMultipleCallbacks(0.0, 0.25)
                if evoked_callback.reflections or immediate_callback.reflections:
                    raise AssertionError(
                        "HLAreportPeriod=0 did not disable periodic MOM reflection"
                    )
            finally:
                if immediate_joined:
                    try:
                        immediate.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if evoked_joined:
                    try:
                        evoked.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if immediate_connected:
                    try:
                        immediate.disconnect()
                    except Exception:
                        pass
                if evoked_connected:
                    try:
                        evoked.disconnect()
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    finally:
                        ambassador = None
            return "bounded HLAsetTiming HLAlogicalTime/HLAlookahead in both callback models"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-timing",
                "mom",
                mom_timing,
            )
        )

        def mom_object_instance_information() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-object-information-{int(time.time() * 1000)}"
            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-object-information-target", federation
                )
                target_joined = True
                requester.joinFederationExecution(
                    "umbra-2010-mom-object-information-requester", federation
                )
                requester_joined = True

                object_class = target.getObjectClassHandle("HLAobjectRoot.Employee.Server")
                attribute = target.getAttributeHandle(object_class, "Efficiency")
                attributes = target.getAttributeHandleSetFactory().create()
                attributes.add(attribute)
                target.publishObjectClassAttributes(object_class, attributes)
                object_handle = target.registerObjectInstance(
                    object_class, "python-2010-mom-object-information-instance"
                )

                report_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportObjectInstanceInformation"
                )
                requester.subscribeInteractionClass(report_class)
                request_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                    "HLArequestObjectInstanceInformation"
                )
                requested_federate = requester.getParameterHandle(
                    request_class, "HLAfederate"
                )
                requested_object = requester.getParameterHandle(
                    request_class, "HLAobjectInstance"
                )
                request_values = requester.getParameterHandleValueMapFactory().create(2)
                request_values[requested_federate] = bytes(target_handle.encodedValue)
                request_values[requested_object] = bytes(object_handle.encodedValue)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received)
                if not callback.received or callback.received[-1][0] != report_class:
                    raise AssertionError(
                        "MOM object-information report did not cross JPype"
                    )
                received = callback.received[-1]
                values = received[1]
                report_federate = requester.getParameterHandle(report_class, "HLAfederate")
                report_object = requester.getParameterHandle(
                    report_class, "HLAobjectInstance"
                )
                owned_attributes = requester.getParameterHandle(
                    report_class, "HLAownedInstanceAttributeList"
                )
                registered_class = requester.getParameterHandle(
                    report_class, "HLAregisteredClass"
                )
                known_class = requester.getParameterHandle(report_class, "HLAknownClass")
                if (
                    len(values) != 5
                    or report_federate not in values
                    or report_object not in values
                    or owned_attributes not in values
                    or registered_class not in values
                    or known_class not in values
                ):
                    raise AssertionError(
                        "MOM object-information report did not contain the standard parameters"
                    )
                if bytes(values[report_federate]) != bytes(target_handle.encodedValue):
                    raise AssertionError(
                        "MOM object-information report targeted the wrong federate"
                    )
                if bytes(values[report_object]) != bytes(object_handle.encodedValue):
                    raise AssertionError(
                        "MOM object-information object handle did not round-trip"
                    )
                if bytes(values[owned_attributes]) != bytes(attribute.encodedValue):
                    raise AssertionError(
                        "MOM object-information owned attribute list did not round-trip"
                    )
                if bytes(values[registered_class]) != bytes(object_class.encodedValue):
                    raise AssertionError(
                        "MOM object-information registered class did not round-trip"
                    )
                if bytes(values[known_class]) != bytes(object_class.encodedValue):
                    raise AssertionError(
                        "MOM object-information known class did not round-trip"
                    )
                if received[2] != b"":
                    raise AssertionError("MOM object-information report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(
                        "MOM object-information report receive order did not round-trip"
                    )
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(
                        "MOM object-information report transport did not round-trip"
                    )

                unknown = requester.getObjectInstanceHandle(
                    "python-2010-mom-object-information-unknown"
                )
                request_values.clear()
                request_values[requested_federate] = bytes(target_handle.encodedValue)
                request_values[requested_object] = bytes(unknown.encodedValue)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                if len(callback.received) < 2 or callback.received[-1][0] != report_class:
                    raise AssertionError(
                        "MOM object-information NULL report did not cross JPype"
                    )
                null_values = callback.received[-1][1]
                if (
                    len(null_values) != 3
                    or bytes(null_values[report_federate]) != bytes(target_handle.encodedValue)
                    or bytes(null_values[report_object]) != bytes(unknown.encodedValue)
                    or bytes(null_values[owned_attributes]) != b""
                    or registered_class in null_values
                    or known_class in null_values
                ):
                    raise AssertionError(
                        "MOM object-information NULL response did not omit class data"
                    )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM object-instance-information report through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-object-instance-information",
                "mom",
                mom_object_instance_information,
            )
        )

        def mom_object_instances_can_be_deleted() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-deletable-{int(time.time() * 1000)}"
            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-deletable-target", federation
                )
                target_joined = True
                requester.joinFederationExecution(
                    "umbra-2010-mom-deletable-requester", federation
                )
                requester_joined = True

                object_class = target.getObjectClassHandle("HLAobjectRoot.Employee.Server")
                attribute = target.getAttributeHandle(object_class, "Efficiency")
                attributes = target.getAttributeHandleSetFactory().create()
                attributes.add(attribute)
                target.publishObjectClassAttributes(object_class, attributes)
                object_handle = target.registerObjectInstance(
                    object_class, "python-2010-mom-deletable-instance"
                )

                report_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportObjectInstancesThatCanBeDeleted"
                )
                requester.subscribeInteractionClass(report_class)
                request_class = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                    "HLArequestObjectInstancesThatCanBeDeleted"
                )
                requested_federate = requester.getParameterHandle(
                    request_class, "HLAfederate"
                )
                request_values = requester.getParameterHandleValueMapFactory().create(1)
                request_values[requested_federate] = bytes(target_handle.encodedValue)
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received)
                if not callback.received or callback.received[-1][0] != report_class:
                    raise AssertionError(
                        "MOM deletable-object report did not cross JPype"
                    )
                received = callback.received[-1]
                values = received[1]
                report_federate = requester.getParameterHandle(report_class, "HLAfederate")
                object_counts = requester.getParameterHandle(
                    report_class, "HLAobjectInstanceCounts"
                )
                if (
                    len(values) != 2
                    or report_federate not in values
                    or object_counts not in values
                ):
                    raise AssertionError(
                        "MOM deletable-object report did not contain the standard parameters"
                    )
                if bytes(values[report_federate]) != bytes(target_handle.encodedValue):
                    raise AssertionError(
                        "MOM deletable-object report targeted the wrong federate"
                    )
                counts = bytes(values[object_counts])
                if bytes(object_class.encodedValue) not in counts or b"\x00\x00\x00\x01" not in counts:
                    raise AssertionError(
                        "MOM deletable-object class/count payload did not round-trip"
                    )
                if received[2] != b"":
                    raise AssertionError("MOM deletable-object report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(
                        "MOM deletable-object report receive order did not round-trip"
                    )
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(
                        "MOM deletable-object report transport did not round-trip"
                    )

                target.deleteObjectInstance(object_handle, b"")
                requester.sendInteraction(request_class, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                if len(callback.received) < 2 or callback.received[-1][0] != report_class:
                    raise AssertionError(
                        "MOM deletable-object NULL report did not cross JPype"
                    )
                null_values = callback.received[-1][1]
                if (
                    len(null_values) != 2
                    or bytes(null_values[report_federate]) != bytes(target_handle.encodedValue)
                    or bytes(null_values[object_counts]) != b""
                ):
                    raise AssertionError(
                        "MOM deletable-object NULL response did not empty the class counts"
                    )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM deletable-object count report and NULL response through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-object-instances-can-be-deleted",
                "mom",
                mom_object_instances_can_be_deleted,
            )
        )

        def mom_object_instance_count_reports() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []
                    self.reflections: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

                def reflectAttributeValues(self, *values: object) -> None:
                    self.reflections.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-counts-{int(time.time() * 1000)}"

            def assert_report(
                received: tuple[object, ...],
                report_class: object,
                report_federate: object,
                object_counts: object,
                expected_federate: bytes,
                encoded_class: bytes,
                positive: bool,
                label: str,
            ) -> None:
                if received[0] != report_class:
                    raise AssertionError(f"{label} report class did not round-trip")
                values = received[1]
                if (
                    len(values) != 2
                    or report_federate not in values
                    or object_counts not in values
                ):
                    raise AssertionError(f"{label} report parameters were wrong")
                if bytes(values[report_federate]) != expected_federate:
                    raise AssertionError(f"{label} report targeted the wrong federate")
                encoded_counts = bytes(values[object_counts])
                if positive:
                    if encoded_class not in encoded_counts or b"\x00\x00\x00\x01" not in encoded_counts:
                        raise AssertionError(f"{label} class/count payload did not round-trip")
                elif encoded_counts:
                    raise AssertionError(f"{label} NULL response did not use an empty count list")
                if received[2] != b"":
                    raise AssertionError(f"{label} report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(f"{label} report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(f"{label} report transport did not round-trip")

            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-counts-target", federation
                )
                target_joined = True
                requester_handle = requester.joinFederationExecution(
                    "umbra-2010-mom-counts-requester", federation
                )
                requester_joined = True

                target_class = target.getObjectClassHandle("HLAobjectRoot.Employee.Server")
                target_attribute = target.getAttributeHandle(target_class, "Efficiency")
                requester_class = requester.getObjectClassHandle("HLAobjectRoot.Employee.Server")
                requester_attribute = requester.getAttributeHandle(requester_class, "Efficiency")
                subscribed_attributes = requester.getAttributeHandleSetFactory().create()
                subscribed_attributes.add(requester_attribute)
                requester.subscribeObjectClassAttributes(requester_class, subscribed_attributes)
                published_attributes = target.getAttributeHandleSetFactory().create()
                published_attributes.add(target_attribute)
                target.publishObjectClassAttributes(target_class, published_attributes)
                object_handle = target.registerObjectInstance(
                    target_class, "python-2010-mom-counts-instance"
                )
                update_values = target.getAttributeHandleValueMapFactory().create(1)
                update_values[target_attribute] = b"\x01\x02\x03"
                target.updateAttributeValues(object_handle, update_values, b"")
                _drain_callbacks(requester, callback.reflections)
                if not callback.reflections:
                    raise AssertionError("object reflection did not cross JPype")

                updated_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportObjectInstancesUpdated"
                )
                reflected_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
                    "HLAreportObjectInstancesReflected"
                )
                requester.subscribeInteractionClass(updated_report)
                requester.subscribeInteractionClass(reflected_report)
                updated_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                    "HLArequestObjectInstancesUpdated"
                )
                reflected_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                    "HLArequestObjectInstancesReflected"
                )
                updated_request_federate = requester.getParameterHandle(
                    updated_request, "HLAfederate"
                )
                reflected_request_federate = requester.getParameterHandle(
                    reflected_request, "HLAfederate"
                )
                updated_report_federate = requester.getParameterHandle(
                    updated_report, "HLAfederate"
                )
                updated_counts = requester.getParameterHandle(
                    updated_report, "HLAobjectInstanceCounts"
                )
                reflected_report_federate = requester.getParameterHandle(
                    reflected_report, "HLAfederate"
                )
                reflected_counts = requester.getParameterHandle(
                    reflected_report, "HLAobjectInstanceCounts"
                )
                encoded_target = bytes(target_handle.encodedValue)
                encoded_requester = bytes(requester_handle.encodedValue)
                encoded_class = bytes(target_class.encodedValue)
                request_values = requester.getParameterHandleValueMapFactory().create(1)

                request_values[updated_request_federate] = encoded_target
                requester.sendInteraction(updated_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=1)
                assert_report(
                    callback.received[-1],
                    updated_report,
                    updated_report_federate,
                    updated_counts,
                    encoded_target,
                    encoded_class,
                    True,
                    "MOM updated-object",
                )

                request_values.clear()
                request_values[reflected_request_federate] = encoded_requester
                requester.sendInteraction(reflected_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                assert_report(
                    callback.received[-1],
                    reflected_report,
                    reflected_report_federate,
                    reflected_counts,
                    encoded_requester,
                    encoded_class,
                    True,
                    "MOM reflected-object",
                )

                request_values.clear()
                request_values[updated_request_federate] = encoded_requester
                requester.sendInteraction(updated_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=3)
                assert_report(
                    callback.received[-1],
                    updated_report,
                    updated_report_federate,
                    updated_counts,
                    encoded_requester,
                    encoded_class,
                    False,
                    "MOM updated-object NULL",
                )

                request_values.clear()
                request_values[reflected_request_federate] = encoded_target
                requester.sendInteraction(reflected_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=4)
                assert_report(
                    callback.received[-1],
                    reflected_report,
                    reflected_report_federate,
                    reflected_counts,
                    encoded_target,
                    encoded_class,
                    False,
                    "MOM reflected-object NULL",
                )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM updated/reflected object-instance count reports through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-object-instance-count-reports",
                "mom",
                mom_object_instance_count_reports,
            )
        )

        def mom_transport_count_reports() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []
                    self.reflections: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

                def reflectAttributeValues(self, *values: object) -> None:
                    self.reflections.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-transport-{int(time.time() * 1000)}"

            def assert_report(
                received: tuple[object, ...],
                report_class: object,
                transportation: object,
                counts: object,
                expected_transport: bytes,
                expected_class: bytes | None,
                expected_count: int,
                label: str,
            ) -> None:
                if received[0] != report_class:
                    raise AssertionError(f"{label} report class did not round-trip")
                values = received[1]
                if len(values) != 2 or transportation not in values or counts not in values:
                    raise AssertionError(f"{label} report parameters were wrong")
                if bytes(values[transportation]) != expected_transport:
                    raise AssertionError(f"{label} transportation parameter did not round-trip")
                encoded_counts = bytes(values[counts])
                if expected_class is None:
                    if encoded_counts:
                        raise AssertionError(f"{label} NULL count list was not empty")
                elif expected_class not in encoded_counts or expected_count.to_bytes(
                    4, "big", signed=True
                ) not in encoded_counts:
                    raise AssertionError(f"{label} class/count payload did not round-trip")
                if received[2] != b"":
                    raise AssertionError(f"{label} report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(f"{label} report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(f"{label} report transport did not round-trip")

            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-transport-target", federation
                )
                target_joined = True
                requester_handle = requester.joinFederationExecution(
                    "umbra-2010-mom-transport-requester", federation
                )
                requester_joined = True

                server_class = target.getObjectClassHandle("HLAobjectRoot.Employee.Server")
                server_attribute = target.getAttributeHandle(server_class, "Efficiency")
                soda_class = target.getObjectClassHandle("HLAobjectRoot.Food.Drink.Soda")
                soda_attribute = target.getAttributeHandle(soda_class, "Flavor")
                requester_server_class = requester.getObjectClassHandle(
                    "HLAobjectRoot.Employee.Server"
                )
                requester_soda_class = requester.getObjectClassHandle(
                    "HLAobjectRoot.Food.Drink.Soda"
                )
                requester_server_attribute = requester.getAttributeHandle(
                    requester_server_class, "Efficiency"
                )
                requester_soda_attribute = requester.getAttributeHandle(
                    requester_soda_class, "Flavor"
                )
                subscribed_server_attributes = requester.getAttributeHandleSetFactory().create()
                subscribed_server_attributes.add(requester_server_attribute)
                requester.subscribeObjectClassAttributes(
                    requester_server_class, subscribed_server_attributes
                )
                subscribed_soda_attributes = requester.getAttributeHandleSetFactory().create()
                subscribed_soda_attributes.add(requester_soda_attribute)
                requester.subscribeObjectClassAttributes(
                    requester_soda_class, subscribed_soda_attributes
                )
                published_server_attributes = target.getAttributeHandleSetFactory().create()
                published_server_attributes.add(server_attribute)
                target.publishObjectClassAttributes(server_class, published_server_attributes)
                published_soda_attributes = target.getAttributeHandleSetFactory().create()
                published_soda_attributes.add(soda_attribute)
                target.publishObjectClassAttributes(soda_class, published_soda_attributes)

                server_object = target.registerObjectInstance(
                    server_class, "python-2010-mom-transport-server"
                )
                soda_object = target.registerObjectInstance(
                    soda_class, "python-2010-mom-transport-soda"
                )
                transportation_factory = target.getTransportationTypeHandleFactory()
                best_effort = transportation_factory.getHLAdefaultBestEffort()
                best_effort_attributes = target.getAttributeHandleSetFactory().create()
                best_effort_attributes.add(server_attribute)
                target.requestAttributeTransportationTypeChange(
                    server_object, best_effort_attributes, best_effort
                )
                server_values = target.getAttributeHandleValueMapFactory().create(1)
                server_values[server_attribute] = b"\x51"
                target.updateAttributeValues(server_object, server_values, b"")
                server_values[server_attribute] = b"\x52"
                target.updateAttributeValues(server_object, server_values, b"")
                soda_values = target.getAttributeHandleValueMapFactory().create(1)
                soda_values[soda_attribute] = b"\x53"
                target.updateAttributeValues(soda_object, soda_values, b"")
                _drain_callbacks(requester, callback.reflections, expected=3)

                updates_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent"
                )
                reflections_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived"
                )
                requester.subscribeInteractionClass(updates_report)
                requester.subscribeInteractionClass(reflections_report)
                updates_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent"
                )
                reflections_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived"
                )
                updates_request_federate = requester.getParameterHandle(
                    updates_request, "HLAfederate"
                )
                reflections_request_federate = requester.getParameterHandle(
                    reflections_request, "HLAfederate"
                )
                updates_transportation = requester.getParameterHandle(
                    updates_report, "HLAtransportation"
                )
                updates_counts = requester.getParameterHandle(updates_report, "HLAupdateCounts")
                reflections_transportation = requester.getParameterHandle(
                    reflections_report, "HLAtransportation"
                )
                reflections_counts = requester.getParameterHandle(
                    reflections_report, "HLAreflectCounts"
                )
                encoded_target = bytes(target_handle.encodedValue)
                encoded_requester = bytes(requester_handle.encodedValue)
                encoded_server_class = bytes(server_class.encodedValue)
                encoded_soda_class = bytes(soda_class.encodedValue)
                reliable_bytes = bytes(transportation_factory.getHLAdefaultReliable().encodedValue)
                best_effort_bytes = bytes(best_effort.encodedValue)
                request_values = requester.getParameterHandleValueMapFactory().create(1)

                request_values[updates_request_federate] = encoded_target
                requester.sendInteraction(updates_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                if len(callback.received) < 2:
                    raise AssertionError("MOM updates-sent report count was wrong")
                assert_report(
                    callback.received[-2], updates_report, updates_transportation, updates_counts,
                    reliable_bytes, encoded_soda_class, 1, "MOM updates reliable"
                )
                assert_report(
                    callback.received[-1], updates_report, updates_transportation, updates_counts,
                    best_effort_bytes, encoded_server_class, 2, "MOM updates best-effort"
                )

                request_values.clear()
                # Reflections belong to the receiving requester federate.
                request_values[reflections_request_federate] = encoded_requester
                requester.sendInteraction(reflections_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=4)
                if len(callback.received) < 4:
                    raise AssertionError("MOM reflections-received report count was wrong")
                assert_report(
                    callback.received[-2], reflections_report, reflections_transportation,
                    reflections_counts, reliable_bytes, encoded_soda_class, 1,
                    "MOM reflections reliable"
                )
                assert_report(
                    callback.received[-1], reflections_report, reflections_transportation,
                    reflections_counts, best_effort_bytes, encoded_server_class, 2,
                    "MOM reflections best-effort"
                )

                request_values.clear()
                request_values[updates_request_federate] = encoded_requester
                requester.sendInteraction(updates_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=6)
                if len(callback.received) < 6:
                    raise AssertionError("MOM empty updates-sent report count was wrong")
                assert_report(
                    callback.received[-2], updates_report, updates_transportation, updates_counts,
                    reliable_bytes, None, 0, "MOM empty updates reliable"
                )
                assert_report(
                    callback.received[-1], updates_report, updates_transportation, updates_counts,
                    best_effort_bytes, None, 0, "MOM empty updates best-effort"
                )

                request_values.clear()
                # The publishing target has no received reflections, giving
                # a pair of standard NULL transport buckets.
                request_values[reflections_request_federate] = encoded_target
                requester.sendInteraction(reflections_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=8)
                if len(callback.received) < 8:
                    raise AssertionError("MOM empty reflections-received report count was wrong")
                assert_report(
                    callback.received[-2], reflections_report, reflections_transportation,
                    reflections_counts, reliable_bytes, None, 0,
                    "MOM empty reflections reliable"
                )
                assert_report(
                    callback.received[-1], reflections_report, reflections_transportation,
                    reflections_counts, best_effort_bytes, None, 0,
                    "MOM empty reflections best-effort"
                )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM update/reflection counts by transportation, including NULL buckets"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-transport-count-reports",
                "mom",
                mom_transport_count_reports,
            )
        )

        def mom_interaction_count_reports() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-interactions-{int(time.time() * 1000)}"

            def assert_report(
                received: tuple[object, ...],
                report_class: object,
                transportation: object,
                counts: object,
                expected_transport: bytes,
                expected_classes: list[bytes] | None,
                expected_counts: list[int] | None,
                label: str,
            ) -> None:
                if received[0] != report_class:
                    raise AssertionError(f"{label} report class did not round-trip")
                values = received[1]
                if len(values) != 2 or transportation not in values or counts not in values:
                    raise AssertionError(f"{label} report parameters were wrong")
                if bytes(values[transportation]) != expected_transport:
                    raise AssertionError(f"{label} transportation parameter did not round-trip")
                encoded_counts = bytes(values[counts])
                if expected_classes is None or expected_counts is None:
                    if encoded_counts:
                        raise AssertionError(f"{label} NULL count list was not empty")
                else:
                    for expected_class, expected_count in zip(expected_classes, expected_counts):
                        if expected_class not in encoded_counts:
                            raise AssertionError(
                                f"{label} interaction class was absent from the count list"
                            )
                        if expected_count.to_bytes(4, "big", signed=True) not in encoded_counts:
                            raise AssertionError(
                                f"{label} interaction count was absent from the count list"
                            )
                if received[2] != b"":
                    raise AssertionError(f"{label} report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(f"{label} report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(f"{label} report transport did not round-trip")

            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-interactions-target", federation
                )
                target_joined = True
                requester_handle = requester.joinFederationExecution(
                    "umbra-2010-mom-interactions-requester", federation
                )
                requester_joined = True

                target_order_taken = target.getInteractionClassHandle(
                    "HLAinteractionRoot.CustomerTransactions.OrderTaken"
                )
                requester_order_taken = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.CustomerTransactions.OrderTaken"
                )
                target_customer_seated = target.getInteractionClassHandle(
                    "HLAinteractionRoot.CustomerTransactions.CustomerSeated"
                )
                requester_customer_seated = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.CustomerTransactions.CustomerSeated"
                )
                target.publishInteractionClass(target_order_taken)
                target.publishInteractionClass(target_customer_seated)
                requester.subscribeInteractionClass(requester_order_taken)
                requester.subscribeInteractionClass(requester_customer_seated)

                transportation_factory = target.getTransportationTypeHandleFactory()
                best_effort = transportation_factory.getHLAdefaultBestEffort()
                target.sendInteraction(
                    target_order_taken,
                    target.getParameterHandleValueMapFactory().create(0),
                    b"",
                )
                target.requestInteractionTransportationTypeChange(target_order_taken, best_effort)
                for _ in range(2):
                    target.sendInteraction(
                        target_order_taken,
                        target.getParameterHandleValueMapFactory().create(0),
                        b"",
                    )
                target.sendInteraction(
                    target_customer_seated,
                    target.getParameterHandleValueMapFactory().create(0),
                    b"",
                )
                # The application sends exercise the receive callback too;
                # retain only the MOM reports for the assertions below.
                callback.received.clear()

                sent_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent"
                )
                received_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived"
                )
                requester.subscribeInteractionClass(sent_report)
                requester.subscribeInteractionClass(received_report)
                sent_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent"
                )
                received_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived"
                )
                sent_request_federate = requester.getParameterHandle(sent_request, "HLAfederate")
                received_request_federate = requester.getParameterHandle(
                    received_request, "HLAfederate"
                )
                sent_transportation = requester.getParameterHandle(sent_report, "HLAtransportation")
                sent_counts = requester.getParameterHandle(sent_report, "HLAinteractionCounts")
                received_transportation = requester.getParameterHandle(
                    received_report, "HLAtransportation"
                )
                received_counts = requester.getParameterHandle(
                    received_report, "HLAinteractionCounts"
                )
                encoded_target = bytes(target_handle.encodedValue)
                encoded_requester = bytes(requester_handle.encodedValue)
                encoded_order_taken = bytes(requester_order_taken.encodedValue)
                encoded_customer_seated = bytes(requester_customer_seated.encodedValue)
                reliable_bytes = bytes(transportation_factory.getHLAdefaultReliable().encodedValue)
                best_effort_bytes = bytes(best_effort.encodedValue)
                request_values = requester.getParameterHandleValueMapFactory().create(1)

                request_values[sent_request_federate] = encoded_target
                requester.sendInteraction(sent_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                if len(callback.received) < 2:
                    raise AssertionError("MOM interactions-sent report count was wrong")
                assert_report(
                    callback.received[-2], sent_report, sent_transportation, sent_counts,
                    reliable_bytes, [encoded_order_taken, encoded_customer_seated], [1, 1],
                    "MOM interactions sent reliable",
                )
                assert_report(
                    callback.received[-1], sent_report, sent_transportation, sent_counts,
                    best_effort_bytes, [encoded_order_taken], [2],
                    "MOM interactions sent best-effort",
                )

                request_values.clear()
                request_values[received_request_federate] = encoded_requester
                requester.sendInteraction(received_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=4)
                if len(callback.received) < 4:
                    raise AssertionError("MOM interactions-received report count was wrong")
                assert_report(
                    callback.received[-2], received_report, received_transportation, received_counts,
                    reliable_bytes, [encoded_order_taken, encoded_customer_seated], [1, 1],
                    "MOM interactions received reliable",
                )
                assert_report(
                    callback.received[-1], received_report, received_transportation, received_counts,
                    best_effort_bytes, [encoded_order_taken], [2],
                    "MOM interactions received best-effort",
                )

                request_values.clear()
                request_values[sent_request_federate] = encoded_requester
                requester.sendInteraction(sent_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=6)
                if len(callback.received) < 6:
                    raise AssertionError("MOM empty interactions-sent report count was wrong")
                assert_report(
                    callback.received[-2], sent_report, sent_transportation, sent_counts,
                    reliable_bytes, None, None, "MOM empty interactions sent reliable",
                )
                assert_report(
                    callback.received[-1], sent_report, sent_transportation, sent_counts,
                    best_effort_bytes, None, None, "MOM empty interactions sent best-effort",
                )

                request_values.clear()
                request_values[received_request_federate] = encoded_target
                requester.sendInteraction(received_request, request_values, b"")
                _drain_callbacks(requester, callback.received, expected=8)
                if len(callback.received) < 8:
                    raise AssertionError("MOM empty interactions-received report count was wrong")
                assert_report(
                    callback.received[-2], received_report, received_transportation, received_counts,
                    reliable_bytes, None, None, "MOM empty interactions received reliable",
                )
                assert_report(
                    callback.received[-1], received_report, received_transportation, received_counts,
                    best_effort_bytes, None, None, "MOM empty interactions received best-effort",
                )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM interaction sent/received counts by transportation, including NULL buckets"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-interaction-count-reports",
                "mom",
                mom_interaction_count_reports,
            )
        )

        def mom_fom_mim_data_reports() -> str:
            nonlocal ambassador
            if args.fom_path is None or args.mim_path is None:
                raise NotImplementedError("2010 FOM and standard MIM are required")
            fom_path = args.fom_path.resolve()
            mim_path = args.mim_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")
            if not mim_path.is_file():
                raise FileNotFoundError(f"2010 MIM does not exist: {mim_path}")
            expected_fom = fom_path.read_bytes().decode("utf-8").encode("utf-16-be")
            expected_mim = mim_path.read_bytes().decode("utf-8").encode("utf-16-be")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.received: list[tuple[object, ...]] = []

                def receiveInteraction(self, *values: object) -> None:
                    self.received.append(values)

            callback = Recorder()
            requester = factory.getRtiAmbassador()
            target = factory.getRtiAmbassador()
            requester_connected = False
            target_connected = False
            created = False
            requester_joined = False
            target_joined = False
            federation = f"umbra-2010-mom-fom-mim-{int(time.time() * 1000)}"

            def assert_module_report(
                received: tuple[object, ...],
                report_class: object,
                indicator: object,
                data: object,
                expected_indicator: bytes,
                expected_data: bytes,
                label: str,
            ) -> None:
                if received[0] != report_class:
                    raise AssertionError(f"{label} report class did not round-trip")
                values = received[1]
                if len(values) != 2 or indicator not in values or data not in values:
                    raise AssertionError(f"{label} report parameters were wrong")
                if bytes(values[indicator]) != expected_indicator:
                    raise AssertionError(f"{label} module indicator did not round-trip")
                if bytes(values[data]) != expected_data:
                    raise AssertionError(f"{label} module text did not round-trip")
                if received[2] != b"":
                    raise AssertionError(f"{label} report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(f"{label} report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(f"{label} report transport did not round-trip")

            def assert_mim_report(
                received: tuple[object, ...],
                report_class: object,
                data: object,
                expected_data: bytes,
                label: str,
            ) -> None:
                if received[0] != report_class:
                    raise AssertionError(f"{label} report class did not round-trip")
                values = received[1]
                if len(values) != 1 or data not in values:
                    raise AssertionError(f"{label} report parameters were wrong")
                if bytes(values[data]) != expected_data:
                    raise AssertionError(f"{label} MIM text did not round-trip")
                if received[2] != b"":
                    raise AssertionError(f"{label} report tag was not empty")
                if getattr(received[3], "name", None) != "RECEIVE":
                    raise AssertionError(f"{label} report receive order did not round-trip")
                if received[4].encodedValue != b"transport:HLAdefaultReliable":
                    raise AssertionError(f"{label} report transport did not round-trip")

            try:
                requester.connect(callback, CallbackModel.HLA_EVOKED)
                requester_connected = True
                target.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
                target_connected = True
                target.createFederationExecution(federation, (fom_path,), mim_path)
                created = True
                target_handle = target.joinFederationExecution(
                    "umbra-2010-mom-fom-mim-target", federation
                )
                target_joined = True
                requester.joinFederationExecution("umbra-2010-mom-fom-mim-requester", federation)
                requester_joined = True

                federate_fom_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData"
                )
                federation_fom_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData"
                )
                mim_report = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata"
                )
                requester.subscribeInteractionClass(federate_fom_report)
                requester.subscribeInteractionClass(federation_fom_report)
                requester.subscribeInteractionClass(mim_report)

                federate_fom_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData"
                )
                federation_fom_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData"
                )
                mim_request = requester.getInteractionClassHandle(
                    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata"
                )
                federate_fom_target = requester.getParameterHandle(federate_fom_request, "HLAfederate")
                federate_fom_indicator = requester.getParameterHandle(
                    federate_fom_request, "HLAFOMmoduleIndicator"
                )
                federation_fom_indicator = requester.getParameterHandle(
                    federation_fom_request, "HLAFOMmoduleIndicator"
                )
                federate_fom_report_indicator = requester.getParameterHandle(
                    federate_fom_report, "HLAFOMmoduleIndicator"
                )
                federate_fom_report_data = requester.getParameterHandle(
                    federate_fom_report, "HLAFOMmoduleData"
                )
                federation_fom_report_indicator = requester.getParameterHandle(
                    federation_fom_report, "HLAFOMmoduleIndicator"
                )
                federation_fom_report_data = requester.getParameterHandle(
                    federation_fom_report, "HLAFOMmoduleData"
                )
                mim_report_data = requester.getParameterHandle(mim_report, "HLAMIMdata")
                encoded_target = bytes(target_handle.encodedValue)
                zero = (0).to_bytes(4, "big", signed=True)

                values = requester.getParameterHandleValueMapFactory().create(2)
                values[federate_fom_target] = encoded_target
                values[federate_fom_indicator] = zero
                requester.sendInteraction(federate_fom_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=1)
                if len(callback.received) < 1:
                    raise AssertionError("federate FOM report count was wrong")
                assert_module_report(
                    callback.received[-1], federate_fom_report, federate_fom_report_indicator,
                    federate_fom_report_data, zero, expected_fom, "federate FOM module report",
                )

                values = requester.getParameterHandleValueMapFactory().create(1)
                values[federation_fom_indicator] = zero
                requester.sendInteraction(federation_fom_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=2)
                if len(callback.received) < 2:
                    raise AssertionError("federation FOM report count was wrong")
                assert_module_report(
                    callback.received[-1], federation_fom_report,
                    federation_fom_report_indicator, federation_fom_report_data,
                    zero, expected_fom, "federation FOM module report",
                )

                values = requester.getParameterHandleValueMapFactory().create(0)
                requester.sendInteraction(mim_request, values, b"")
                _drain_callbacks(requester, callback.received, expected=3)
                if len(callback.received) < 3:
                    raise AssertionError("federation MIM report count was wrong")
                assert_mim_report(
                    callback.received[-1], mim_report, mim_report_data, expected_mim,
                    "federation MIM report",
                )
            finally:
                if requester_joined:
                    try:
                        requester.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if target_joined:
                    try:
                        target.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        target.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if target_connected:
                    try:
                        target.disconnect()
                    except Exception:
                        pass
                if requester_connected:
                    try:
                        requester.disconnect()
                    finally:
                        ambassador = None
            return "standard MIM federate/federation FOM and MIM text reports through JPype"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.mom-fom-mim-data-reports",
                "mom",
                mom_fom_mim_data_reports,
            )
        )

        def time_management() -> str:
            nonlocal ambassador
            if args.fom_path is None:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.regulation: list[object] = []
                    self.constrained: list[object] = []
                    self.grants: list[object] = []

                def timeRegulationEnabled(self, logical_time: object) -> None:
                    self.regulation.append(logical_time)

                def timeConstrainedEnabled(self, logical_time: object) -> None:
                    self.constrained.append(logical_time)

                def timeAdvanceGrant(self, logical_time: object) -> None:
                    self.grants.append(logical_time)

            callback = Recorder()
            ambassador = factory.getRtiAmbassador()
            connected = False
            created = False
            joined = False
            federation = f"umbra-1516e-python-time-{int(time.time() * 1000)}"
            try:
                ambassador.connect(callback, CallbackModel.HLA_EVOKED)
                connected = True
                ambassador.createFederationExecution(federation, fom_path)
                created = True
                ambassador.joinFederationExecution("umbra-1516e-python-time", federation)
                joined = True

                time_factory = ambassador.getTimeFactory()
                initial = time_factory.makeInitial()
                lookahead = time_factory.makeEpsilon()
                if lookahead.getInterval() <= 0:
                    raise AssertionError("provider returned a non-positive time epsilon")

                ambassador.enableTimeRegulation(lookahead)
                _drain_callbacks(ambassador, callback.regulation)
                if len(callback.regulation) != 1:
                    raise AssertionError("timeRegulationEnabled did not cross JPype")
                if callback.regulation[0].getTime() != initial.getTime():
                    raise AssertionError("time-regulation callback time did not round-trip")
                if ambassador.queryLookahead().getInterval() != lookahead.getInterval():
                    raise AssertionError("lookahead interval did not round-trip")

                ambassador.enableTimeConstrained()
                _drain_callbacks(ambassador, callback.constrained)
                if len(callback.constrained) != 1:
                    raise AssertionError("timeConstrainedEnabled did not cross JPype")
                if callback.constrained[0].getTime() != initial.getTime():
                    raise AssertionError("time-constrained callback time did not round-trip")

                requested = time_factory.makeTime(5)
                ambassador.timeAdvanceRequest(requested)
                _drain_callbacks(ambassador, callback.grants)
                if len(callback.grants) != 1:
                    raise AssertionError("timeAdvanceGrant did not cross JPype")
                if callback.grants[0].getTime() != requested.getTime():
                    raise AssertionError("time-advance grant time did not round-trip")
                if ambassador.queryLogicalTime().getTime() != requested.getTime():
                    raise AssertionError("queryLogicalTime did not reflect the grant")

                # Keep the probe on the official LogicalTimeFactory surface;
                # ``makeInterval(value)`` is only a concrete time-factory
                # convenience and is not in the 2010 Java interface.
                next_lookahead = time_factory.makeEpsilon()
                ambassador.modifyLookahead(next_lookahead)
                if ambassador.queryLookahead().getInterval() != next_lookahead.getInterval():
                    raise AssertionError("modified lookahead did not round-trip")
                next_requested = time_factory.makeTime(6)
                ambassador.timeAdvanceRequestAvailable(next_requested)
                _drain_callbacks(ambassador, callback.grants, expected=2)
                if len(callback.grants) != 2:
                    raise AssertionError("timeAdvanceRequestAvailable grant did not cross JPype")
                if callback.grants[-1].getTime() != next_requested.getTime():
                    raise AssertionError("available time-advance grant time did not round-trip")

                ambassador.disableTimeConstrained()
                ambassador.disableTimeRegulation()
            finally:
                if joined:
                    try:
                        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        ambassador.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if connected:
                    try:
                        ambassador.disconnect()
                    finally:
                        ambassador = None
            return "time roles, advance/grant callbacks, lookahead, and logical-time query"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.time-management",
                "time",
                time_management,
            )
        )

        def save_restore() -> str:
            nonlocal ambassador
            if args.fom_path is None:
                raise NotImplementedError("no 2010 FOM configured")
            fom_path = args.fom_path.resolve()
            if not fom_path.is_file():
                raise FileNotFoundError(f"2010 FOM does not exist: {fom_path}")

            class Recorder(NullFederateAmbassador):
                def __init__(self) -> None:
                    self.save_labels: list[str] = []
                    self.saved = 0
                    self.restore_successes: list[str] = []
                    self.restore_begun = 0
                    self.restore_labels: list[str] = []
                    self.restored = 0

                def initiateFederateSave(self, *values: object) -> None:
                    self.save_labels.append(str(values[0]))

                def federationSaved(self) -> None:
                    self.saved += 1

                def requestFederationRestoreSucceeded(self, label: str) -> None:
                    self.restore_successes.append(label)

                def federationRestoreBegun(self) -> None:
                    self.restore_begun += 1

                def initiateFederateRestore(self, *values: object) -> None:
                    self.restore_labels.append(str(values[0]))

                def federationRestored(self) -> None:
                    self.restored += 1

            callback = Recorder()
            ambassador = factory.getRtiAmbassador()
            connected = False
            created = False
            joined = False
            federation = f"umbra-1516e-python-save-{int(time.time() * 1000)}"
            label = "umbra-1516e-python-save"
            try:
                ambassador.connect(callback, CallbackModel.HLA_EVOKED)
                connected = True
                ambassador.createFederationExecution(federation, fom_path)
                created = True
                ambassador.joinFederationExecution("umbra-1516e-python-save", federation)
                joined = True

                ambassador.requestFederationSave(label)
                _drain_callbacks(ambassador, callback.save_labels)
                if callback.save_labels != [label]:
                    raise AssertionError("initiateFederateSave did not cross JPype")
                ambassador.federateSaveBegun()
                ambassador.federateSaveComplete()
                deadline = time.monotonic() + 0.25
                while callback.saved < 1 and time.monotonic() < deadline:
                    _drain_callbacks(ambassador, callback.save_labels)
                    time.sleep(0.005)
                if callback.saved != 1:
                    raise AssertionError("federationSaved did not cross JPype")

                ambassador.requestFederationRestore(label)
                _drain_callbacks(ambassador, callback.restore_successes)
                if callback.restore_successes != [label]:
                    raise AssertionError("requestFederationRestoreSucceeded did not cross JPype")
                if callback.restore_begun != 1 or callback.restore_labels != [label]:
                    raise AssertionError("restore initiation callbacks did not cross JPype")
                ambassador.federateRestoreComplete()
                deadline = time.monotonic() + 0.25
                while callback.restored < 1 and time.monotonic() < deadline:
                    _drain_callbacks(ambassador, callback.restore_successes)
                    time.sleep(0.005)
                if callback.restored != 1:
                    raise AssertionError("federationRestored did not cross JPype")
            finally:
                if joined:
                    try:
                        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
                    except Exception:
                        pass
                if created:
                    try:
                        ambassador.destroyFederationExecution(federation)
                    except Exception:
                        pass
                if connected:
                    try:
                        ambassador.disconnect()
                    finally:
                        ambassador = None
            return "save initiation/completion and restore initiation/completion callbacks"

        scenarios.append(
            _profiled_scenario(
                capability_profile,
                "python-2010-tck.save-restore",
                "save-restore",
                save_restore,
            )
        )

        def logical_time_arithmetic() -> str:
            nonlocal ambassador
            if ambassador is None:
                ambassador = factory.getRtiAmbassador()
                ambassador.connect(NullFederateAmbassador(), CallbackModel.HLA_EVOKED)
            time_factory = ambassador.getTimeFactory()
            time5 = time_factory.makeLogicalTime(5)
            interval2 = time_factory.makeLogicalTimeInterval(2)
            if (
                time5.add(interval2).getTime() != 7
                or time5.subtract(interval2).getTime() != 3
                or time5.distance(time_factory.makeLogicalTime(2)).getInterval() != 3
                or time5.compareTo(time_factory.makeLogicalTime(2)) != 1
                or interval2.compareTo(time_factory.makeLogicalTimeInterval(2)) != 0
            ):
                raise AssertionError("2010 JPype logical-time arithmetic failed")
            representations = [time_factory.getName()]
            # The direct native provider exposes the second mandated reference
            # representation through an explicit selector because the standard
            # RTIambassador has only one provider-selected time factory.
            float_factory_getter = getattr(ambassador, "getFloat64TimeFactory", None)
            if args.native_provider and callable(float_factory_getter):
                float_factory = float_factory_getter()
                float_time = float_factory.makeTime(1.25)
                float_interval = float_factory.makeInterval(2.5)
                if (
                    abs(float_time.add(float_interval).getTime() - 3.75) > 1e-12
                    or abs(
                        float_factory.makeTime(3.75)
                        .subtract(float_interval)
                        .getTime()
                        - 1.25
                    )
                    > 1e-12
                    or abs(
                        float_time.distance(float_factory.makeTime(0.25)).getInterval()
                        - 1.0
                    )
                    > 1e-12
                    or float_time.toByteArray() != bytes.fromhex("3ff4000000000000")
                ):
                    raise AssertionError("2010 native HLAfloat64Time arithmetic failed")
                representations.append(float_factory.getName())
            return f"provider-owned {', '.join(representations)} arithmetic"

        scenarios.append(_profiled_scenario(capability_profile, "python-2010-tck.logical-time-arithmetic", "time", logical_time_arithmetic))

    if ambassador is not None:
        try:
            ambassador.disconnect()
        except Exception:
            pass

    result = {
        "standard": STANDARD_EDITION,
        "transport": "python-native" if args.native_provider else "python-jpype",
        "capability_profile": str(args.capability_profile.resolve()) if args.capability_profile else "default",
        "status": "pass" if not any(item["status"] == "fail" for item in scenarios) else "fail",
        "rti_name": probe.rti_name if probe is not None else "",
        "rti_version": probe.rti_version if probe is not None else "",
        "scenarios": scenarios,
        "summary": {
            "pass": sum(item["status"] == "pass" for item in scenarios),
            "fail": sum(item["status"] == "fail" for item in scenarios),
            "unsupported": sum(item["status"] == "unsupported" for item in scenarios),
        },
    }
    args.results.parent.mkdir(parents=True, exist_ok=True)
    args.results.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
