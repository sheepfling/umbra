"""Verify the focused IEEE 1516e-2010 JNI carrier round-trip matrix."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = (
    ROOT / "compliance" / "catalogs" / "ieee1516e-2010-jni-type-roundtrip-catalog.json"
)

EXPECTED_DATA_ELEMENTS = {
    "HLAASCIIchar",
    "HLAASCIIstring",
    "HLAboolean",
    "HLAbyte",
    "HLAfloat32BE",
    "HLAfloat32LE",
    "HLAfloat64BE",
    "HLAfloat64LE",
    "HLAinteger16BE",
    "HLAinteger16LE",
    "HLAinteger32BE",
    "HLAinteger32LE",
    "HLAinteger64BE",
    "HLAinteger64LE",
    "HLAoctet",
    "HLAoctetPairBE",
    "HLAoctetPairLE",
    "HLAunicodeChar",
    "HLAunicodeString",
    "HLAopaqueData",
    "HLAvariableArray",
    "HLAfixedArray",
    "HLAfixedRecord",
    "HLAvariantRecord",
}
EXPECTED_BASIC_DATA_ELEMENTS = EXPECTED_DATA_ELEMENTS - {
    "HLAvariableArray",
    "HLAfixedArray",
    "HLAfixedRecord",
    "HLAvariantRecord",
}
EXPECTED_BASIC_VALUE_MATRIX_COUNT = 79
EXPECTED_HANDLES = {
    "FederateHandle",
    "ObjectClassHandle",
    "InteractionClassHandle",
    "ObjectInstanceHandle",
    "AttributeHandle",
    "ParameterHandle",
    "DimensionHandle",
    "MessageRetractionHandle",
    "RegionHandle",
}
EXPECTED_TIMES = {
    "HLAinteger64Time",
    "HLAinteger64Interval",
    "HLAfloat64Time",
    "HLAfloat64Interval",
}
EXPECTED_JAVA_ONLY_HANDLES = {"TransportationTypeHandle"}
EXPECTED_LOGICAL_TIME_FACTORIES = {"HLAinteger64TimeFactory", "HLAfloat64TimeFactory"}
EXPECTED_HANDLE_FACTORIES = {
    "FederateHandleFactory",
    "ObjectClassHandleFactory",
    "InteractionClassHandleFactory",
    "ObjectInstanceHandleFactory",
    "AttributeHandleFactory",
    "ParameterHandleFactory",
    "DimensionHandleFactory",
    "TransportationTypeHandleFactory",
}
EXPECTED_COLLECTION_KINDS = {
    "AttributeHandleSet",
    "DimensionHandleSet",
    "FederateHandleSet",
    "RegionHandleSet",
    "FederationExecutionInformationSet",
    "AttributeHandleValueMap",
    "ParameterHandleValueMap",
    "AttributeSetRegionSetPairList",
}
EXPECTED_COLLECTION_FACTORIES = {
    "AttributeHandleSetFactory",
    "DimensionHandleSetFactory",
    "FederateHandleSetFactory",
    "RegionHandleSetFactory",
    "AttributeHandleValueMapFactory",
    "ParameterHandleValueMapFactory",
    "AttributeSetRegionSetPairListFactory",
}
EXPECTED_DATA_ELEMENT_FACTORY_KINDS = {
    "HLAoctet",
    "HLAinteger32BE",
    "HLAASCIIstring",
    "HLAopaqueData",
    "HLAvariableArray",
    "HLAfixedArray",
    "HLAfixedRecord",
    "HLAvariantRecord",
}
EXPECTED_MALFORMED_LENGTH_PREFIX_KINDS = {
    "HLAASCIIstring",
    "HLAunicodeString",
    "HLAopaqueData",
    "HLAvariableArray",
}
EXPECTED_DATA_ELEMENT_DECODE_OVERLOADS = {"byte[]", "ByteWrapper"}
EXPECTED_PYTHON_ENCODER_FACADE = "umbra._java.rti1516e.encoding.JavaEncoderFactory"
EXPECTED_ENCODER_CREATOR_COUNT = 24
EXPECTED_ENCODER_CREATOR_JAVA_TEST = "requireEncoderFactorySurface"
EXPECTED_ENCODER_COMPOSITE_JAVA_TEST = "requireEncoderFactoryCompositeArguments"
EXPECTED_ENCODER_CREATOR_PYTHON_TEST = (
    "test_python_encoder_facade_matches_reflected_2010_creator_surface"
)
EXPECTED_ENCODER_COMPOSITE_PYTHON_TEST = (
    "test_python_encoder_facade_forwards_composite_creator_arguments"
)
EXPECTED_LOGICAL_TIME_WIRE_VECTOR_COUNT = 16
EXPECTED_LOGICAL_TIME_WIRE_SUPPORT = (
    "packages/umbra-rti-test-support/src/umbra_rti_test_support/wire_matrix.py"
)
EXPECTED_LOGICAL_TIME_WIRE_TEST = (
    "test_python_logical_time_wire_matrix_round_trips_through_jni"
)
EXPECTED_VENDOR_TIME_ARITHMETIC_VECTOR_COUNT = 7
EXPECTED_VENDOR_TIME_ARITHMETIC_SUPPORT = (
    "packages/umbra-rti-test-support/src/umbra_rti_test_support/provider_extension_matrix.py"
)
EXPECTED_VENDOR_TIME_ARITHMETIC_TEST = (
    "test_python_logical_time_operation_surface_is_callable_through_jni"
)
EXPECTED_VENDOR_DATA_ELEMENT_VECTOR_COUNT = 4
EXPECTED_VENDOR_DATA_ELEMENT_SUPPORT = (
    "packages/umbra-rti-test-support/src/umbra_rti_test_support/provider_extension_matrix.py"
)
EXPECTED_VENDOR_DATA_ELEMENT_TEST = (
    "test_python_unknown_vendor_data_element_preserves_jni_carrier"
)
EXPECTED_PYTHON_CARRIER_FACADE_TEST = (
    "test_python_handle_and_collection_facades_round_trip_through_jni"
)
EXPECTED_PYTHON_CARRIER_FACADE_HANDLES = {
    "FederateHandle",
    "ObjectClassHandle",
    "ObjectInstanceHandle",
    "AttributeHandle",
    "InteractionClassHandle",
    "ParameterHandle",
    "TransportationTypeHandle",
    "DimensionHandle",
    "MessageRetractionHandle",
    "RegionHandle",
}
EXPECTED_PYTHON_CARRIER_FACADE_COLLECTIONS = {
    "AttributeHandleSet",
    "DimensionHandleSet",
    "FederateHandleSet",
    "RegionHandleSet",
    "FederationExecutionInformationSet",
    "AttributeHandleValueMap",
    "ParameterHandleValueMap",
    "AttributeSetRegionSetPairList",
}
EXPECTED_PYTHON_CARRIER_FACADE_RECORDS = {
    "RangeBounds",
    "FederationExecutionInformation",
    "FederateHandleSaveStatusPair",
    "FederateRestoreStatus",
    "MessageRetractionReturn",
    "TimeQueryReturn",
}
EXPECTED_PYTHON_CARRIER_FACADE_CALLBACKS = {
    "SupplementalReflectInfo",
    "SupplementalReceiveInfo",
    "SupplementalRemoveInfo",
}
EXPECTED_PYTHON_CARRIER_RECORD_TEST = (
    "test_python_callback_and_record_carriers_round_trip_through_jni"
)
EXPECTED_PYTHON_CALLBACK_OVERLOAD_TEST = (
    "test_python_callback_overload_carriers_round_trip_through_jni"
)
EXPECTED_PYTHON_RETURN_HANDLE_TEST = (
    "test_python_returned_retraction_handle_can_round_trip_back_to_jni"
)
EXPECTED_PYTHON_RETURN_SERVICES = {
    "updateAttributeValues",
    "sendInteraction",
    "deleteObjectInstance",
    "sendInteractionWithRegions",
}
EXPECTED_PYTHON_RETURN_SURFACE_TEST = (
    "test_python_rti_return_surface_matrix_round_trips_jni_carriers"
)
EXPECTED_PYTHON_CALLBACK_SURFACE_TEST = (
    "test_python_callback_surface_matrix_round_trips_through_jni"
)
EXPECTED_CALLBACK_METHOD_COUNT = 51
EXPECTED_CALLBACK_OVERLOAD_COUNT = 60
EXPECTED_RETURN_OVERLOAD_COUNT = 50
EXPECTED_PYTHON_FACTORY_SURFACE_TEST = (
    "test_python_discovers_jni_factory_and_binds_complete_2010_surface"
)
EXPECTED_RTIAMBASSADOR_METHOD_COUNT = 150
EXPECTED_RTIAMBASSADOR_OVERLOAD_COUNT = 172
EXPECTED_PYTHON_SERVICE_ARGUMENT_TEST = (
    "test_python_jni_service_arguments_reach_standard_null_surface"
)
EXPECTED_PYTHON_SERVICE_ARGUMENT_OVERLOAD_COUNT = 169
EXPECTED_REQUIREMENT_REFS = {
    "logical_time_wire_matrix": {
        "compliance/requirements-lab/logical-time-encoding-requirements-contract.json",
        "compliance/requirements-lab/float-time-requirements-contract.json",
        "compliance/requirements-lab/reference-time-requirements-contract.json",
    },
    "python_carrier_facade_matrix": {
        "compliance/requirements-lab/handle-encoding-requirements-contract.json",
        "compliance/requirements-lab/handle-decoding-api-contract.json",
        "compliance/requirements-lab/connection-callback-api-contract.json",
        "compliance/requirements-lab/save-control-requirements-contract.json",
        "compliance/requirements-lab/restore-control-requirements-contract.json",
        "compliance/requirements-lab/logical-time-encoding-requirements-contract.json",
    },
    "basic_value_matrix": {
        "compliance/requirements-lab/basic-data-elements-requirements-contract.json",
    },
    "malformed_wire_matrix": {
        "compliance/requirements-lab/basic-data-elements-requirements-contract.json",
        "compliance/requirements-lab/logical-time-encoding-requirements-contract.json",
    },
    "byte_wrapper_cursor_matrix": {
        "compliance/requirements-lab/basic-data-elements-requirements-contract.json",
    },
    "python_encoder_facade_matrix": {
        "compliance/requirements-lab/basic-data-elements-requirements-contract.json",
    },
    "callback_surface_matrix": {
        "compliance/requirements-lab/connection-callback-api-contract.json",
    },
    "python_return_surface_matrix": {
        "compliance/requirements-lab/handle-encoding-requirements-contract.json",
        "compliance/requirements-lab/handle-decoding-api-contract.json",
        "compliance/requirements-lab/logical-time-encoding-requirements-contract.json",
        "compliance/requirements-lab/connection-callback-api-contract.json",
    },
    "python_factory_surface_matrix": {
        "compliance/requirements-lab/connection-callback-api-contract.json",
    },
    "python_service_argument_matrix": {
        "compliance/requirements-lab/connection-callback-api-contract.json",
        "compliance/requirements-lab/handle-encoding-requirements-contract.json",
        "compliance/requirements-lab/logical-time-encoding-requirements-contract.json",
    },
}
EXPECTED_AUXILIARY = {
    "factory": {
        "encoderFactoryCarrier",
        "dataElementFactoryCarrier",
        "logicalTimeFactoryCarrier",
        "handleFactoryCarrier",
        "collectionFactoryCarrier",
    },
    "collection": {
        "roundTripHandleCollection",
        "roundTripByteMatrix",
        "collectionCarrier",
    },
    "enum": {"roundTripEnum"},
    "record": {"roundTripObject"},
    "callback": {"roundTripObject"},
    "exception": {"roundTripObject", "roundTripDataElement"},
}


def verify(path: Path) -> list[str]:
    with path.open(encoding="utf-8") as stream:
        catalog: dict[str, Any] = json.load(stream)
    findings: list[str] = []
    if catalog.get("standard") != "IEEE 1516.1-2010":
        findings.append("catalog standard must be IEEE 1516.1-2010")
    if catalog.get("kind") != "ieee1516e-2010-jni-type-roundtrip-catalog":
        findings.append("catalog kind is not the JNI type-roundtrip catalog")
    carriers = catalog.get("carriers")
    if not isinstance(carriers, list) or not carriers:
        return findings + ["carriers must be a non-empty array"]
    probes = {item.get("probe") for item in carriers if isinstance(item, dict)}
    inventories = catalog.get("carrier_inventories")
    if not isinstance(inventories, dict):
        findings.append("carrier_inventories must be an object")
    else:
        inventory_expectations = {
            "data_elements": EXPECTED_DATA_ELEMENTS,
            "cpp_handles": EXPECTED_HANDLES,
            "java_only_handles": EXPECTED_JAVA_ONLY_HANDLES,
            "logical_times": EXPECTED_TIMES,
            "logical_time_factories": EXPECTED_LOGICAL_TIME_FACTORIES,
            "handle_factories": EXPECTED_HANDLE_FACTORIES,
            "data_element_factories": EXPECTED_DATA_ELEMENT_FACTORY_KINDS,
            "collection_kinds": EXPECTED_COLLECTION_KINDS,
            "collection_factories": EXPECTED_COLLECTION_FACTORIES,
        }
        for name, expected in inventory_expectations.items():
            actual = set(inventories.get(name, []))
            if expected - actual:
                findings.append(
                    f"carrier inventory {name} is missing: "
                    + ", ".join(sorted(expected - actual))
                )
    if "roundTripBytes" not in probes or "roundTripByteWrapper" not in probes:
        findings.append("raw byte and ByteWrapper probes are required")
    for section_name, expected_refs in EXPECTED_REQUIREMENT_REFS.items():
        section = catalog.get(section_name)
        if not isinstance(section, dict):
            continue
        refs = section.get("requirement_refs")
        if not isinstance(refs, list) or any(
            not isinstance(reference, str) or not reference for reference in refs
        ):
            findings.append(
                f"{section_name} requirement_refs must be a non-empty string array"
            )
            continue
        if set(refs) != expected_refs:
            findings.append(
                f"{section_name} requirement_refs do not match the expected Requirements Lab linkage"
            )
        for reference in refs:
            if not (ROOT / reference).is_file():
                findings.append(
                    f"{section_name} requirement reference is missing: {reference}"
                )
    logical_time_matrix = catalog.get("logical_time_wire_matrix")
    if not isinstance(logical_time_matrix, dict):
        findings.append("logical_time_wire_matrix must be an object")
    else:
        if logical_time_matrix.get("support") != EXPECTED_LOGICAL_TIME_WIRE_SUPPORT:
            findings.append("logical-time wire matrix shared support path is incorrect")
        if logical_time_matrix.get("wire_vector_count") != (
            EXPECTED_LOGICAL_TIME_WIRE_VECTOR_COUNT
        ):
            findings.append("logical-time wire matrix vector count must remain 16")
        if logical_time_matrix.get("canonical_output_owned_by_cpp") is not True:
            findings.append(
                "logical-time wire matrix must identify C++-owned canonical output"
            )
        if logical_time_matrix.get("offset_window_preserves_suffix") is not True:
            findings.append(
                "logical-time wire matrix must require offset-window suffix preservation"
            )
        if logical_time_matrix.get("arithmetic_status") != "deferred":
            findings.append(
                "2010 JNI logical-time arithmetic must remain explicitly deferred"
            )
        if not logical_time_matrix.get("arithmetic_reason"):
            findings.append(
                "2010 JNI logical-time arithmetic deferral requires a reason"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = logical_time_matrix.get("python_test_method")
        if python_test != EXPECTED_LOGICAL_TIME_WIRE_TEST:
            findings.append("logical-time wire matrix Python test method is incorrect")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"logical-time wire matrix Python test method is missing: {python_test}"
            )
    vendor_time_matrix = catalog.get("vendor_time_arithmetic_surface")
    if not isinstance(vendor_time_matrix, dict):
        findings.append("vendor_time_arithmetic_surface must be an object")
    else:
        if vendor_time_matrix.get("support") != EXPECTED_VENDOR_TIME_ARITHMETIC_SUPPORT:
            findings.append("vendor time arithmetic matrix shared support path is incorrect")
        if vendor_time_matrix.get("vector_count") != (
            EXPECTED_VENDOR_TIME_ARITHMETIC_VECTOR_COUNT
        ):
            findings.append("vendor time arithmetic matrix vector count must remain 7")
        if vendor_time_matrix.get("status") != "carrier-return-shape-only":
            findings.append(
                "vendor time arithmetic matrix must remain carrier-return-shape-only"
            )
        if not vendor_time_matrix.get("reason"):
            findings.append(
                "vendor time arithmetic matrix requires an explicit semantic boundary"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = vendor_time_matrix.get("python_test_method")
        if python_test != EXPECTED_VENDOR_TIME_ARITHMETIC_TEST:
            findings.append("vendor time arithmetic matrix Python test method is incorrect")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"vendor time arithmetic matrix Python test method is missing: {python_test}"
            )
    vendor_data_matrix = catalog.get("vendor_data_element_surface")
    if not isinstance(vendor_data_matrix, dict):
        findings.append("vendor_data_element_surface must be an object")
    else:
        if vendor_data_matrix.get("support") != EXPECTED_VENDOR_DATA_ELEMENT_SUPPORT:
            findings.append("vendor data-element matrix shared support path is incorrect")
        if vendor_data_matrix.get("vector_count") != (
            EXPECTED_VENDOR_DATA_ELEMENT_VECTOR_COUNT
        ):
            findings.append("vendor data-element matrix vector count must remain 4")
        if vendor_data_matrix.get("status") != "raw-carrier-preservation":
            findings.append(
                "vendor data-element matrix must remain raw-carrier-preservation"
            )
        if not vendor_data_matrix.get("reason"):
            findings.append(
                "vendor data-element matrix requires an explicit semantic boundary"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = vendor_data_matrix.get("python_test_method")
        if python_test != EXPECTED_VENDOR_DATA_ELEMENT_TEST:
            findings.append("vendor data-element matrix Python test method is incorrect")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"vendor data-element matrix Python test method is missing: {python_test}"
            )
    carrier_matrix = catalog.get("python_carrier_facade_matrix")
    if not isinstance(carrier_matrix, dict):
        findings.append("python_carrier_facade_matrix must be an object")
    else:
        if set(carrier_matrix.get("handle_families", [])) != (
            EXPECTED_PYTHON_CARRIER_FACADE_HANDLES
        ):
            findings.append(
                "Python carrier façade matrix must cover every standard handle family"
            )
        if set(carrier_matrix.get("collections", [])) != (
            EXPECTED_PYTHON_CARRIER_FACADE_COLLECTIONS
        ):
            findings.append("Python carrier façade matrix collection set is incomplete")
        if set(carrier_matrix.get("record_carriers", [])) != (
            EXPECTED_PYTHON_CARRIER_FACADE_RECORDS
        ):
            findings.append("Python carrier façade matrix record set is incomplete")
        if set(carrier_matrix.get("callback_carriers", [])) != (
            EXPECTED_PYTHON_CARRIER_FACADE_CALLBACKS
        ):
            findings.append("Python carrier façade matrix callback set is incomplete")
        if carrier_matrix.get("round_trip_direction") != (
            "Python -> standard Java carrier -> JNI -> C++ -> JNI -> standard Java carrier -> Python"
        ):
            findings.append(
                "Python carrier façade matrix must declare the complete round-trip direction"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = carrier_matrix.get("python_test_method")
        if python_test != EXPECTED_PYTHON_CARRIER_FACADE_TEST:
            findings.append(
                "Python carrier façade matrix Python test method is incorrect"
            )
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"Python carrier façade matrix test method is missing: {python_test}"
            )
        record_test = carrier_matrix.get("record_callback_test_method")
        if record_test != EXPECTED_PYTHON_CARRIER_RECORD_TEST:
            findings.append(
                "Python carrier façade matrix record/callback test method is incorrect"
            )
        elif not python_path.is_file() or record_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"Python carrier façade matrix record/callback test method is missing: {record_test}"
            )
        callback_test = carrier_matrix.get("callback_overload_test_method")
        if callback_test != EXPECTED_PYTHON_CALLBACK_OVERLOAD_TEST:
            findings.append(
                "Python carrier façade matrix callback-overload test method is incorrect"
            )
        elif not python_path.is_file() or callback_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                "Python carrier façade matrix callback-overload test method is missing: "
                + str(callback_test)
            )
        return_handle_test = carrier_matrix.get("return_handle_test_method")
        if return_handle_test != EXPECTED_PYTHON_RETURN_HANDLE_TEST:
            findings.append(
                "Python carrier façade matrix returned-handle test method is incorrect"
            )
        elif not python_path.is_file() or return_handle_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                "Python carrier façade matrix returned-handle test method is missing: "
                + str(return_handle_test)
            )
        if set(carrier_matrix.get("return_services", [])) != (
            EXPECTED_PYTHON_RETURN_SERVICES
        ):
            findings.append(
                "Python carrier façade matrix must cover every standard retraction-return service"
            )
    callback_matrix = catalog.get("callback_surface_matrix")
    if not isinstance(callback_matrix, dict):
        findings.append("callback_surface_matrix must be an object")
    else:
        if callback_matrix.get("method_count") != EXPECTED_CALLBACK_METHOD_COUNT:
            findings.append(
                "callback surface matrix method count must remain 51"
            )
        if callback_matrix.get("overload_count") != EXPECTED_CALLBACK_OVERLOAD_COUNT:
            findings.append(
                "callback surface matrix overload count must remain 60"
            )
        if callback_matrix.get("time_implementations") != [
            "HLAinteger64Time",
            "HLAfloat64Time",
        ]:
            findings.append(
                "callback surface matrix must exercise both standard time carriers"
            )
        if callback_matrix.get("round_trip_direction") != (
            "JNI-produced standard Java callback carriers -> JPype callback proxy -> Python callback arguments"
        ):
            findings.append(
                "callback surface matrix round-trip direction is incorrect"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = callback_matrix.get("python_test_method")
        if python_test != EXPECTED_PYTHON_CALLBACK_SURFACE_TEST:
            findings.append("callback surface matrix Python test method is incorrect")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                "callback surface matrix Python test method is missing: "
                + str(python_test)
            )
    return_matrix = catalog.get("python_return_surface_matrix")
    if not isinstance(return_matrix, dict):
        findings.append("python_return_surface_matrix must be an object")
    else:
        if return_matrix.get("service_return_overload_count") != (
            EXPECTED_RETURN_OVERLOAD_COUNT
        ):
            findings.append(
                "Python return surface matrix overload count must remain 50"
            )
        if return_matrix.get("time_implementations") != [
            "HLAinteger64Time",
            "HLAfloat64Time",
        ]:
            findings.append(
                "Python return surface matrix must exercise both standard time carriers"
            )
        if return_matrix.get("round_trip_direction") != (
            "JNI-produced standard Java RTI return carriers -> Java-shaped implementation -> Python RTIambassador return values"
        ):
            findings.append(
                "Python return surface matrix round-trip direction is incorrect"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = return_matrix.get("python_test_method")
        if python_test != EXPECTED_PYTHON_RETURN_SURFACE_TEST:
            findings.append(
                "Python return surface matrix Python test method is incorrect"
            )
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                "Python return surface matrix test method is missing: "
                + str(python_test)
            )
    factory_matrix = catalog.get("python_factory_surface_matrix")
    if not isinstance(factory_matrix, dict):
        findings.append("python_factory_surface_matrix must be an object")
    else:
        if factory_matrix.get("factory_name") != "Umbra JNI IEEE 1516e Null RTI":
            findings.append("Python factory surface matrix provider name is incorrect")
        if factory_matrix.get("factory_version") != "0.1.0-null":
            findings.append("Python factory surface matrix provider version is incorrect")
        if factory_matrix.get("discovery") != (
            "standard hla.rti1516e.RtiFactoryFactory / ServiceLoader"
        ):
            findings.append("Python factory surface matrix must use standard discovery")
        if factory_matrix.get("rtiambassador_method_count") != (
            EXPECTED_RTIAMBASSADOR_METHOD_COUNT
        ):
            findings.append("Python factory surface matrix method count must remain 150")
        if factory_matrix.get("rtiambassador_overload_count") != (
            EXPECTED_RTIAMBASSADOR_OVERLOAD_COUNT
        ):
            findings.append(
                "Python factory surface matrix overload count must remain 172"
            )
        if factory_matrix.get("lifecycle_services") != ["connect", "disconnect"]:
            findings.append(
                "Python factory surface matrix must cover connect and disconnect"
            )
        if factory_matrix.get("encoder_factory") != (
            "standard hla.rti1516e.encoding.EncoderFactory"
        ):
            findings.append("Python factory surface matrix encoder factory is incorrect")
        if factory_matrix.get("round_trip_direction") != (
            "Python Java2010RtiFactory -> standard Java RtiFactory -> Java RTIambassador proxy -> JPype -> Python façade"
        ):
            findings.append(
                "Python factory surface matrix round-trip direction is incorrect"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = factory_matrix.get("python_test_method")
        if python_test != EXPECTED_PYTHON_FACTORY_SURFACE_TEST:
            findings.append("Python factory surface matrix Python test method is incorrect")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                "Python factory surface matrix test method is missing: "
                + str(python_test)
            )
    service_matrix = catalog.get("python_service_argument_matrix")
    if not isinstance(service_matrix, dict):
        findings.append("python_service_argument_matrix must be an object")
    else:
        if service_matrix.get("service_overload_count") != (
            EXPECTED_PYTHON_SERVICE_ARGUMENT_OVERLOAD_COUNT
        ):
            findings.append(
                "Python service argument matrix overload count must remain 169"
            )
        if service_matrix.get("time_implementations") != [
            "HLAinteger64Time",
            "HLAfloat64Time",
        ]:
            findings.append(
                "Python service argument matrix must exercise both standard time carriers"
            )
        if service_matrix.get("lifecycle_services") != ["connect", "disconnect"]:
            findings.append(
                "Python service argument matrix must cover connect and disconnect"
            )
        if service_matrix.get("factory_carriers") != [
            "LogicalTimeFactory",
            "HandleFactory",
            "HandleSetFactory",
            "HandleValueMapFactory",
            "AttributeSetRegionSetPairListFactory",
        ]:
            findings.append(
                "Python service argument matrix factory carrier inventory is incorrect"
            )
        if service_matrix.get("expected_unsupported_result") != (
            "hla.rti1516e.exceptions.RTIinternalError"
        ):
            findings.append(
                "Python service argument matrix must preserve RTIinternalError"
            )
        if service_matrix.get("round_trip_direction") != (
            "Provider-neutral Python arguments -> JPype conversion -> standard Java RTIambassador proxy -> JNI -> C++ null-service result"
        ):
            findings.append(
                "Python service argument matrix round-trip direction is incorrect"
            )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        python_test = service_matrix.get("python_test_method")
        if python_test != EXPECTED_PYTHON_SERVICE_ARGUMENT_TEST:
            findings.append(
                "Python service argument matrix Python test method is incorrect"
            )
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                "Python service argument matrix test method is missing: "
                + str(python_test)
            )
    value_matrix = catalog.get("basic_value_matrix")
    if not isinstance(value_matrix, dict):
        findings.append("basic_value_matrix must be an object")
    else:
        if value_matrix.get("edition") != "2010":
            findings.append("basic value matrix must target IEEE 1516e-2010")
        if set(value_matrix.get("data_elements", [])) != EXPECTED_BASIC_DATA_ELEMENTS:
            findings.append(
                "basic value matrix must cover every standard 2010 basic data element"
            )
        if value_matrix.get("vector_count") != EXPECTED_BASIC_VALUE_MATRIX_COUNT:
            findings.append("basic value matrix vector count must remain 79")
        if value_matrix.get("wire_owned_by_provider") is not True:
            findings.append("basic value matrix must leave wire encoding to providers")
        support_path = (
            ROOT
            / "packages"
            / "umbra-rti-test-support"
            / "src"
            / "umbra_rti_test_support"
            / "data_element_matrix.py"
        )
        if value_matrix.get("shared_support") != str(
            support_path.relative_to(ROOT)
        ).replace("\\", "/"):
            findings.append("basic value matrix shared support path is incorrect")
        methods = value_matrix.get("python_test_methods")
        native_path = (
            ROOT / "packages" / "umbra-rti-native" / "tests" / "test_native_2010.py"
        )
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / "test_jpype_2010_jni_type_roundtrip.py"
        )
        if not isinstance(methods, list) or len(methods) != 2:
            findings.append(
                "basic value matrix requires native and JNI Python test methods"
            )
        else:
            if not native_path.is_file() or methods[0] not in native_path.read_text(
                encoding="utf-8"
            ):
                findings.append(
                    f"basic value matrix native test method is missing: {methods[0]}"
                )
            if not python_path.is_file() or methods[1] not in python_path.read_text(
                encoding="utf-8"
            ):
                findings.append(
                    f"basic value matrix JNI test method is missing: {methods[1]}"
                )
    malformed_matrix = catalog.get("malformed_wire_matrix")
    if not isinstance(malformed_matrix, dict):
        findings.append("malformed_wire_matrix must be an object")
    else:
        truncated = set(malformed_matrix.get("truncated_data_elements", []))
        if truncated != EXPECTED_DATA_ELEMENTS:
            findings.append(
                "malformed truncated matrix must cover every standard data element"
            )
        overrun = set(malformed_matrix.get("declared_length_overrun", []))
        if overrun != EXPECTED_MALFORMED_LENGTH_PREFIX_KINDS:
            findings.append(
                "malformed declared-length matrix must cover every length-prefixed carrier"
            )
        if malformed_matrix.get("java_exception") != (
            "hla.rti1516e.encoding.DecoderException"
        ):
            findings.append(
                "malformed wire matrix must preserve Java DecoderException identity"
            )
        python_test = malformed_matrix.get("python_test_method")
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / ("test_jpype_2010_jni_type_roundtrip.py")
        )
        if not isinstance(python_test, str) or not python_test:
            findings.append("malformed wire matrix requires a Python test method")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"malformed wire matrix Python test method is missing: {python_test}"
            )
    cursor_matrix = catalog.get("byte_wrapper_cursor_matrix")
    if not isinstance(cursor_matrix, dict):
        findings.append("byte_wrapper_cursor_matrix must be an object")
    else:
        if set(cursor_matrix.get("data_elements", [])) != EXPECTED_DATA_ELEMENTS:
            findings.append(
                "ByteWrapper cursor matrix must cover every standard data element"
            )
        if set(cursor_matrix.get("decode_overloads", [])) != (
            EXPECTED_DATA_ELEMENT_DECODE_OVERLOADS
        ):
            findings.append(
                "ByteWrapper cursor matrix must declare both decode overloads"
            )
        if cursor_matrix.get("preserves_trailing_octet") is not True:
            findings.append(
                "ByteWrapper cursor matrix must require trailing-octet preservation"
            )
        python_test = cursor_matrix.get("python_test_method")
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / ("test_jpype_2010_jni_type_roundtrip.py")
        )
        if not isinstance(python_test, str) or not python_test:
            findings.append("ByteWrapper cursor matrix requires a Python test method")
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"ByteWrapper cursor matrix Python test method is missing: {python_test}"
            )
    facade_matrix = catalog.get("python_encoder_facade_matrix")
    if not isinstance(facade_matrix, dict):
        findings.append("python_encoder_facade_matrix must be an object")
    else:
        if set(facade_matrix.get("data_elements", [])) != EXPECTED_DATA_ELEMENTS:
            findings.append(
                "Python encoder façade matrix must cover every standard data element"
            )
        if facade_matrix.get("factory_facade") != EXPECTED_PYTHON_ENCODER_FACADE:
            findings.append(
                "Python encoder façade matrix must identify JavaEncoderFactory"
            )
        creator_surface = facade_matrix.get("creator_surface")
        if not isinstance(creator_surface, dict):
            findings.append("Python encoder façade matrix creator_surface must be an object")
        else:
            if creator_surface.get("creator_count") != EXPECTED_ENCODER_CREATOR_COUNT:
                findings.append(
                    "2010 EncoderFactory creator surface count must remain 24"
                )
            java_path = (
                ROOT
                / "packages"
                / "umbra-rti-jni-2010"
                / "src"
                / "main"
                / "java"
                / "org"
                / "umbra"
                / "jni"
                / "rti1516e"
                / "NativeTypeRoundTripTest.java"
            )
            java_source = (
                java_path.read_text(encoding="utf-8") if java_path.is_file() else ""
            )
            for field, expected in (
                ("java_test_method", EXPECTED_ENCODER_CREATOR_JAVA_TEST),
                (
                    "java_composite_argument_test_method",
                    EXPECTED_ENCODER_COMPOSITE_JAVA_TEST,
                ),
            ):
                method = creator_surface.get(field)
                if method != expected:
                    findings.append(f"2010 EncoderFactory {field} is incorrect")
                elif method not in java_source:
                    findings.append(f"2010 EncoderFactory Java test method is missing: {method}")
            python_creator_path = (
                ROOT
                / "packages"
                / "umbra-rti-jpype"
                / "tests"
                / "test_jpype_2010_jni_type_roundtrip.py"
            )
            python_creator_source = (
                python_creator_path.read_text(encoding="utf-8")
                if python_creator_path.is_file()
                else ""
            )
            for field, expected in (
                ("python_test_method", EXPECTED_ENCODER_CREATOR_PYTHON_TEST),
                (
                    "python_composite_argument_test_method",
                    EXPECTED_ENCODER_COMPOSITE_PYTHON_TEST,
                ),
            ):
                method = creator_surface.get(field)
                if method != expected:
                    findings.append(f"2010 EncoderFactory {field} is incorrect")
                elif method not in python_creator_source:
                    findings.append(
                        f"2010 EncoderFactory Python test method is missing: {method}"
                    )
        python_test = facade_matrix.get("python_test_method")
        python_path = (
            ROOT
            / "packages"
            / "umbra-rti-jpype"
            / "tests"
            / ("test_jpype_2010_jni_type_roundtrip.py")
        )
        if not isinstance(python_test, str) or not python_test:
            findings.append(
                "Python encoder façade matrix requires a Python test method"
            )
        elif not python_path.is_file() or python_test not in python_path.read_text(
            encoding="utf-8"
        ):
            findings.append(
                f"Python encoder façade test method is missing: {python_test}"
            )
    by_category: dict[str, set[str]] = {}
    for item in carriers:
        if not isinstance(item, dict):
            findings.append("carrier entries must be objects")
            continue
        category = item.get("category")
        probe = item.get("probe")
        status = item.get("status")
        if not isinstance(category, str) or not isinstance(probe, str):
            findings.append("each carrier requires category and probe")
            continue
        if status != "implemented":
            findings.append(
                f"implemented matrix carrier is not marked implemented: {probe}"
            )
        by_category.setdefault(category, set()).add(probe)
    if EXPECTED_DATA_ELEMENTS - by_category.get("encoding", set()):
        findings.append(
            "missing data-element probes: "
            + ", ".join(
                sorted(EXPECTED_DATA_ELEMENTS - by_category.get("encoding", set()))
            )
        )
    if EXPECTED_HANDLES - by_category.get("handle", set()):
        findings.append(
            "missing handle probes: "
            + ", ".join(sorted(EXPECTED_HANDLES - by_category.get("handle", set())))
        )
    if EXPECTED_JAVA_ONLY_HANDLES - by_category.get("java-only-handle", set()):
        findings.append(
            "missing Java-only handle probes: "
            + ", ".join(
                sorted(
                    EXPECTED_JAVA_ONLY_HANDLES
                    - by_category.get("java-only-handle", set())
                )
            )
        )
    if EXPECTED_TIMES - by_category.get("logical-time", set()):
        findings.append(
            "missing logical-time probes: "
            + ", ".join(sorted(EXPECTED_TIMES - by_category.get("logical-time", set())))
        )
    for category, expected in EXPECTED_AUXILIARY.items():
        if expected - by_category.get(category, set()):
            findings.append(
                f"missing {category} probes: "
                + ", ".join(sorted(expected - by_category.get(category, set())))
            )
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    args = parser.parse_args()
    try:
        findings = verify(args.catalog)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(
            f"IEEE 1516e JNI type round-trip verification: FAIL: {error}",
            file=sys.stderr,
        )
        return 1
    if findings:
        print("IEEE 1516e JNI type round-trip verification: FAIL", file=sys.stderr)
        for finding in findings:
            print(f"  {finding}", file=sys.stderr)
        return 1
    print("IEEE 1516e JNI type round-trip verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
