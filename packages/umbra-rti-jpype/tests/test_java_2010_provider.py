from __future__ import annotations

import math
import struct
import unittest
from collections import Counter
from importlib.metadata import EntryPoint
from pathlib import Path
from unittest.mock import patch

from hla.rti1516e import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    FederateHandle,
    FederateHandleSaveStatusPair,
    FederateHandleSet,
    FederateRestoreStatus,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Time,
    InteractionClassHandle,
    LogicalTime,
    LogicalTimeInterval,
    MessageRetractionHandle,
    NullFederateAmbassador,
    ObjectClassHandle,
    ObjectInstanceHandle,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    ResignAction,
    RestoreFailureReason,
    RestoreStatus,
    RtiFactoryFactory,
    SaveFailureReason,
    SaveStatus,
    ServiceGroup,
    SupplementalReceiveInfo,
    SupplementalReflectInfo,
    SupplementalRemoveInfo,
    SynchronizationPointFailureReason,
    TransportationTypeHandle,
)
from hla.rti1516e.contracts import (
    FEDERATE_AMBASSADOR_METHODS,
    FEDERATE_AMBASSADOR_PARAMETER_TYPES,
    RTIAMBASSADOR_METHODS,
    RTIAMBASSADOR_PARAMETER_TYPES,
)
from hla.rti1516e.encoding import (
    ByteWrapper,
    DecoderException,
    EncoderException,
    EncoderFactory,
    HLAinteger32BE,
)
from hla.rti1516e.exceptions import (
    CouldNotDecode,
    InvalidLogicalTime,
    InvalidLogicalTimeInterval,
)
from umbra._java.rti1516e import (
    Java2010Integer64Time,
    Java2010RTIambassador,
    Java2010RtiFactory,
)
from umbra._java.rti1516e.encoding import (
    JavaEncoderFactory,
    _JavaDataElement,
    _wrap_element,
)
from umbra._java.rti1516e.runtime import JPype2010Runtime, _FederateAmbassadorProxy
from umbra_rti_test_support import (
    CallbackDeliveryObservation,
    SurfaceEventTrace,
    assert_callback_delivery_parity,
    assert_event_order,
    iter_callback_provenance_matrix,
    iter_vendor_data_element_encode_matrix,
    iter_vendor_data_element_wire_matrix,
    iter_logical_time_arithmetic_matrix,
    iter_logical_time_wire_matrix,
    iter_save_restore_matrix,
    iter_surface_matrix,
    iter_vendor_time_arithmetic_matrix,
)


class _FakeJavaAmbassador:
    def __init__(self) -> None:
        self.calls: list[tuple[str, tuple[object, ...]]] = []

    def connect(self, *args: object) -> None:
        self.calls.append(("connect", args))

    def disconnect(self) -> None:
        self.calls.append(("disconnect", ()))

    def getHLAversion(self) -> str:
        return "IEEE 1516.1-2010"

    def createFederationExecution(self, *args: object) -> None:
        self.calls.append(("createFederationExecution", args))

    def registerFederationSynchronizationPoint(self, *args: object) -> None:
        self.calls.append(("registerFederationSynchronizationPoint", args))

    def changeInteractionOrderType(self, *args: object) -> None:
        self.calls.append(("changeInteractionOrderType", args))

    def getOrderType(self, *args: object) -> object:
        self.calls.append(("getOrderType", args))
        return "java-order"

    def getTimeFactory(self):
        return _FakeJavaTimeFactory()


class _RecordingJavaAmbassador:
    """Dynamic Java-shaped object used to exercise every generated forwarder."""

    def __init__(self, time_factory: object | None = None) -> None:
        self.calls: list[tuple[str, tuple[object, ...]]] = []
        self.time_factory = time_factory or _FakeJavaTimeFactory()

    def __getattr__(self, name: str):
        if name == "getTimeFactory":

            def time_factory() -> object:
                self.calls.append((name, ()))
                return self.time_factory

            return time_factory

        def invoke(*args: object) -> None:
            self.calls.append((name, args))

        return invoke


class _FakeJavaFactory:
    def __init__(self) -> None:
        self.ambassador = _FakeJavaAmbassador()

    def getRtiAmbassador(self) -> _FakeJavaAmbassador:
        return self.ambassador

    def getEncoderFactory(self) -> object:
        return _FakeJavaEncoder()

    def rtiName(self) -> str:
        return "fake-2010"

    def rtiVersion(self) -> str:
        return "1.0"


class _FakeRuntime:
    def __init__(self) -> None:
        self.factory = _FakeJavaFactory()

    def get_rti_factory(self, configuration):
        return self.factory

    def callback_model(self, value):
        return f"java-{value.name}"

    def bind_federate_ambassador(self, callback):
        return type("Binding", (), {"proxy": "callback-proxy", "target": callback})()

    def byte_array(self, value):
        return bytes(value)

    def java_url(self, value):
        return f"url:{value}"

    def java_urls(self, values):
        return tuple(self.java_url(value) for value in values)

    def string_set(self, values):
        return set(values)

    def range_bounds(self, value):
        return (value.lower, value.upper)

    def exception_name(self, error):
        return None

    def decode_handle(self, ambassador, factory_name, encoded):
        return (factory_name, bytes(encoded))

    def java_enum(self, name, member):
        return (name, member)

    def from_java_value(self, value, expected_type):
        return (expected_type, value)

    def handle_set(self, ambassador, factory_name, values, set_factory_name=None):
        return ("set", factory_name, set_factory_name, tuple(values))

    def handle_value_map(self, ambassador, factory_name, values, map_factory_name=None):
        return ("map", factory_name, map_factory_name, dict(values))

    def raw_handle_set(self, values):
        return ("raw-set", tuple(values))

    def attribute_region_pair_list(self, ambassador, values, raw_handle_resolver=None):
        return ("pair-list", tuple(values), raw_handle_resolver)

    def java_int(self, value):
        return int(value)

    def java_byte_wrapper(self, value):
        return value

    def copy_from_java_byte_wrapper(self, raw, target):
        return None

    def data_element_factory(self, factory):
        return factory

    def handle_bytes(self, value):
        return bytes(value.encoded)

    def decode_logical_time(self, ambassador, encoded):
        return _FakeJavaTime(int.from_bytes(bytes(encoded), "big", signed=True))

    def decode_logical_interval(self, ambassador, encoded):
        return _FakeJavaInterval(int.from_bytes(bytes(encoded), "big", signed=True))


class _FakeClass:
    def __init__(self, name: str) -> None:
        self.name = name

    def getSimpleName(self) -> str:
        return self.name


class _FakeJavaInteger32:
    def __init__(self, value: int = 0) -> None:
        self.value = value

    def getClass(self):
        return _FakeClass("HLAinteger32BE")

    def getValue(self) -> int:
        return self.value

    def setValue(self, value: int) -> None:
        self.value = int(value)

    def getOctetBoundary(self) -> int:
        return 4

    def getEncodedLength(self) -> int:
        return 4

    def toByteArray(self) -> bytes:
        return int(self.value).to_bytes(4, "big", signed=True)

    def decode(self, value: bytes) -> None:
        self.value = int.from_bytes(bytes(value), "big", signed=True)

    def encode(self, wrapper) -> None:
        wrapper.put(self.toByteArray())


class _FakeJavaEncoder:
    def createHLAinteger32BE(self, value: int = 0) -> _FakeJavaInteger32:
        return _FakeJavaInteger32(value)

    def createHLAfixedRecord(self):
        return _FakeJavaRecord()

    def createHLAfixedArray(self, *elements):
        return _FakeJavaArray(list(elements))

    def createHLAvariableArray(self, factory, *elements):
        return _FakeJavaVariable(list(elements))


class _FakeVendorDataElement:
    """Unknown Java carrier used to test provider-owned wire preservation."""

    def __init__(self, payload: bytes = b"vendor") -> None:
        self.payload = bytes(payload)

    def getClass(self):
        return _FakeClass("VendorOpaqueData")

    def getOctetBoundary(self) -> int:
        return 1

    def getEncodedLength(self) -> int:
        return len(self.payload)

    def toByteArray(self) -> bytes:
        return self.payload

    def encode(self, wrapper) -> None:
        wrapper.put(self.payload)

    def decode(self, value) -> None:
        if hasattr(value, "getPos"):
            start = int(value.getPos())
            available = int(value.remaining())
            if available < len(self.payload):
                raise ValueError("vendor payload is truncated")
            self.payload = bytes(value.array())[start : start + len(self.payload)]
            value.setPosition(start + len(self.payload))
            return
        raw = bytes(value)
        if len(raw) != len(self.payload):
            raise ValueError("vendor payload length mismatch")
        self.payload = raw


class _FakeJavaRecord:
    def __init__(self, elements=()) -> None:
        self.elements = list(elements)

    def getClass(self):
        return _FakeClass("HLAfixedRecord")

    def add(self, value):
        self.elements.append(value)

    def size(self):
        return len(self.elements)

    def get(self, index):
        return self.elements[index]

    def getOctetBoundary(self):
        return 1

    def getEncodedLength(self):
        return sum(item.getEncodedLength() for item in self.elements)

    def toByteArray(self):
        return b"".join(item.toByteArray() for item in self.elements)

    def encode(self, wrapper):
        wrapper.put(self.toByteArray())

    def decode(self, value):
        return None


class _FakeJavaArray(_FakeJavaRecord):
    def getClass(self):
        return _FakeClass("HLAfixedArray")


class _FakeJavaVariable(_FakeJavaRecord):
    def getClass(self):
        return _FakeClass("HLAvariableArray")

    def addElement(self, value):
        self.elements.append(value)

    def resize(self, size):
        while len(self.elements) < size:
            self.elements.append(_FakeJavaInteger32())
        del self.elements[size:]


class _FakeJavaTime:
    def __init__(self, value: int) -> None:
        self.value = value
        self.encoded = int(value).to_bytes(8, "big", signed=True)

    def isInitial(self) -> bool:
        return self.value == 0

    def getClass(self):
        return _FakeClass("HLAinteger64Time")

    def isFinal(self) -> bool:
        return False

    def encodedLength(self) -> int:
        return 8

    def encode(self, buffer, offset) -> None:
        buffer[offset : offset + 8] = self.encoded

    def toString(self) -> str:
        return str(self.value)

    def add(self, interval):
        return _FakeJavaTime(self.value + interval.value)

    def subtract(self, interval):
        return _FakeJavaTime(self.value - interval.value)

    def distance(self, other):
        return _FakeJavaInterval(self.value - other.value)

    def compareTo(self, other):
        return (self.value > other.value) - (self.value < other.value)


class _FakeJavaInterval:
    def __init__(self, value: int) -> None:
        self.value = value
        self.encoded = int(value).to_bytes(8, "big", signed=True)

    def isZero(self) -> bool:
        return self.value == 0

    def isEpsilon(self) -> bool:
        return self.value == 1

    def encodedLength(self) -> int:
        return 8

    def encode(self, buffer, offset) -> None:
        buffer[offset : offset + 8] = self.encoded

    def toString(self) -> str:
        return str(self.value)

    def add(self, interval):
        return _FakeJavaInterval(self.value + interval.value)

    def subtract(self, interval):
        return _FakeJavaInterval(self.value - interval.value)

    def compareTo(self, other):
        return (self.value > other.value) - (self.value < other.value)


class _FakeJavaTimeFactory:
    def getName(self) -> str:
        return "HLAinteger64Time"

    def decodeTime(self, encoded, offset):
        return _FakeJavaTime(int.from_bytes(bytes(encoded), "big", signed=True))

    def decodeInterval(self, encoded, offset):
        return _FakeJavaInterval(int.from_bytes(bytes(encoded), "big", signed=True))

    def makeInitial(self):
        return _FakeJavaTime(0)

    def makeFinal(self):
        return _FakeJavaTime(2**31)

    def makeZero(self):
        return _FakeJavaInterval(0)

    def makeEpsilon(self):
        return _FakeJavaInterval(1)

    def makeTime(self, value):
        return _FakeJavaTime(int(value))

    def makeInterval(self, value):
        return _FakeJavaInterval(int(value))


class _FakeJavaFloatTime:
    """Small Java-shaped float64 logical-time carrier for the 2010 adapter."""

    def __init__(self, value: float) -> None:
        self.value = float(value)
        self.encoded = struct.pack(">d", self.value)

    def isInitial(self) -> bool:
        return self.value == 0.0

    def isFinal(self) -> bool:
        return self.value == float.fromhex("0x1.fffffffffffffp+1023")

    def getClass(self):
        return _FakeClass("HLAfloat64Time")

    def encodedLength(self) -> int:
        return 8

    def encode(self, buffer, offset) -> None:
        buffer[offset : offset + 8] = self.encoded

    def toString(self) -> str:
        return str(self.value)

    def add(self, interval):
        result = self.value + interval.value
        if interval.value == math.nextafter(0.0, 1.0) and result == self.value:
            result = math.nextafter(self.value, math.inf)
        return _FakeJavaFloatTime(result)

    def subtract(self, interval):
        result = self.value - interval.value
        if interval.value == math.nextafter(0.0, 1.0) and result == self.value:
            result = math.nextafter(self.value, 0.0)
        return _FakeJavaFloatTime(result)

    def distance(self, other):
        return _FakeJavaFloatInterval(self.value - other.value)

    def compareTo(self, other):
        return (self.value > other.value) - (self.value < other.value)


class _FakeJavaFloatInterval:
    """Small Java-shaped float64 logical-time interval carrier."""

    def __init__(self, value: float) -> None:
        self.value = float(value)
        self.encoded = struct.pack(">d", self.value)

    def isZero(self) -> bool:
        return self.value == 0.0

    def isEpsilon(self) -> bool:
        return self.value == math.nextafter(0.0, 1.0)

    def encodedLength(self) -> int:
        return 8

    def encode(self, buffer, offset) -> None:
        buffer[offset : offset + 8] = self.encoded

    def toString(self) -> str:
        return str(self.value)

    def add(self, interval):
        return _FakeJavaFloatInterval(self.value + interval.value)

    def subtract(self, interval):
        return _FakeJavaFloatInterval(self.value - interval.value)

    def compareTo(self, other):
        return (self.value > other.value) - (self.value < other.value)


class _FakeJavaFloatTimeFactory:
    def getName(self) -> str:
        return "HLAfloat64Time"

    def decodeTime(self, encoded, offset):
        value = struct.unpack(">d", bytes(encoded)[offset : offset + 8])[0]
        return _FakeJavaFloatTime(value)

    def decodeInterval(self, encoded, offset):
        value = struct.unpack(">d", bytes(encoded)[offset : offset + 8])[0]
        return _FakeJavaFloatInterval(value)

    def makeInitial(self):
        return _FakeJavaFloatTime(0.0)

    def makeFinal(self):
        return _FakeJavaFloatTime(float.fromhex("0x1.fffffffffffffp+1023"))

    def makeZero(self):
        return _FakeJavaFloatInterval(0.0)

    def makeEpsilon(self):
        return _FakeJavaFloatInterval(math.nextafter(0.0, 1.0))

    def makeTime(self, value):
        return _FakeJavaFloatTime(float(value))

    def makeInterval(self, value):
        return _FakeJavaFloatInterval(float(value))


class _FakeJavaError(Exception):
    """Java-shaped exception used by provider decode fixtures."""

    def __init__(self, name: str, message: str) -> None:
        super().__init__(message)
        self.name = name


class _FakeVendorTime(_FakeJavaFloatTime):
    """Vendor time carrier with deliberately non-standard arithmetic result."""

    def getClass(self):
        return _FakeClass("VendorLogicalTime")

    def add(self, interval):
        return ("vendor-add", self.value, interval.value)

    def subtract(self, interval):
        return ("vendor-subtract", self.value, interval.value)

    def distance(self, other):
        return ("vendor-distance", self.value, other.value)

    def compareTo(self, other):
        del other
        return "vendor-time-compare"


class _FakeVendorInterval(_FakeJavaFloatInterval):
    def getClass(self):
        return _FakeClass("VendorLogicalTimeInterval")

    def add(self, interval):
        return ("vendor-interval-add", self.value, interval.value)

    def subtract(self, interval):
        return ("vendor-interval-subtract", self.value, interval.value)

    def compareTo(self, other):
        del other
        return "vendor-interval-compare"


class _FakeVendorTimeFactory(_FakeJavaFloatTimeFactory):
    """Java-shaped factory whose carrier semantics are provider-owned."""

    def getName(self) -> str:
        return "VendorLogicalTime"

    def decodeTime(self, encoded, offset):
        return _FakeVendorTime(
            struct.unpack(">d", bytes(encoded)[offset : offset + 8])[0]
        )

    def decodeInterval(self, encoded, offset):
        return _FakeVendorInterval(
            struct.unpack(">d", bytes(encoded)[offset : offset + 8])[0]
        )

    def makeInitial(self):
        return _FakeVendorTime(0.0)

    def makeFinal(self):
        return _FakeVendorTime(float.fromhex("0x1.fffffffffffffp+1023"))

    def makeZero(self):
        return _FakeVendorInterval(0.0)

    def makeEpsilon(self):
        return _FakeVendorInterval(math.nextafter(0.0, 1.0))

    def makeTime(self, value):
        return _FakeVendorTime(float(value))

    def makeInterval(self, value):
        return _FakeVendorInterval(float(value))


class _FakeWideVendorTime(_FakeVendorTime):
    """Provider-owned 12-octet 2010 logical-time carrier."""

    def __init__(self, value: float) -> None:
        super().__init__(value)
        self.encoded = b"CHRN" + struct.pack(">d", self.value)

    def encodedLength(self) -> int:
        return 12

    def encode(self, buffer, offset) -> None:
        buffer[offset : offset + 12] = self.encoded


class _FakeWideVendorInterval(_FakeVendorInterval):
    """Provider-owned 12-octet 2010 logical-time interval carrier."""

    def __init__(self, value: float) -> None:
        super().__init__(value)
        self.encoded = b"INTV" + struct.pack(">d", self.value)

    def encodedLength(self) -> int:
        return 12

    def encode(self, buffer, offset) -> None:
        buffer[offset : offset + 12] = self.encoded


class _FakeWideVendorTimeFactory(_FakeVendorTimeFactory):
    """2010 factory whose provider wire carriers are wider than eight octets."""

    def getName(self) -> str:
        return "VendorLogicalTimeWide"

    def decodeTime(self, encoded, offset):
        payload = bytes(encoded)[offset:]
        if len(payload) != 12 or payload[:4] != b"CHRN":
            raise _FakeJavaError("CouldNotDecode", "invalid wide vendor time")
        return _FakeWideVendorTime(struct.unpack(">d", payload[4:])[0])

    def decodeInterval(self, encoded, offset):
        payload = bytes(encoded)[offset:]
        if len(payload) != 12 or payload[:4] != b"INTV":
            raise _FakeJavaError("CouldNotDecode", "invalid wide vendor interval")
        return _FakeWideVendorInterval(struct.unpack(">d", payload[4:])[0])

    def makeInitial(self):
        return _FakeWideVendorTime(0.0)

    def makeFinal(self):
        return _FakeWideVendorTime(float.fromhex("0x1.fffffffffffffp+1023"))

    def makeZero(self):
        return _FakeWideVendorInterval(0.0)

    def makeEpsilon(self):
        return _FakeWideVendorInterval(math.nextafter(0.0, 1.0))

    def makeTime(self, value):
        return _FakeWideVendorTime(float(value))

    def makeInterval(self, value):
        return _FakeWideVendorInterval(float(value))


class _SurfaceMatrixFederateAmbassador(NullFederateAmbassador):
    """Minimal callback sink shared by the 2010 binding matrix."""

    def __init__(self, trace: SurfaceEventTrace) -> None:
        self.trace = trace

    def timeRegulationEnabled(self, time: object) -> None:
        del time
        self.trace.record("timeRegulationEnabled")

    def timeConstrainedEnabled(self, time: object) -> None:
        del time
        self.trace.record("timeConstrainedEnabled")

    def timeAdvanceGrant(self, time: object) -> None:
        del time
        self.trace.record("timeAdvanceGrant")


class Java2010ProviderTests(unittest.TestCase):
    def test_provider_neutral_callback_and_save_overload_matrix(self) -> None:
        """Exercise the transplantable matrix against a Java-shaped 2010 RTI."""

        cases = iter_surface_matrix()
        for case in cases:
            with self.subTest(case=case.case_id):
                ambassadors: list[Java2010RTIambassador] = []
                traces: list[SurfaceEventTrace] = []
                try:
                    for member in range(case.member_count):
                        runtime = _FakeRuntime()
                        implementation = _RecordingJavaAmbassador(
                            _FakeJavaFloatTimeFactory()
                            if case.time_implementation == "HLAfloat64Time"
                            else _FakeJavaTimeFactory()
                        )
                        ambassador = Java2010RTIambassador(implementation, runtime)
                        trace = SurfaceEventTrace()
                        callback = _SurfaceMatrixFederateAmbassador(trace)
                        ambassador.connect(callback, CallbackModel[case.callback_model])
                        time_factory = ambassador.getTimeFactory()
                        if case.time_implementation == "HLAfloat64Time":
                            target = time_factory.makeTime(5.5)
                            lookahead = time_factory.makeInterval(0.25)
                            self.assertIsInstance(target, HLAfloat64Time)
                            self.assertIsInstance(lookahead, HLAfloat64Interval)
                        else:
                            target = time_factory.makeTime(5)
                            lookahead = time_factory.makeInterval(1)
                            self.assertIsInstance(target, HLAinteger64Time)
                        encoded_target = target.toByteArray()
                        encoded_interval = lookahead.toByteArray()
                        self.assertEqual(
                            time_factory.decodeTime(
                                b"\x5a" + encoded_target + b"\xa5", 1
                            ).getTime(),
                            target.getTime(),
                        )
                        self.assertEqual(
                            time_factory.decodeInterval(
                                b"\x5a" + encoded_interval + b"\xa5", 1
                            ).getInterval(),
                            lookahead.getInterval(),
                        )
                        self.assertEqual(
                            target.add(lookahead).getTime(),
                            target.getTime() + lookahead.getInterval(),
                        )
                        self.assertEqual(
                            target.add(lookahead).distance(target).getInterval(),
                            lookahead.getInterval(),
                        )
                        ambassador.enableTimeRegulation(lookahead)
                        ambassador.enableTimeConstrained()
                        label = f"matrix-{case.callback_model}-{member}"
                        if case.save_kind == "timestamped":
                            ambassador.requestFederationSave(label, target)
                        else:
                            ambassador.requestFederationSave(label)
                        getattr(ambassador, case.advance_service)(target)

                        # A real Java provider emits these through its callback
                        # proxy.  Drive the same proxy directly here so this
                        # source-checkout test remains independent of a JVM.
                        proxy = _FederateAmbassadorProxy(callback, runtime)
                        raw_target = implementation.time_factory.makeTime(
                            5.5 if case.time_implementation == "HLAfloat64Time" else 5
                        )
                        proxy.timeRegulationEnabled(raw_target)
                        proxy.timeConstrainedEnabled(raw_target)
                        proxy.timeAdvanceGrant(raw_target)

                        ambassador.federateSaveBegun()
                        ambassador.federateSaveComplete()
                        ambassador.queryFederationRestoreStatus()
                        ambassador.requestFederationRestore(label)
                        ambassador.federateRestoreComplete()
                        assert_event_order(
                            trace.events,
                            "timeRegulationEnabled",
                            "timeConstrainedEnabled",
                            "timeAdvanceGrant",
                        )
                        names = [name for name, _args in implementation.calls]
                        self.assertIn("requestFederationSave", names)
                        self.assertIn(case.advance_service, names)
                        self.assertIn("requestFederationRestore", names)
                        self.assertEqual(len(trace.events), 3)
                        ambassadors.append(ambassador)
                        traces.append(trace)
                finally:
                    for ambassador in ambassadors:
                        ambassador.disconnect()

    def test_provider_neutral_save_restore_outcome_matrix(self) -> None:
        """Forward every save/restore outcome through the 1516e Java surface."""

        for case in iter_save_restore_matrix():
            with self.subTest(case=case.case_id):
                ambassadors: list[Java2010RTIambassador] = []
                implementations: list[_RecordingJavaAmbassador] = []
                try:
                    for member in range(case.member_count):
                        runtime = _FakeRuntime()
                        time_factory = (
                            _FakeJavaFloatTimeFactory()
                            if case.time_implementation == "HLAfloat64Time"
                            else _FakeJavaTimeFactory()
                        )
                        implementation = _RecordingJavaAmbassador(time_factory)
                        ambassador = Java2010RTIambassador(implementation, runtime)
                        callback = _SurfaceMatrixFederateAmbassador(SurfaceEventTrace())
                        ambassador.connect(callback, CallbackModel[case.callback_model])
                        factory = ambassador.getTimeFactory()
                        target = (
                            factory.makeTime(5.5)
                            if case.time_implementation == "HLAfloat64Time"
                            else factory.makeTime(5)
                        )
                        label = f"outcome-{case.save_outcome}-{member}"
                        if case.save_kind == "timestamped":
                            ambassador.requestFederationSave(label, target)
                        else:
                            ambassador.requestFederationSave(label)
                        getattr(ambassador, case.advance_service)(target)
                        ambassador.queryFederationSaveStatus()
                        ambassador.federateSaveBegun()
                        getattr(ambassador, case.save_service)()
                        ambassador.queryFederationRestoreStatus()
                        ambassador.requestFederationRestore(label)
                        getattr(ambassador, case.restore_service)()

                        names = [name for name, _args in implementation.calls]
                        self.assertIn("connect", names)
                        self.assertIn("queryFederationSaveStatus", names)
                        self.assertIn("requestFederationSave", names)
                        self.assertIn(case.advance_service, names)
                        self.assertIn(case.save_service, names)
                        self.assertIn("queryFederationRestoreStatus", names)
                        self.assertIn("requestFederationRestore", names)
                        self.assertIn(case.restore_service, names)
                        connect_args = next(
                            args
                            for name, args in implementation.calls
                            if name == "connect"
                        )
                        self.assertEqual(connect_args[1], f"java-{case.callback_model}")
                        save_call = next(
                            args
                            for name, args in implementation.calls
                            if name == "requestFederationSave"
                        )
                        self.assertEqual(
                            len(save_call), 2 if case.save_kind == "timestamped" else 1
                        )
                        self.assertEqual(
                            sum(name == case.save_service for name in names), 1
                        )
                        self.assertEqual(
                            sum(name == case.restore_service for name in names), 1
                        )
                        ambassadors.append(ambassador)
                        implementations.append(implementation)
                finally:
                    for ambassador in ambassadors:
                        ambassador.disconnect()

    def test_provider_consumes_shared_logical_time_wire_and_arithmetic_vectors(
        self,
    ) -> None:
        """Exercise both 1516e time families through the Java-shaped carrier."""

        for implementation in ("HLAinteger64Time", "HLAfloat64Time"):
            with self.subTest(implementation=implementation):
                runtime = _FakeRuntime()
                java_time_factory = (
                    _FakeJavaFloatTimeFactory()
                    if implementation == "HLAfloat64Time"
                    else _FakeJavaTimeFactory()
                )
                ambassador = Java2010RTIambassador(
                    _RecordingJavaAmbassador(java_time_factory), runtime
                )
                time_factory = ambassador.getTimeFactory()
                for vector in iter_logical_time_wire_matrix((implementation,)):
                    with self.subTest(vector=vector.case_id):
                        if vector.valid:
                            decoded_time = time_factory.decodeTime(
                                b"\x5a" + vector.encoded + b"\xa5", 1
                            )
                            decoded_interval = time_factory.decodeInterval(
                                b"\x5a" + vector.encoded + b"\xa5", 1
                            )
                            self.assertEqual(decoded_time.getTime(), vector.expected)
                            self.assertEqual(
                                decoded_interval.getInterval(), vector.expected
                            )
                        elif vector.label == "trailing":
                            # The 1516e Java overload takes an offset and
                            # intentionally permits a suffix after the fixed
                            # width value.  Prove that carrier shape while the
                            # 2025 exact-width route rejects this vector.
                            decoded_time = time_factory.decodeTime(
                                b"\x5a" + vector.encoded[:8] + b"\xa5", 1
                            )
                            decoded_interval = time_factory.decodeInterval(
                                b"\x5a" + vector.encoded[:8] + b"\xa5", 1
                            )
                            self.assertEqual(decoded_time.getTime(), 0)
                            self.assertEqual(decoded_interval.getInterval(), 0)
                        else:
                            with self.assertRaises(CouldNotDecode):
                                time_factory.decodeTime(vector.encoded)
                            with self.assertRaises(CouldNotDecode):
                                time_factory.decodeInterval(vector.encoded)
                for vector in iter_logical_time_arithmetic_matrix((implementation,)):
                    with self.subTest(arithmetic=vector.case_id):
                        base = time_factory.makeTime(vector.base)
                        interval = time_factory.makeInterval(vector.interval)
                        self.assertEqual(
                            base.add(interval).getTime(), vector.expected_sum
                        )
                        self.assertEqual(
                            base.subtract(interval).getTime(),
                            vector.expected_difference,
                        )
                        self.assertEqual(
                            base.add(interval).distance(base).getInterval(),
                            vector.expected_distance,
                        )

    def test_time_factory_preserves_unknown_vendor_carriers(self) -> None:
        """Unknown factory names never get guessed into a standard wrapper."""

        runtime = _FakeRuntime()
        ambassador = Java2010RTIambassador(
            _RecordingJavaAmbassador(_FakeVendorTimeFactory()), runtime
        )
        factory = ambassador.getTimeFactory()
        raw_time = _FakeJavaFloatTime(1.5)
        raw_interval = _FakeJavaFloatInterval(0.25)
        self.assertIs(factory.from_java_value(raw_time, "LogicalTime"), raw_time)
        self.assertIs(
            factory.from_java_value(raw_interval, "LogicalTimeInterval"), raw_interval
        )

        made_time = factory.makeTime(2.5)
        made_interval = factory.makeInterval(0.5)
        self.assertIsInstance(made_time, _FakeVendorTime)
        self.assertIsInstance(made_interval, _FakeVendorInterval)
        self.assertEqual(made_time.add(made_interval), ("vendor-add", 2.5, 0.5))
        decoded = factory.decodeTime(b"\x3f\xf8\x00\x00\x00\x00\x00\x00")
        self.assertIsInstance(decoded, _FakeVendorTime)
        self.assertEqual(decoded.value, 1.5)

        # Keep every provider-owned arithmetic call on the unknown Java
        # carrier.  The markers are intentionally fixture-specific: the
        # portable assertion is that Python does not replace these results
        # with standard HLAinteger64/HLAfloat64 values.
        other_time = factory.makeTime(1.5)
        for vector in iter_vendor_time_arithmetic_matrix():
            with self.subTest(operation=vector.case_id):
                receiver = made_time if vector.receiver == "time" else made_interval
                argument = other_time if vector.argument == "time" else made_interval
                result = getattr(receiver, vector.operation)(argument)
                if isinstance(result, tuple):
                    self.assertEqual(result[0], vector.expected_marker)
                else:
                    self.assertEqual(result, vector.expected_marker)

    def test_ambassador_queries_preserve_unknown_vendor_time_carriers(self) -> None:
        """2010 query returns retain the provider-owned Java carrier identity."""

        class Implementation(_RecordingJavaAmbassador):
            def __init__(self) -> None:
                super().__init__(_FakeVendorTimeFactory())
                self.current_time = _FakeVendorTime(4.5)
                self.current_lookahead = _FakeVendorInterval(0.25)

            def queryLogicalTime(self) -> object:
                self.calls.append(("queryLogicalTime", ()))
                return self.current_time

            def queryLookahead(self) -> object:
                self.calls.append(("queryLookahead", ()))
                return self.current_lookahead

            def queryGALT(self) -> object:
                self.calls.append(("queryGALT", ()))
                return type(
                    "RawQuery",
                    (),
                    {"timeIsValid": True, "time": self.current_time},
                )()

            def queryLITS(self) -> object:
                self.calls.append(("queryLITS", ()))
                return type(
                    "RawQuery",
                    (),
                    {"timeIsValid": True, "time": self.current_time},
                )()

        runtime = _FakeRuntime()
        implementation = Implementation()
        ambassador = Java2010RTIambassador(implementation, runtime)

        queried_time = ambassador.queryLogicalTime()
        queried_lookahead = ambassador.queryLookahead()
        self.assertIs(queried_time, implementation.current_time)
        self.assertIs(queried_lookahead, implementation.current_lookahead)
        for query in (ambassador.queryGALT(), ambassador.queryLITS()):
            with self.subTest(query=query):
                self.assertTrue(query.timeIsValid)
                self.assertIs(query.time, implementation.current_time)

    def test_time_factory_delegates_variable_width_vendor_wire(self) -> None:
        """The 2010 offset overload forwards unknown carrier widths to Java."""

        class Runtime(_FakeRuntime):
            def exception_name(self, error):
                return getattr(error, "name", None)

        runtime = Runtime()
        implementation = _RecordingJavaAmbassador(_FakeWideVendorTimeFactory())
        ambassador = Java2010RTIambassador(implementation, runtime)
        factory = ambassador.getTimeFactory()
        self.assertEqual(factory.implementationName(), "VendorLogicalTimeWide")

        time_wire = b"CHRN" + struct.pack(">d", 12.25)
        interval_wire = b"INTV" + struct.pack(">d", 0.75)
        decoded_time = factory.decodeTime(b"\x7e" + time_wire, 1)
        decoded_interval = factory.decodeInterval(b"\x7e" + interval_wire, 1)
        self.assertIsInstance(decoded_time, _FakeWideVendorTime)
        self.assertIsInstance(decoded_interval, _FakeWideVendorInterval)
        self.assertEqual(decoded_time.value, 12.25)
        self.assertEqual(decoded_interval.value, 0.75)
        self.assertEqual(decoded_time.encodedLength(), 12)
        self.assertEqual(decoded_interval.encodedLength(), 12)
        time_round_trip = bytearray(decoded_time.encodedLength())
        interval_round_trip = bytearray(decoded_interval.encodedLength())
        decoded_time.encode(time_round_trip, 0)
        decoded_interval.encode(interval_round_trip, 0)
        self.assertEqual(bytes(time_round_trip), time_wire)
        self.assertEqual(bytes(interval_round_trip), interval_wire)
        for value, wire in (
            (decoded_time, time_wire),
            (decoded_interval, interval_wire),
        ):
            with self.subTest(offset_carrier=type(value).__name__):
                destination = bytearray(b"\xaa" + wire + b"\xbb")
                value.encode(destination, 1)
                self.assertEqual(bytes(destination), b"\xaa" + wire + b"\xbb")

        malformed_time = (b"", time_wire[:-1], b"WRNG" + time_wire[4:], time_wire + b"\x00")
        malformed_interval = (
            b"",
            interval_wire[:-1],
            b"WRNG" + interval_wire[4:],
            interval_wire + b"\x00",
        )
        for encoded in malformed_time:
            with self.subTest(carrier="time", length=len(encoded)):
                with self.assertRaises(CouldNotDecode):
                    factory.decodeTime(encoded)
        for encoded in malformed_interval:
            with self.subTest(carrier="interval", length=len(encoded)):
                with self.assertRaises(CouldNotDecode):
                    factory.decodeInterval(encoded)

        class SentinelOnlyFactory(_FakeWideVendorTimeFactory):
            # The generic 2010 interface does not require concrete numeric
            # creators.  Verify the Python convenience spelling reports that
            # capability boundary explicitly when a provider omits both the
            # concrete and compatibility method names.
            makeTime = None
            makeInterval = None
            makeLogicalTime = None
            makeLogicalTimeInterval = None

        implementation.time_factory = SentinelOnlyFactory()
        sentinel_factory = ambassador.getTimeFactory()
        with self.assertRaises(NotImplementedError):
            sentinel_factory.makeTime(1)
        with self.assertRaises(NotImplementedError):
            sentinel_factory.makeInterval(1)

    def test_2010_raw_vendor_time_carriers_can_return_to_java_services(self) -> None:
        """Raw provider callbacks remain usable as later service arguments."""

        class Runtime(_FakeRuntime):
            def __init__(self) -> None:
                super().__init__()
                self.seen_time: bytes | None = None
                self.seen_interval: bytes | None = None

            def decode_logical_time(self, ambassador, encoded):
                del ambassador
                self.seen_time = bytes(encoded)
                return _FakeWideVendorTime(12.25)

            def decode_logical_interval(self, ambassador, encoded):
                del ambassador
                self.seen_interval = bytes(encoded)
                return _FakeWideVendorInterval(0.75)

        runtime = Runtime()
        implementation = _RecordingJavaAmbassador(_FakeWideVendorTimeFactory())
        ambassador = Java2010RTIambassador(implementation, runtime)
        raw_time = _FakeWideVendorTime(12.25)
        raw_interval = _FakeWideVendorInterval(0.75)

        converted_time = ambassador._convert_argument(raw_time, "LogicalTime")
        converted_interval = ambassador._convert_argument(
            raw_interval, "LogicalTimeInterval"
        )
        self.assertIsInstance(converted_time, _FakeWideVendorTime)
        self.assertIsInstance(converted_interval, _FakeWideVendorInterval)
        self.assertEqual(runtime.seen_time, raw_time.encoded)
        self.assertEqual(runtime.seen_interval, raw_interval.encoded)

        # Use the generated service forwarders as the transplant path, not
        # only the private conversion helper.  The Java-shaped implementation
        # must receive the provider-decoded carriers for both time argument
        # families.
        ambassador.timeAdvanceRequest(raw_time)
        ambassador.modifyLookahead(raw_interval)
        self.assertIsInstance(
            next(args for name, args in implementation.calls if name == "timeAdvanceRequest")[0],
            _FakeWideVendorTime,
        )
        self.assertIsInstance(
            next(args for name, args in implementation.calls if name == "modifyLookahead")[0],
            _FakeWideVendorInterval,
        )

    def test_standard_time_factory_rejects_malformed_numeric_carriers(self) -> None:
        """NaN, infinity, and negative values fail before Python wrapping."""

        runtime = _FakeRuntime()
        ambassador = Java2010RTIambassador(
            _RecordingJavaAmbassador(_FakeJavaFloatTimeFactory()), runtime
        )
        factory = ambassador.getTimeFactory()
        for value in (-1.0, math.inf, math.nan):
            with self.subTest(value=value):
                with self.assertRaises(InvalidLogicalTime):
                    factory.from_java_value(
                        _FakeJavaFloatTime(value), "LogicalTime"
                    )
                with self.assertRaises(InvalidLogicalTimeInterval):
                    factory.from_java_value(
                        _FakeJavaFloatInterval(value), "LogicalTimeInterval"
                    )

    def test_installed_style_entry_point_loading_preserves_the_2010_java_alias(
        self,
    ) -> None:
        """The package metadata route must load the standard JPype factory class."""

        entry_point = EntryPoint(
            name="java-2010",
            value="umbra._java.rti1516e:Java2010RtiFactory",
            group="hla.rti1516e.factories",
        )
        with patch("hla.rti1516e.core.entry_points", return_value=[entry_point]):
            factories = RtiFactoryFactory.getAvailableRtiFactories()
            selected = RtiFactoryFactory.getRtiFactory("java-2010")
        self.assertEqual(len(factories), 1)
        self.assertIsInstance(factories[0], Java2010RtiFactory)
        self.assertIsInstance(selected, Java2010RtiFactory)

    @staticmethod
    def _matrix_argument(expected_type: str) -> object:
        handles: dict[str, object] = {
            "AttributeHandle": AttributeHandle(b"attribute"),
            "DimensionHandle": DimensionHandle(b"dimension"),
            "FederateHandle": FederateHandle(b"federate"),
            "InteractionClassHandle": InteractionClassHandle(b"interaction"),
            "MessageRetractionHandle": MessageRetractionHandle(b"retraction"),
            "ObjectClassHandle": ObjectClassHandle(b"object-class"),
            "ObjectInstanceHandle": ObjectInstanceHandle(b"object-instance"),
            "ParameterHandle": ParameterHandle(b"parameter"),
            "TransportationTypeHandle": TransportationTypeHandle(b"transport"),
            "RegionHandle": RegionHandle(b"region"),
        }
        if expected_type in handles:
            return handles[expected_type]
        if expected_type == "AttributeHandleSet":
            return AttributeHandleSet((AttributeHandle(b"attribute"),))
        if expected_type == "DimensionHandleSet":
            return DimensionHandleSet((DimensionHandle(b"dimension"),))
        if expected_type == "FederateHandleSet":
            return FederateHandleSet((FederateHandle(b"federate"),))
        if expected_type == "RegionHandleSet":
            return RegionHandleSet((RegionHandle(b"region"),))
        if expected_type == "AttributeHandleValueMap":
            return AttributeHandleValueMap({AttributeHandle(b"attribute"): b"value"})
        if expected_type == "ParameterHandleValueMap":
            return ParameterHandleValueMap({ParameterHandle(b"parameter"): b"value"})
        if expected_type == "AttributeSetRegionSetPairList":
            return AttributeSetRegionSetPairList()
        if expected_type == "LogicalTime":
            return LogicalTime(encodedValue=b"\x00" * 8, value=0)
        if expected_type == "LogicalTimeInterval":
            return LogicalTimeInterval(encodedValue=b"\x00" * 8, value=0)
        if expected_type == "RangeBounds":
            return RangeBounds(0, 1)
        if expected_type == "CallbackModel":
            return CallbackModel.HLA_EVOKED
        if expected_type == "OrderType":
            return OrderType.RECEIVE
        if expected_type == "ResignAction":
            return ResignAction.NO_ACTION
        if expected_type == "ServiceGroup":
            return ServiceGroup.SUPPORT_SERVICES
        if expected_type == "URL[]":
            return (Path("fom.xml"),)
        if expected_type == "URL":
            return Path("fom.xml")
        if expected_type == "Set<String>":
            return {"federate"}
        if expected_type == "byte[]":
            return b"tag"
        if expected_type == "boolean":
            return False
        if expected_type in {"double", "long"}:
            return 1
        if expected_type == "String":
            return "value"
        if expected_type == "FederateAmbassador":
            return NullFederateAmbassador()
        raise AssertionError(f"matrix has no dummy for {expected_type}")

    def test_generated_rti_forwarding_matrix_covers_every_standard_overload(
        self,
    ) -> None:
        runtime = _FakeRuntime()
        implementation = _RecordingJavaAmbassador()
        ambassador = Java2010RTIambassador(implementation, runtime)
        raw_region = object()
        ambassador._raw_handles[(RegionHandle, b"region")] = raw_region
        raw_retraction = object()
        ambassador._raw_handles[(MessageRetractionHandle, b"retraction")] = (
            raw_retraction
        )
        factory_accessors = {
            "getAttributeHandleFactory",
            "getAttributeHandleSetFactory",
            "getAttributeHandleValueMapFactory",
            "getAttributeSetRegionSetPairListFactory",
            "getDimensionHandleFactory",
            "getDimensionHandleSetFactory",
            "getFederateHandleFactory",
            "getFederateHandleSetFactory",
            "getInteractionClassHandleFactory",
            "getObjectClassHandleFactory",
            "getObjectInstanceHandleFactory",
            "getParameterHandleFactory",
            "getParameterHandleValueMapFactory",
            "getRegionHandleSetFactory",
            "getTransportationTypeHandleFactory",
        }
        expected_calls: Counter[str] = Counter()
        for name in RTIAMBASSADOR_METHODS:
            self.assertTrue(callable(getattr(ambassador, name)), name)
            for overload in RTIAMBASSADOR_PARAMETER_TYPES[name]:
                args = tuple(self._matrix_argument(item) for item in overload)
                result = getattr(ambassador, name)(*args)
                if name not in factory_accessors:
                    expected_calls[name] += 1
                if name in factory_accessors:
                    self.assertIsNotNone(result)
        actual_calls = Counter(name for name, _ in implementation.calls)
        self.assertEqual(actual_calls, expected_calls)
        self.assertEqual(
            sum(actual_calls.values()),
            sum(
                len(RTIAMBASSADOR_PARAMETER_TYPES[name])
                for name in RTIAMBASSADOR_METHODS
                if name not in factory_accessors
            ),
        )

    def test_callback_proxy_matrix_covers_every_standard_callback_overload(
        self,
    ) -> None:
        runtime = _FakeRuntime()
        callback = NullFederateAmbassador()
        calls: list[tuple[str, tuple[object, ...]]] = []
        for name in FEDERATE_AMBASSADOR_METHODS:
            setattr(
                callback, name, lambda *args, _name=name: calls.append((_name, args))
            )
        proxy = _FederateAmbassadorProxy(callback, runtime)
        expected_calls: Counter[str] = Counter()
        for name in FEDERATE_AMBASSADOR_METHODS:
            self.assertTrue(callable(getattr(proxy, name)), name)
            for overload in FEDERATE_AMBASSADOR_PARAMETER_TYPES[name]:
                getattr(proxy, name)(*[object() for _ in overload])
                expected_calls[name] += 1
        self.assertEqual(Counter(name for name, _ in calls), expected_calls)
        self.assertEqual(sum(expected_calls.values()), 60)

    def test_callback_proxy_preserves_shared_callback_provenance_vectors(self) -> None:
        """Normalize 2010 supplemental callback records to shared semantics."""

        class _RawHandle:
            def __init__(self, encoded: bytes) -> None:
                self.encoded = bytes(encoded)

        class _RawEntry:
            def __init__(self, key: bytes, value: bytes) -> None:
                self._key = _RawHandle(key)
                self._value = bytes(value)

            def getKey(self) -> _RawHandle:
                return self._key

            def getValue(self) -> bytes:
                return self._value

        class _RawMap:
            def __init__(self, entry: _RawEntry) -> None:
                self._entry = entry

            def entrySet(self) -> tuple[tuple[_RawHandle, bytes], ...]:
                return ((self._entry.getKey(), self._entry.getValue()),)

        class _RawSupplement:
            def __init__(self, producer: bytes, region: bytes | None) -> None:
                self._producer = _RawHandle(producer)
                self._region = None if region is None else _RawHandle(region)

            def hasProducingFederate(self) -> bool:
                return True

            def hasSentRegions(self) -> bool:
                return self._region is not None

            def getProducingFederate(self) -> _RawHandle:
                return self._producer

            def getSentRegions(self) -> tuple[_RawHandle, ...] | None:
                return None if self._region is None else (self._region,)

        class _RawEnum:
            def __init__(self, name: str) -> None:
                self.name = name

        class _RawTime:
            def __init__(self, value: int | float, implementation: str) -> None:
                self.value = value
                self.implementation = implementation
                self.encoded = (
                    struct.pack(">q", int(value))
                    if implementation == "HLAinteger64Time"
                    else struct.pack(">d", float(value))
                )

            def getClass(self):
                return type(
                    "RawClass",
                    (),
                    {"getSimpleName": lambda _self: self.implementation},
                )()

            def isInitial(self) -> bool:
                return self.value == 0

            def isFinal(self) -> bool:
                return False

            def encodedLength(self) -> int:
                return len(self.encoded)

            def encode(self, buffer: bytearray, offset: int = 0) -> None:
                buffer[offset : offset + len(self.encoded)] = self.encoded

            def getTime(self) -> int | float:
                return self.value

            def toString(self) -> str:
                return str(self.value)

        class _Runtime(JPype2010Runtime):
            def handle_bytes(self, value: object) -> bytes:
                if isinstance(value, (bytes, bytearray)):
                    return bytes(value)
                encoded = getattr(value, "encoded", None)
                if encoded is not None:
                    return bytes(encoded)
                return bytes(getattr(value, "encodedValue"))

        class _Callback(NullFederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []
                self.interactions: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *args: object) -> None:
                self.discoveries.append(args)

            def reflectAttributeValues(self, *args: object) -> None:
                self.reflections.append(args)

            def receiveInteraction(self, *args: object) -> None:
                self.interactions.append(args)

        for vector in iter_callback_provenance_matrix():
            with self.subTest(case=vector.case_id):
                callback = _Callback()
                proxy = _FederateAmbassadorProxy(callback, _Runtime())
                if vector.callback == "discoverObjectInstance":
                    proxy.discoverObjectInstance(
                        _RawHandle(vector.object_instance),
                        _RawHandle(vector.object_class),
                        vector.instance_name,
                        _RawHandle(vector.producing_federate),
                    )
                    result = callback.discoveries[-1]
                    self.assertEqual(result[0].encodedValue, vector.object_instance)
                    self.assertEqual(result[1].encodedValue, vector.object_class)
                    self.assertEqual(result[2], vector.instance_name)
                    self.assertEqual(
                        result[3].encodedValue, vector.producing_federate
                    )
                    continue

                raw_map = _RawMap(
                    _RawEntry(vector.payload_handle, vector.payload)  # type: ignore[arg-type]
                )
                raw_supplement = _RawSupplement(
                    vector.producing_federate, vector.sent_region  # type: ignore[arg-type]
                )
                common = (
                    _RawHandle(vector.object_instance)
                    if vector.callback == "reflectAttributeValues"
                    else _RawHandle(vector.interaction_class),
                    raw_map,
                    vector.tag,
                )
                order = _RawEnum(
                    vector.received_order
                    if not vector.timed
                    else vector.sent_order  # type: ignore[arg-type]
                )
                if vector.callback == "reflectAttributeValues":
                    if vector.timed:
                        arguments = common + (
                            order,
                            _RawHandle(vector.transportation),  # type: ignore[arg-type]
                            _RawTime(vector.time_value, "HLAinteger64Time"),  # type: ignore[arg-type]
                            _RawEnum(vector.received_order),  # type: ignore[arg-type]
                            _RawHandle(vector.retraction),  # type: ignore[arg-type]
                            raw_supplement,
                        )
                    else:
                        arguments = common + (
                            order,
                            _RawHandle(vector.transportation),  # type: ignore[arg-type]
                            raw_supplement,
                        )
                else:
                    if vector.timed:
                        arguments = common + (
                            order,
                            _RawHandle(vector.transportation),  # type: ignore[arg-type]
                            _RawTime(vector.time_value, "HLAfloat64Time"),  # type: ignore[arg-type]
                            _RawEnum(vector.received_order),  # type: ignore[arg-type]
                            _RawHandle(vector.retraction),  # type: ignore[arg-type]
                            raw_supplement,
                        )
                    else:
                        arguments = common + (
                            order,
                            _RawHandle(vector.transportation),  # type: ignore[arg-type]
                            raw_supplement,
                        )
                getattr(proxy, vector.callback)(*arguments)
                result = (
                    callback.reflections[-1]
                    if vector.callback == "reflectAttributeValues"
                    else callback.interactions[-1]
                )
                self.assertEqual(result[2], vector.tag)
                self.assertEqual(result[4].encodedValue, vector.transportation)
                info = result[-1]
                self.assertEqual(
                    info.getProducingFederate().encodedValue,
                    vector.producing_federate,
                )
                if vector.sent_region is None:
                    self.assertFalse(info.hasSentRegions())
                else:
                    self.assertEqual(
                        next(iter(info.getSentRegions())).encodedValue,
                        vector.sent_region,
                    )
                if vector.callback == "reflectAttributeValues":
                    self.assertEqual(
                        result[1][AttributeHandle(vector.payload_handle)],  # type: ignore[arg-type]
                        vector.payload,
                    )
                else:
                    self.assertEqual(
                        result[1][ParameterHandle(vector.payload_handle)],  # type: ignore[arg-type]
                        vector.payload,
                    )
                if vector.timed:
                    self.assertEqual(result[5].getTime(), vector.time_value)
                    self.assertEqual(result[3], OrderType[vector.sent_order])  # type: ignore[index]
                    self.assertEqual(result[6], OrderType[vector.received_order])  # type: ignore[index]
                    self.assertEqual(result[7].encodedValue, vector.retraction)
                else:
                    self.assertEqual(result[3], OrderType[vector.received_order])  # type: ignore[index]

    def test_callback_delivery_normalizes_two_2010_member_streams(self) -> None:
        """Compare 2010 supplemental callbacks by semantic stream order."""

        class _RawHandle:
            def __init__(self, encoded: bytes) -> None:
                self.encoded = bytes(encoded)

        class _RawEntry:
            def __init__(self, key: bytes, value: bytes) -> None:
                self._key = _RawHandle(key)
                self._value = bytes(value)

            def getKey(self) -> _RawHandle:
                return self._key

            def getValue(self) -> bytes:
                return self._value

        class _RawMap:
            def __init__(self, entry: _RawEntry) -> None:
                self._entry = entry

            def entrySet(self) -> tuple[tuple[_RawHandle, bytes], ...]:
                return ((self._entry.getKey(), self._entry.getValue()),)

        class _RawSupplement:
            def __init__(self, producer: bytes, region: bytes | None) -> None:
                self._producer = _RawHandle(producer)
                self._region = None if region is None else _RawHandle(region)

            def hasProducingFederate(self) -> bool:
                return True

            def hasSentRegions(self) -> bool:
                return self._region is not None

            def getProducingFederate(self) -> _RawHandle:
                return self._producer

            def getSentRegions(self) -> tuple[_RawHandle, ...] | None:
                return None if self._region is None else (self._region,)

        class _RawEnum:
            def __init__(self, name: str) -> None:
                self.name = name

        class _RawTime:
            def __init__(self, value: int | float, implementation: str) -> None:
                self.value = value
                self.implementation = implementation
                self.encoded = (
                    struct.pack(">q", int(value))
                    if implementation == "HLAinteger64Time"
                    else struct.pack(">d", float(value))
                )

            def getClass(self):
                return type(
                    "RawClass",
                    (),
                    {"getSimpleName": lambda _self: self.implementation},
                )()

            def isInitial(self) -> bool:
                return self.value == 0

            def isFinal(self) -> bool:
                return False

            def encodedLength(self) -> int:
                return len(self.encoded)

            def encode(self, buffer: bytearray, offset: int = 0) -> None:
                buffer[offset : offset + len(self.encoded)] = self.encoded

            def getTime(self) -> int | float:
                return self.value

            def toString(self) -> str:
                return str(self.value)

        class _Runtime(JPype2010Runtime):
            def handle_bytes(self, value: object) -> bytes:
                if isinstance(value, (bytes, bytearray)):
                    return bytes(value)
                encoded = getattr(value, "encoded", None)
                if encoded is not None:
                    return bytes(encoded)
                return bytes(getattr(value, "encodedValue"))

        class _Callback(NullFederateAmbassador):
            def __init__(self) -> None:
                self.discoveries: list[tuple[object, ...]] = []
                self.reflections: list[tuple[object, ...]] = []
                self.interactions: list[tuple[object, ...]] = []

            def discoverObjectInstance(self, *args: object) -> None:
                self.discoveries.append(args)

            def reflectAttributeValues(self, *args: object) -> None:
                self.reflections.append(args)

            def receiveInteraction(self, *args: object) -> None:
                self.interactions.append(args)

        vectors = iter_callback_provenance_matrix()
        actual: list[CallbackDeliveryObservation] = []
        for recipient in ("member-a", "member-b"):
            callback = _Callback()
            proxy = _FederateAmbassadorProxy(callback, _Runtime())
            for sequence, vector in enumerate(vectors):
                with self.subTest(recipient=recipient, case=vector.case_id):
                    if vector.callback == "discoverObjectInstance":
                        proxy.discoverObjectInstance(
                            _RawHandle(vector.object_instance),
                            _RawHandle(vector.object_class),
                            vector.instance_name,
                            _RawHandle(vector.producing_federate),
                        )
                        result = callback.discoveries[-1]
                        actual.append(
                            CallbackDeliveryObservation(
                                recipient=recipient,
                                sequence=sequence,
                                vector_id=vector.case_id,
                                callback=vector.callback,
                                timed=False,
                                object_instance=bytes(result[0].encodedValue),  # type: ignore[attr-defined]
                                object_class=bytes(result[1].encodedValue),  # type: ignore[attr-defined]
                                instance_name=result[2],  # type: ignore[arg-type]
                                producing_federate=bytes(result[3].encodedValue),  # type: ignore[attr-defined]
                            )
                        )
                        continue

                    raw_map = _RawMap(
                        _RawEntry(vector.payload_handle, vector.payload)  # type: ignore[arg-type]
                    )
                    supplement = _RawSupplement(
                        vector.producing_federate, vector.sent_region  # type: ignore[arg-type]
                    )
                    common = (
                        _RawHandle(vector.object_instance)
                        if vector.callback == "reflectAttributeValues"
                        else _RawHandle(vector.interaction_class),
                        raw_map,
                        vector.tag,
                    )
                    order = _RawEnum(
                        vector.received_order
                        if not vector.timed
                        else vector.sent_order  # type: ignore[arg-type]
                    )
                    arguments = common + (
                        order,
                        _RawHandle(vector.transportation),  # type: ignore[arg-type]
                    )
                    if vector.timed:
                        arguments += (
                            _RawTime(vector.time_value, vector.time_implementation),  # type: ignore[arg-type]
                            _RawEnum(vector.received_order),  # type: ignore[arg-type]
                            _RawHandle(vector.retraction),  # type: ignore[arg-type]
                            supplement,
                        )
                    else:
                        arguments += (supplement,)
                    getattr(proxy, vector.callback)(*arguments)
                    result = (
                        callback.reflections[-1]
                        if vector.callback == "reflectAttributeValues"
                        else callback.interactions[-1]
                    )
                    payload_key = next(iter(result[1]))
                    info = result[-1]
                    actual.append(
                        CallbackDeliveryObservation(
                            recipient=recipient,
                            sequence=sequence,
                            vector_id=vector.case_id,
                            callback=vector.callback,
                            timed=vector.timed,
                            object_instance=(
                                bytes(result[0].encodedValue)
                                if vector.callback == "reflectAttributeValues"
                                else None
                            ),  # type: ignore[attr-defined]
                            interaction_class=(
                                bytes(result[0].encodedValue)
                                if vector.callback == "receiveInteraction"
                                else None
                            ),  # type: ignore[attr-defined]
                            payload_handle=bytes(payload_key.encodedValue),  # type: ignore[attr-defined]
                            payload=result[1][payload_key],
                            tag=result[2],  # type: ignore[arg-type]
                            transportation=bytes(result[4].encodedValue),  # type: ignore[attr-defined]
                            producing_federate=bytes(info.getProducingFederate().encodedValue),  # type: ignore[attr-defined]
                            sent_region=(
                                bytes(next(iter(info.getSentRegions())).encodedValue)
                                if info.hasSentRegions()
                                else None
                            ),  # type: ignore[attr-defined]
                            time_implementation=(
                                result[5].implementationName() if vector.timed else None
                            ),
                            time_value=(result[5].getTime() if vector.timed else None),
                            sent_order=(result[3].name if vector.timed else None),
                            received_order=(result[6].name if vector.timed else vector.received_order),
                            retraction=(
                                bytes(result[7].encodedValue)
                                if vector.timed and result[7] is not None
                                else None
                            ),  # type: ignore[attr-defined]
                        )
                    )

        expected = tuple(
            CallbackDeliveryObservation.from_vector(
                vector,
                recipient=recipient,
                sequence=sequence,
            )
            for recipient in ("member-a", "member-b")
            for sequence, vector in enumerate(vectors)
        )
        assert_callback_delivery_parity(expected, reversed(actual))

    def test_local_fom_paths_use_the_standard_file_url_shape(self) -> None:
        class URL:
            def __init__(self, value: str) -> None:
                self.value = value

        class JPype:
            @staticmethod
            def isJVMStarted() -> bool:
                return True

            @staticmethod
            def JClass(name: str):
                if name != "java.net.URL":
                    raise AssertionError(name)
                return URL

        runtime = JPype2010Runtime()
        runtime._jpype = JPype()
        converted = runtime.java_url(Path("RestaurantFOMmodule.xml"))
        self.assertTrue(converted.value.startswith("file:"))

    def test_opaque_region_handles_use_the_java_string_identity_fallback(self) -> None:
        runtime = JPype2010Runtime()

        class RawRegion:
            def __str__(self) -> str:
                return "region:fixture-1"

        self.assertEqual(
            runtime.handle_bytes(RawRegion()),
            b"opaque:region:fixture-1",
        )

    def test_from_jar_preserves_exact_2010_factory_configuration(self) -> None:
        runtime = _FakeRuntime()
        factory = Java2010RtiFactory.from_jar(
            "vendor-2010.jar", dependencies=("dependency.jar",), runtime=runtime
        )
        self.assertEqual(factory.rtiName(), "fake-2010")
        self.assertEqual(factory.rtiVersion(), "1.0")
        self.assertEqual(
            factory.probe().configuration.classpath,
            ("vendor-2010.jar", "dependency.jar"),
        )

    def test_ambassador_forwards_standard_callback_and_method_names(self) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        self.assertEqual(ambassador.getHLAversion(), "IEEE 1516.1-2010")
        callback = NullFederateAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED, "settings")
        ambassador.createFederationExecution("demo", ("fom.xml",))
        self.assertEqual(runtime.factory.ambassador.calls[0][0], "connect")
        self.assertEqual(runtime.factory.ambassador.calls[0][1][1], "java-HLA_EVOKED")
        self.assertEqual(
            runtime.factory.ambassador.calls[1][0], "createFederationExecution"
        )
        ambassador.disconnect()

    def test_collections_and_enum_arguments_cross_the_adapter_boundary(self) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        federates = FederateHandleSet((FederateHandle(b"a"),))
        ambassador.registerFederationSynchronizationPoint("sync", b"tag", federates)
        ambassador.changeInteractionOrderType(
            InteractionClassHandle(b"i"), OrderType.RECEIVE
        )
        calls = runtime.factory.ambassador.calls
        self.assertEqual(calls[0][0], "registerFederationSynchronizationPoint")
        self.assertEqual(calls[0][1][2][0], "set")
        self.assertEqual(calls[0][1][2][2], "getFederateHandleSetFactory")
        self.assertEqual(calls[1][0], "changeInteractionOrderType")
        self.assertEqual(calls[1][1][1], ("OrderType", "RECEIVE"))
        self.assertEqual(
            ambassador.getOrderType("receive"), ("OrderType", "java-order")
        )

    def test_same_arity_url_and_string_overloads_use_generated_parameter_types(
        self,
    ) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        ambassador.createFederationExecution("demo", ("fom.xml",))
        ambassador.createFederationExecution("demo", "fom.xml")
        calls = runtime.factory.ambassador.calls
        self.assertEqual(calls[0][1][1], ("url:fom.xml",))
        self.assertEqual(calls[1][1][1], "url:fom.xml")

    def test_standard_return_carriers_are_converted_without_leaking_java_objects(
        self,
    ) -> None:
        raw_federate = type("RawFederate", (), {"encoded": b"federate"})()
        raw_time = _FakeJavaTime(7)

        class RawQuery:
            timeIsValid = True
            time = raw_time

        class RawReturn:
            retractionHandleIsValid = True
            handle = type("RawRetraction", (), {"encoded": b"retraction"})()

        raw_return = RawReturn()

        class Implementation(_RecordingJavaAmbassador):
            def getFederateHandle(self, *args: object) -> object:
                self.calls.append(("getFederateHandle", args))
                return raw_federate

            def queryLogicalTime(self) -> object:
                self.calls.append(("queryLogicalTime", ()))
                return raw_time

            def queryGALT(self) -> object:
                self.calls.append(("queryGALT", ()))
                return RawQuery()

            def sendInteraction(self, *args: object) -> object:
                self.calls.append(("sendInteraction", args))
                return raw_return

        runtime = _FakeRuntime()
        ambassador = Java2010RTIambassador(Implementation(), runtime)
        federate = ambassador.getFederateHandle("federate")
        self.assertIsInstance(federate, FederateHandle)
        self.assertEqual(federate.encodedValue, b"federate")
        logical_time = ambassador.queryLogicalTime()
        self.assertIsInstance(logical_time, Java2010Integer64Time)
        self.assertEqual(logical_time.getTime(), 7)
        query = ambassador.queryGALT()
        self.assertTrue(query.timeIsValid)
        self.assertIsInstance(query.time, Java2010Integer64Time)
        result = ambassador.sendInteraction(
            InteractionClassHandle(b"interaction"),
            ParameterHandleValueMap(),
            b"tag",
            LogicalTime(encodedValue=b"\x00" * 8, value=0),
        )
        self.assertEqual(result[0], "MessageRetractionReturn")
        self.assertIs(result[1], raw_return)

    def test_create_federation_maps_the_standard_fom_array_and_mim_url_overload(
        self,
    ) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        ambassador.createFederationExecution(
            "demo", ("fom.xml",), Path("HLAstandardMIM.xml")
        )
        call = runtime.factory.ambassador.calls[0]
        self.assertEqual(call[0], "createFederationExecution")
        self.assertEqual(call[1][1], ("url:fom.xml",))
        self.assertEqual(call[1][2], "url:HLAstandardMIM.xml")

    def test_region_handles_use_the_live_2010_identity_cache(self) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        raw = type("RawRegion", (), {"encoded": b"region"})()
        ambassador._raw_handles[(type(RegionHandle(b"")), b"region")] = raw
        converted = ambassador._convert_argument(
            RegionHandleSet((RegionHandle(b"region"),)), "RegionHandleSet"
        )
        self.assertEqual(converted[0], "raw-set")
        self.assertIs(converted[1][0], raw)

    def test_regional_registration_returns_a_provider_owned_object_handle(self) -> None:
        raw = type("RawObject", (), {"encoded": b"object-instance"})()

        class Implementation(_RecordingJavaAmbassador):
            def registerObjectInstanceWithRegions(self, *args: object) -> object:
                self.calls.append(("registerObjectInstanceWithRegions", args))
                return raw

        runtime = _FakeRuntime()
        implementation = Implementation()
        ambassador = Java2010RTIambassador(implementation, runtime)
        result = ambassador.registerObjectInstanceWithRegions(
            ObjectClassHandle(b"object-class"), AttributeSetRegionSetPairList()
        )
        self.assertIsInstance(result, ObjectInstanceHandle)
        self.assertEqual(result.encodedValue, b"object-instance")

    def test_callback_proxy_selects_the_authoritative_overload_signature(self) -> None:
        class Callback(NullFederateAmbassador):
            received = None

            def reflectAttributeValues(self, *args: object) -> None:
                self.received = args

        class Runtime:
            @staticmethod
            def from_java_value(value, expected_type):
                return (expected_type, value)

        callback = Callback()
        proxy = _FederateAmbassadorProxy(callback, Runtime())
        proxy.reflectAttributeValues(*range(9))
        self.assertEqual(callback.received[0], ("ObjectInstanceHandle", 0))
        self.assertEqual(callback.received[5], ("LogicalTime", 5))
        self.assertEqual(callback.received[-1], ("SupplementalReflectInfo", 8))

    def test_runtime_converts_every_distinct_standard_callback_carrier(self) -> None:
        class Runtime(JPype2010Runtime):
            def handle_bytes(self, value):
                return bytes(value.encoded)

        runtime = Runtime()
        raw_handle = lambda value: type("RawHandle", (), {"encoded": value})()
        raw_enum = lambda name: type("RawEnum", (), {"name": name})()

        class RawMap:
            def __init__(self, handle, value):
                self._entry = (handle, value)

            def entrySet(self):
                return [self._entry]

        raw_time = _FakeJavaTime(4)
        raw_supplement = type(
            "RawSupplement",
            (),
            {
                "hasProducingFederate": lambda self: False,
                "hasSentRegions": lambda self: False,
            },
        )()
        raw_save_pair = type(
            "RawSavePair",
            (),
            {"handle": raw_handle(b"f"), "status": raw_enum("FEDERATE_SAVING")},
        )()
        raw_restore_pair = type(
            "RawRestorePair",
            (),
            {
                "preRestoreHandle": raw_handle(b"pre"),
                "postRestoreHandle": raw_handle(b"post"),
                "status": raw_enum("FEDERATE_RESTORING"),
            },
        )()
        raw_federation = type(
            "RawFederation",
            (),
            {
                "federationExecutionName": "demo",
                "logicalTimeImplementationName": "HLAinteger64Time",
            },
        )()
        raw_values = {
            "AttributeHandle": raw_handle(b"attribute"),
            "AttributeHandleSet": [raw_handle(b"attribute")],
            "AttributeHandleValueMap": RawMap(raw_handle(b"attribute"), b"value"),
            "FederateHandle": raw_handle(b"federate"),
            "FederateHandleSaveStatusPair[]": [raw_save_pair],
            "FederateHandleSet": [raw_handle(b"federate")],
            "FederateRestoreStatus[]": [raw_restore_pair],
            "FederationExecutionInformation": raw_federation,
            "FederationExecutionInformationSet": [raw_federation],
            "InteractionClassHandle": raw_handle(b"interaction"),
            "LogicalTime": raw_time,
            "MessageRetractionHandle": raw_handle(b"retraction"),
            "ObjectClassHandle": raw_handle(b"object-class"),
            "ObjectInstanceHandle": raw_handle(b"object-instance"),
            "OrderType": raw_enum("RECEIVE"),
            "ParameterHandleValueMap": RawMap(raw_handle(b"parameter"), b"value"),
            "RestoreFailureReason": raw_enum("RESTORE_ABORTED"),
            "SaveFailureReason": raw_enum("SAVE_ABORTED"),
            "Set<String>": ["federate"],
            "String": "callback",
            "SupplementalReceiveInfo": raw_supplement,
            "SupplementalReflectInfo": raw_supplement,
            "SupplementalRemoveInfo": raw_supplement,
            "SynchronizationPointFailureReason": raw_enum(
                "SYNCHRONIZATION_SET_MEMBER_NOT_JOINED"
            ),
            "TransportationTypeHandle": raw_handle(b"transport"),
            "byte[]": bytearray(b"tag"),
        }
        converted = {
            name: runtime.from_java_value(value, name)
            for name, value in raw_values.items()
        }
        self.assertIsInstance(converted["AttributeHandle"], AttributeHandle)
        self.assertIsInstance(converted["AttributeHandleSet"], AttributeHandleSet)
        self.assertIsInstance(
            converted["AttributeHandleValueMap"], AttributeHandleValueMap
        )
        self.assertEqual(
            next(iter(converted["AttributeHandleValueMap"].values())), b"value"
        )
        self.assertIsInstance(converted["FederateHandle"], FederateHandle)
        self.assertIsInstance(
            converted["FederateHandleSaveStatusPair[]"][0], FederateHandleSaveStatusPair
        )
        self.assertIs(
            converted["FederateHandleSaveStatusPair[]"][0].status,
            SaveStatus.FEDERATE_SAVING,
        )
        self.assertIsInstance(converted["FederateHandleSet"], FederateHandleSet)
        self.assertIsInstance(
            converted["FederateRestoreStatus[]"][0], FederateRestoreStatus
        )
        self.assertIs(
            converted["FederateRestoreStatus[]"][0].status,
            RestoreStatus.FEDERATE_RESTORING,
        )
        self.assertIsInstance(
            converted["FederationExecutionInformationSet"],
            FederationExecutionInformationSet,
        )
        self.assertEqual(
            converted["FederationExecutionInformation"],
            FederationExecutionInformation("demo", "HLAinteger64Time"),
        )
        self.assertIsInstance(
            converted["InteractionClassHandle"], InteractionClassHandle
        )
        self.assertIsInstance(converted["LogicalTime"], HLAinteger64Time)
        self.assertIsInstance(
            converted["MessageRetractionHandle"], MessageRetractionHandle
        )
        self.assertIsInstance(converted["ObjectClassHandle"], ObjectClassHandle)
        self.assertIsInstance(converted["ObjectInstanceHandle"], ObjectInstanceHandle)
        self.assertIs(converted["OrderType"], OrderType.RECEIVE)
        self.assertIsInstance(
            converted["ParameterHandleValueMap"], ParameterHandleValueMap
        )
        self.assertEqual(
            next(iter(converted["ParameterHandleValueMap"].values())), b"value"
        )
        self.assertIs(
            converted["RestoreFailureReason"], RestoreFailureReason.RESTORE_ABORTED
        )
        self.assertIs(converted["SaveFailureReason"], SaveFailureReason.SAVE_ABORTED)
        self.assertEqual(converted["Set<String>"], frozenset({"federate"}))
        self.assertEqual(converted["String"], "callback")
        self.assertIsInstance(
            converted["SupplementalReceiveInfo"], SupplementalReceiveInfo
        )
        self.assertIsInstance(
            converted["SupplementalReflectInfo"], SupplementalReflectInfo
        )
        self.assertIsInstance(
            converted["SupplementalRemoveInfo"], SupplementalRemoveInfo
        )
        self.assertIs(
            converted["SynchronizationPointFailureReason"],
            SynchronizationPointFailureReason.SYNCHRONIZATION_SET_MEMBER_NOT_JOINED,
        )
        self.assertIsInstance(
            converted["TransportationTypeHandle"], TransportationTypeHandle
        )
        self.assertEqual(converted["byte[]"], b"tag")

    def test_encoder_factory_wraps_provider_owned_2010_element(self) -> None:
        runtime = _FakeRuntime()
        encoder = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getEncoderFactory()
        value = encoder.createHLAinteger32BE(0x01020304)
        self.assertIsInstance(value, HLAinteger32BE)
        self.assertEqual(value.getValue(), 0x01020304)
        self.assertEqual(value.toByteArray(), b"\x01\x02\x03\x04")
        value.decode(b"\x04\x03\x02\x01")
        self.assertEqual(value.getValue(), 0x04030201)

    def test_2010_encoder_surface_does_not_advertise_2025_extensions(self) -> None:
        """Keep newer unsigned/variant creators out of the 2010 adapter."""

        encoder = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=_FakeRuntime()
        ).getEncoderFactory()
        current_only = (
            "createHLAunsignedInteger16BE",
            "createHLAunsignedInteger16LE",
            "createHLAunsignedInteger32BE",
            "createHLAunsignedInteger32LE",
            "createHLAunsignedInteger64BE",
            "createHLAunsignedInteger64LE",
            "createHLAextendableVariantRecord",
        )
        for method_name in current_only:
            with self.subTest(method=method_name):
                self.assertFalse(hasattr(encoder, method_name))

    def test_2010_encoder_factory_declares_every_contract_creator(self) -> None:
        """Every generated 2010 creator remains callable on the façade."""

        encoder = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=_FakeRuntime()
        ).getEncoderFactory()
        missing = sorted(
            name
            for name in EncoderFactory.__abstractmethods__
            if not callable(getattr(encoder, name, None))
        )
        self.assertEqual(missing, [])

    def test_encoder_proxy_dispatch_uses_java_interface_identity(self) -> None:
        class Runtime(_FakeRuntime):
            def is_java_instance(self, value, class_name):
                return class_name.endswith("HLAinteger32BE")

        runtime = Runtime()
        encoder = JavaEncoderFactory(_FakeJavaEncoder(), runtime)
        value = encoder.createHLAinteger32BE(9)
        self.assertEqual(value.getValue(), 9)

    def test_unknown_data_element_carrier_preserves_wire_and_cursor(self) -> None:
        """Unknown provider elements stay generic and reject truncated input."""

        runtime = _FakeRuntime()
        raw = _FakeVendorDataElement(b"abcd")
        element = _wrap_element(raw, runtime)
        self.assertNotIsInstance(element, HLAinteger32BE)
        self.assertEqual(element.getOctetBoundary(), 1)
        self.assertEqual(element.getEncodedLength(), 4)
        self.assertEqual(element.toByteArray(), b"abcd")
        self.assertEqual(element.encode(), b"abcd")

        encoded_window = ByteWrapper(b"\0" * 6, 1, 4)
        self.assertIs(element.encode(encoded_window), element)
        self.assertEqual(bytes(encoded_window.array()), b"\0abcd\0")
        self.assertEqual(encoded_window.getPos(), 5)

        for vector in iter_vendor_data_element_encode_matrix():
            with self.subTest(encode_vector=vector.case_id):
                target = _wrap_element(_FakeVendorDataElement(b"abcd"), runtime)
                wrapper = ByteWrapper(vector.backing, vector.offset, vector.length)
                before = bytes(wrapper.array())
                if vector.valid:
                    self.assertIs(target.encode(wrapper), target)
                    expected = bytearray(vector.backing)
                    expected[vector.offset : vector.offset + 4] = b"abcd"
                    self.assertEqual(bytes(wrapper.array()), bytes(expected))
                    self.assertEqual(wrapper.getPos(), vector.offset + 4)
                    self.assertEqual(wrapper.remaining(), vector.length - 4)
                else:
                    with self.assertRaises(EncoderException):
                        target.encode(wrapper)
                    self.assertEqual(bytes(wrapper.array()), before)
                    self.assertEqual(wrapper.getPos(), vector.offset)

        wrapper = ByteWrapper(b"!wxyz?", 1, 4)
        element.decode(wrapper)
        self.assertEqual(element.toByteArray(), b"wxyz")
        self.assertEqual(wrapper.getPos(), 5)

        for vector in iter_vendor_data_element_wire_matrix():
            with self.subTest(vector=vector.case_id):
                target = _wrap_element(_FakeVendorDataElement(b"abcd"), runtime)
                if vector.valid:
                    target.decode(vector.payload)
                    self.assertEqual(target.toByteArray(), vector.payload)
                else:
                    with self.assertRaises(DecoderException):
                        target.decode(vector.payload)
                    self.assertEqual(target.toByteArray(), b"abcd")

    def test_python_encode_preflights_undersized_foreign_cursor(self) -> None:
        """Reject a short destination before entering a foreign provider."""

        class TrackingImplementation:
            def __init__(self) -> None:
                self.encode_calls = 0

            def getEncodedLength(self) -> int:
                return 4

            def toByteArray(self) -> bytes:
                return b"wire"

            def encode(self, _destination: object) -> None:
                self.encode_calls += 1
                raise AssertionError("undersized encode entered the foreign provider")

        class ShortCursor:
            def remaining(self) -> int:
                return 3

        implementation = TrackingImplementation()
        element = _JavaDataElement(implementation, _FakeRuntime())
        with self.assertRaises(EncoderException):
            element.encode(ShortCursor())
        self.assertEqual(implementation.encode_calls, 0)

    def test_2010_data_element_class_name_does_not_guess_vendor_interface(self) -> None:
        """A vendor class containing a standard token remains generic."""

        class MisleadingVendorElement(_FakeVendorDataElement):
            def getClass(self):
                return _FakeClass("VendorHLAinteger32BE")

        element = _wrap_element(MisleadingVendorElement(b"abcd"), _FakeRuntime())
        self.assertNotIsInstance(element, HLAinteger32BE)
        self.assertEqual(type(element).__name__, "_JavaDataElement")
        self.assertEqual(element.toByteArray(), b"abcd")

    def test_encoder_factory_wraps_2010_record_and_collection_shapes(self) -> None:
        runtime = _FakeRuntime()
        encoder = JavaEncoderFactory(_FakeJavaEncoder(), runtime)
        first = encoder.createHLAinteger32BE(1)
        record = encoder.createHLAfixedRecord()
        record.add(first)
        self.assertEqual(record.size(), 1)
        self.assertEqual(record.get(0).getValue(), 1)
        fixed = encoder.createHLAfixedArray(
            [_FakeJavaInteger32(2), _FakeJavaInteger32(3)]
        )
        self.assertEqual([element.getValue() for element in fixed], [2, 3])
        fixed_variadic = encoder.createHLAfixedArray(
            first, encoder.createHLAinteger32BE(2)
        )
        self.assertEqual([element.getValue() for element in fixed_variadic], [1, 2])
        variable = encoder.createHLAvariableArray(object(), first)
        variable.resize(2)
        self.assertEqual(variable.size(), 2)

    def test_byte_wrapper_decode_preserves_the_provider_cursor(self) -> None:
        class RawWrapper:
            def __init__(self, value):
                self.value = value
                self.position = value.getPos()

            def advance(self, count):
                self.position += int(count)

            def getPos(self):
                return self.position

            def array(self):
                return self.value.array()

        class RawInteger(_FakeJavaInteger32):
            def decode(self, wrapper):
                wrapper.advance(4)

        class Runtime(_FakeRuntime):
            def java_byte_wrapper(self, value):
                return RawWrapper(value)

            def copy_from_java_byte_wrapper(self, raw, target):
                target.setPosition(raw.getPos())

        wrapper = ByteWrapper(b"\x00" * 8, 1, 7)
        element = JavaEncoderFactory(
            type(
                "Encoder",
                (),
                {
                    "createHLAinteger32BE": lambda self, *args: RawInteger(),
                },
            )(),
            Runtime(),
        ).createHLAinteger32BE()
        element.decode(wrapper)
        self.assertEqual(wrapper.getPos(), 5)

    def test_runtime_interval_carrier_keeps_interval_implementation_identity(
        self,
    ) -> None:
        class RawInterval(_FakeJavaInterval):
            def getClass(self):
                return _FakeClass("HLAinteger64Interval")

        class Runtime(JPype2010Runtime):
            def handle_bytes(self, value):
                return value.encoded

        converted = Runtime().from_java_value(RawInterval(3), "LogicalTimeInterval")
        self.assertEqual(converted.implementationName(), "HLAinteger64Interval")

    def test_runtime_preserves_unknown_vendor_time_carriers(self) -> None:
        """Do not guess a vendor time into the standard float façade."""

        class RawVendorCarrier:
            def __init__(self, name: str) -> None:
                self.name = name

            def getClass(self):
                return _FakeClass(self.name)

        runtime = JPype2010Runtime()
        raw_time = RawVendorCarrier("VendorLogicalTime")
        raw_interval = RawVendorCarrier("VendorLogicalTimeInterval")
        self.assertIs(runtime.from_java_value(raw_time, "LogicalTime"), raw_time)
        self.assertIs(
            runtime.from_java_value(raw_interval, "LogicalTimeInterval"), raw_interval
        )

    def test_time_factory_preserves_provider_arithmetic(self) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        factory = ambassador.getTimeFactory()
        base = factory.makeTime(5)
        interval = factory.makeInterval(2)
        self.assertEqual(base.add(interval).getTime(), 7)
        self.assertEqual(base.subtract(interval).getTime(), 3)
        self.assertEqual(base.distance(factory.makeLogicalTime(2)).getInterval(), 3)
        self.assertEqual(base.compareTo(factory.makeLogicalTime(2)), 1)
        self.assertEqual(interval.compareTo(factory.makeLogicalTimeInterval(2)), 0)

    def test_time_factory_preserves_vendor_arithmetic_failures(self) -> None:
        """Keep raw 2010 vendor-carrier exceptions on the Java object."""

        class FailingVendorTime(_FakeVendorTime):
            def add(self, interval):
                if not isinstance(interval, _FakeVendorInterval):
                    raise _FakeJavaError(
                        "IllegalTimeArithmetic", "vendor time/interval mismatch"
                    )
                raise _FakeJavaError(
                    "IllegalTimeArithmetic", "vendor arithmetic overflow"
                )

        class FailingVendorTimeFactory(_FakeVendorTimeFactory):
            def makeTime(self, value):
                return FailingVendorTime(float(value))

        class Runtime(_FakeRuntime):
            def exception_name(self, error):
                return getattr(error, "name", None)

        runtime = Runtime()
        ambassador = Java2010RTIambassador(
            _RecordingJavaAmbassador(FailingVendorTimeFactory()), runtime
        )
        factory = ambassador.getTimeFactory()
        time = factory.makeTime(1.0)
        interval = factory.makeInterval(0.25)

        # The 2010 contract deliberately returns an unknown provider carrier
        # unchanged.  Calling its provider-owned arithmetic therefore keeps
        # the raw Java exception identity instead of inventing a Python
        # standard-time mapping.
        with self.assertRaises(_FakeJavaError) as raised:
            time.add(interval)
        self.assertEqual(raised.exception.name, "IllegalTimeArithmetic")
        self.assertEqual(str(raised.exception), "vendor arithmetic overflow")

    def test_time_factory_decodes_one_fixed_width_value_at_an_offset(self) -> None:
        runtime = _FakeRuntime()
        ambassador = Java2010RtiFactory.from_jar(
            "vendor.jar", runtime=runtime
        ).getRtiAmbassador()
        factory = ambassador.getTimeFactory()
        encoded = (9).to_bytes(8, "big", signed=True)
        decoded = factory.decodeTime(b"\xaa" + encoded + b"\xbb", 1)
        self.assertEqual(decoded.getTime(), 9)
        self.assertEqual(decoded.encodedValue, encoded)
        with self.assertRaises(CouldNotDecode):
            factory.decodeTime(b"\x00" * 7)

    def test_callback_proxy_maps_2010_supplemental_info_records(self) -> None:
        class RawSupplement:
            def hasProducingFederate(self):
                return True

            def hasSentRegions(self):
                return True

            def getProducingFederate(self):
                return type("RawHandle", (), {"encoded": b"f"})()

            def getSentRegions(self):
                return [type("RawHandle", (), {"encoded": b"r"})()]

        class Callback(NullFederateAmbassador):
            received = None

            def reflectAttributeValues(self, *args: object) -> None:
                self.received = args

        class Runtime(JPype2010Runtime):
            def handle_bytes(self, value):
                return bytes(value.encoded)

            def exception_name(self, error):
                return None

            def from_java_value(self, value, expected_type):
                if expected_type == "SupplementalReflectInfo":
                    return super().from_java_value(value, expected_type)
                return (expected_type, value)

        runtime = Runtime()
        callback = Callback()
        proxy = _FederateAmbassadorProxy(callback, runtime)
        proxy.reflectAttributeValues(*range(5), RawSupplement())
        supplemental = callback.received[-1]
        self.assertTrue(supplemental.hasProducingFederate())
        self.assertEqual(supplemental.getProducingFederate().encodedValue, b"f")
        self.assertEqual(next(iter(supplemental.getSentRegions())).encodedValue, b"r")

    def test_callback_time_proxy_uses_standard_interface_identity(self) -> None:
        class RawTime:
            def getClass(self):
                return _FakeClass("$Proxy42")

            def isInitial(self):
                return False

            def isFinal(self):
                return False

            def encodedLength(self):
                return 8

            def encode(self, buffer, offset):
                return None

            def getValue(self):
                return 4

            def toString(self):
                return "4"

        class Runtime(JPype2010Runtime):
            def handle_bytes(self, value):
                return b"\x00" * 8

            def is_java_instance(self, value, class_name):
                return class_name.endswith("HLAinteger64Time")

        converted = Runtime().from_java_value(RawTime(), "LogicalTime")
        self.assertEqual(converted.implementationName(), "HLAinteger64Time")
        self.assertEqual(converted.getTime(), 4)


if __name__ == "__main__":
    unittest.main()
