from __future__ import annotations

import os
import struct
import unittest
import math

from hla.rti1516_2025 import (
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    AttributeSetRegionSetPair,
    AttributeSetRegionSetPairList,
    CallbackModel,
    DimensionHandle,
    DimensionHandleSet,
    FederateAmbassador,
    FederateHandle,
    FederateHandleSet,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    FederationExecutionMemberInformation,
    FederationExecutionMemberInformationSet,
    FederateHandleSaveStatusPair,
    FederateRestoreStatus,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAfloat64TimeFactory,
    HLAinteger64Interval,
    HLAinteger64Time,
    HLAinteger64TimeFactory,
    InteractionClassHandle,
    InteractionClassHandleSet,
    MessageRetractionHandle,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    MutableDimensionHandleSet,
    MutableFederateHandleSet,
    MutableParameterHandleValueMap,
    MutableRegionHandleSet,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ObjectInstanceNameSet,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RtiConfiguration,
    RangeBounds,
    RegionHandle,
    RegionHandleSet,
    ResignAction,
    RestoreStatus,
    SaveFailureReason,
    SaveStatus,
    ServiceGroup,
    TransportationTypeHandle,
)
from hla.rti1516_2025.auth import HLAnoCredentials
from hla.rti1516_2025.exceptions import AlreadyConnected, ConnectionFailed, RTIinternalError
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory
from umbra._java.rti1516_2025._runtime import (
    JavaCallbackBinding,
    _FederateAmbassadorCallback,
)


class _FakeJavaError(Exception):
    def __init__(self, name: str, message: str) -> None:
        super().__init__(message)
        self.name = name


class _FakeJavaEnum:
    def __init__(self, name: str) -> None:
        self._name = name

    def name(self) -> str:
        return self._name


class _FakeJavaConfigurationResult:
    configurationUsed = True
    addressUsed = False
    additionalSettingsResultCode = _FakeJavaEnum("SETTINGS_APPLIED")
    message = "connected through Java"


class _FakeCallbackProxy:
    def __init__(self, target: FederateAmbassador) -> None:
        self._target = target

    def connectionLost(self, description: str) -> None:
        self._target.connectionLost(description)

    def reportFederationExecutions(self, report: object) -> None:
        self._target.reportFederationExecutions(
            FederationExecutionInformationSet(
                FederationExecutionInformation(
                    information.federationExecutionName,
                    information.logicalTimeImplementationName,
                )
                for information in report  # type: ignore[union-attr]
            )
        )

    def reportFederationExecutionMembers(self, federation_name: str, report: object) -> None:
        self._target.reportFederationExecutionMembers(
            federation_name,
            FederationExecutionMemberInformationSet(
                FederationExecutionMemberInformation(information.federateName, information.federateType)
                for information in report  # type: ignore[union-attr]
            ),
        )

    def reportFederationExecutionDoesNotExist(self, federation_name: str) -> None:
        self._target.reportFederationExecutionDoesNotExist(federation_name)

    def federateResigned(self, reason: str) -> None:
        self._target.federateResigned(reason)

    def synchronizationPointRegistrationSucceeded(self, label: str) -> None:
        self._target.synchronizationPointRegistrationSucceeded(label)

    def announceSynchronizationPoint(self, label: str, tag: bytes) -> None:
        self._target.announceSynchronizationPoint(label, tag)

    def federationSynchronized(self, label: str, failed_to_sync: object) -> None:
        self._target.federationSynchronized(label, FederateHandleSet(failed_to_sync))

    def provideAttributeValueUpdate(
        self, object_instance: bytes, attributes: object, user_supplied_tag: bytes
    ) -> None:
        self._target.provideAttributeValueUpdate(
            ObjectInstanceHandle(object_instance),
            AttributeHandleSet(AttributeHandle(attribute) for attribute in attributes),
            bytes(user_supplied_tag),
        )

    def attributesInScope(self, object_instance: bytes, attributes: object) -> None:
        self._target.attributesInScope(
            ObjectInstanceHandle(object_instance),
            AttributeHandleSet(AttributeHandle(attribute) for attribute in attributes),
        )

    def attributesOutOfScope(self, object_instance: bytes, attributes: object) -> None:
        self._target.attributesOutOfScope(
            ObjectInstanceHandle(object_instance),
            AttributeHandleSet(AttributeHandle(attribute) for attribute in attributes),
        )

    def turnUpdatesOnForObjectInstance(
        self, object_instance: bytes, attributes: object, update_rate_designator: str | None = None
    ) -> None:
        self._target.turnUpdatesOnForObjectInstance(
            ObjectInstanceHandle(object_instance),
            AttributeHandleSet(AttributeHandle(attribute) for attribute in attributes),
            update_rate_designator,
        )

    def turnUpdatesOffForObjectInstance(self, object_instance: bytes, attributes: object) -> None:
        self._target.turnUpdatesOffForObjectInstance(
            ObjectInstanceHandle(object_instance),
            AttributeHandleSet(AttributeHandle(attribute) for attribute in attributes),
        )

    def confirmAttributeTransportationTypeChange(
        self, object_instance: bytes, attributes: object, transportation_type: bytes
    ) -> None:
        self._target.confirmAttributeTransportationTypeChange(
            ObjectInstanceHandle(object_instance),
            AttributeHandleSet(AttributeHandle(attribute) for attribute in attributes),
            TransportationTypeHandle(transportation_type),
        )

    def reportAttributeTransportationType(
        self, object_instance: bytes, attribute: bytes, transportation_type: bytes
    ) -> None:
        self._target.reportAttributeTransportationType(
            ObjectInstanceHandle(object_instance),
            AttributeHandle(attribute),
            TransportationTypeHandle(transportation_type),
        )

    def confirmInteractionTransportationTypeChange(
        self, interaction_class: bytes, transportation_type: bytes
    ) -> None:
        self._target.confirmInteractionTransportationTypeChange(
            InteractionClassHandle(interaction_class),
            TransportationTypeHandle(transportation_type),
        )

    def reportInteractionTransportationType(
        self, federate: bytes, interaction_class: bytes, transportation_type: bytes
    ) -> None:
        self._target.reportInteractionTransportationType(
            FederateHandle(federate),
            InteractionClassHandle(interaction_class),
            TransportationTypeHandle(transportation_type),
        )

    def timeRegulationEnabled(self, time: object) -> None:
        self._target.timeRegulationEnabled(time)

    def timeConstrainedEnabled(self, time: object) -> None:
        self._target.timeConstrainedEnabled(time)

    def flushQueueGrant(self, time: object, optimistic_time: object) -> None:
        self._target.flushQueueGrant(time, optimistic_time)

    def timeAdvanceGrant(self, time: object) -> None:
        self._target.timeAdvanceGrant(time)

    def requestRetraction(self, retraction: bytes) -> None:
        self._target.requestRetraction(MessageRetractionHandle(bytes(retraction)))

    def receiveDirectedInteraction(
        self,
        interaction_class: bytes,
        object_instance: bytes,
        parameter_values: object,
        user_supplied_tag: bytes,
        transportation_type: bytes,
        producing_federate: bytes,
        *timed: object,
    ) -> None:
        arguments = (
            InteractionClassHandle(interaction_class),
            ObjectInstanceHandle(object_instance),
            ParameterHandleValueMap(
                (ParameterHandle(handle), bytes(value))
                for handle, value in dict(parameter_values).items()
            ),
            bytes(user_supplied_tag),
            TransportationTypeHandle(transportation_type),
            FederateHandle(producing_federate),
        )
        if timed:
            self._target.receiveDirectedInteraction(*arguments, *timed)
        else:
            self._target.receiveDirectedInteraction(*arguments)

    def multipleObjectInstanceNameReservationSucceeded(self, object_instance_names: object) -> None:
        self._target.multipleObjectInstanceNameReservationSucceeded(
            ObjectInstanceNameSet(str(name) for name in object_instance_names)
        )

    def multipleObjectInstanceNameReservationFailed(self, object_instance_names: object) -> None:
        self._target.multipleObjectInstanceNameReservationFailed(
            ObjectInstanceNameSet(str(name) for name in object_instance_names)
        )


class _FakeJavaFederationExecutionInformation:
    federationExecutionName = "Fake Federation"
    logicalTimeImplementationName = "HLAinteger64Time"


class _FakeJavaTime:
    def __init__(self, value: int, *, initial: bool = False, final: bool = False) -> None:
        self.value = value
        self.initial = initial
        self.final = final

    def encodedLength(self) -> int:
        return 8

    def encode(self, destination: bytearray, offset: int = 0) -> None:
        destination[offset : offset + 8] = struct.pack(">q", self.value)

    def isInitial(self) -> bool:
        return self.initial

    def isFinal(self) -> bool:
        return self.final

    def add(self, addend: object) -> "_FakeJavaTime":
        if not isinstance(addend, _FakeJavaInterval):
            raise ValueError("logical-time implementation mismatch")
        return _FakeJavaTime(self.value + addend.value)

    def subtract(self, subtrahend: object) -> "_FakeJavaTime":
        if not isinstance(subtrahend, _FakeJavaInterval):
            raise ValueError("logical-time implementation mismatch")
        if subtrahend.value > self.value:
            raise ValueError("logical-time subtraction precedes the initial value")
        return _FakeJavaTime(self.value - subtrahend.value)

    def implementationName(self) -> str:
        return "HLAinteger64Time"

    def getTime(self) -> int:
        return self.value

    def toString(self) -> str:
        return str(self.value)


class _FakeJavaInterval:
    def __init__(self, value: int, *, zero: bool = False, epsilon: bool = False) -> None:
        self.value = value
        self.zero = zero
        self.epsilon = epsilon

    def encodedLength(self) -> int:
        return 8

    def encode(self, destination: bytearray, offset: int = 0) -> None:
        destination[offset : offset + 8] = struct.pack(">q", self.value)

    def isZero(self) -> bool:
        return self.zero

    def isEpsilon(self) -> bool:
        return self.epsilon

    def add(self, addend: object) -> "_FakeJavaInterval":
        if not isinstance(addend, _FakeJavaInterval):
            raise ValueError("logical-time implementation mismatch")
        return _FakeJavaInterval(self.value + addend.value)

    def subtract(self, subtrahend: object) -> "_FakeJavaInterval":
        if not isinstance(subtrahend, _FakeJavaInterval):
            raise ValueError("logical-time implementation mismatch")
        if subtrahend.value > self.value:
            raise ValueError("logical-time interval subtraction precedes zero")
        return _FakeJavaInterval(self.value - subtrahend.value)

    def setToDifference(self, minuend: object, subtrahend: object) -> None:
        if not isinstance(minuend, _FakeJavaTime) or not isinstance(subtrahend, _FakeJavaTime):
            raise ValueError("logical-time implementation mismatch")
        if subtrahend.value > minuend.value:
            raise ValueError("logical-time difference would be negative")
        self.value = minuend.value - subtrahend.value
        self.zero = self.value == 0
        self.epsilon = self.value == 1

    def implementationName(self) -> str:
        return "HLAinteger64Time"

    def getInterval(self) -> int:
        return self.value

    def toString(self) -> str:
        return str(self.value)


class _FakeJavaTimeQueryReturn:
    def __init__(self, valid: bool, time: _FakeJavaTime | None) -> None:
        self.timeIsValid = valid
        self.time = time


class _FakeJavaTimeFactory:
    def getName(self) -> str:
        return "HLAinteger64Time"

    def makeInitial(self) -> _FakeJavaTime:
        return _FakeJavaTime(0, initial=True)

    def makeFinal(self) -> _FakeJavaTime:
        return _FakeJavaTime(2**63 - 1, final=True)

    def makeZero(self) -> _FakeJavaInterval:
        return _FakeJavaInterval(0, zero=True)

    def makeEpsilon(self) -> _FakeJavaInterval:
        return _FakeJavaInterval(1, epsilon=True)

    def makeLogicalTime(self, value: int) -> _FakeJavaTime:
        return _FakeJavaTime(value)

    def makeLogicalTimeInterval(self, value: int) -> _FakeJavaInterval:
        return _FakeJavaInterval(value)

    def decodeLogicalTime(self, encoded: bytes, offset: int) -> _FakeJavaTime:
        return _FakeJavaTime(struct.unpack(">q", bytes(encoded)[offset : offset + 8])[0])

    def decodeLogicalTimeInterval(self, encoded: bytes, offset: int) -> _FakeJavaInterval:
        return _FakeJavaInterval(struct.unpack(">q", bytes(encoded)[offset : offset + 8])[0])


class _FakeJavaFloatTime:
    def __init__(self, value: float, *, initial: bool = False, final: bool = False) -> None:
        self.value = 0.0 if float(value) == 0.0 else float(value)
        self.initial = initial or self.value == 0.0
        self.final = final or self.value == float("1.7976931348623157e+308")

    def encodedLength(self) -> int:
        return 8

    def encode(self, destination: bytearray, offset: int = 0) -> None:
        destination[offset : offset + 8] = struct.pack(">d", self.value)

    def isInitial(self) -> bool:
        return self.initial

    def isFinal(self) -> bool:
        return self.final

    def add(self, addend: object) -> "_FakeJavaFloatTime":
        if not isinstance(addend, _FakeJavaFloatInterval):
            raise ValueError("logical-time implementation mismatch")
        value = self.value
        interval = addend.value
        result = value + interval
        if interval == math.nextafter(0.0, 1.0) and result == value:
            result = math.nextafter(value, float("1.7976931348623157e+308"))
        return _FakeJavaFloatTime(result)

    def subtract(self, subtrahend: object) -> "_FakeJavaFloatTime":
        if not isinstance(subtrahend, _FakeJavaFloatInterval):
            raise ValueError("logical-time implementation mismatch")
        if subtrahend.value > self.value:
            raise ValueError("logical-time subtraction precedes the initial value")
        result = self.value - subtrahend.value
        if subtrahend.value == math.nextafter(0.0, 1.0) and result == self.value and self.value > 0.0:
            result = math.nextafter(self.value, 0.0)
        return _FakeJavaFloatTime(result)

    def implementationName(self) -> str:
        return "HLAfloat64Time"

    def getTime(self) -> float:
        return self.value

    def toString(self) -> str:
        return str(self.value)


class _FakeJavaFloatInterval:
    def __init__(self, value: float, *, zero: bool = False, epsilon: bool = False) -> None:
        self.value = 0.0 if float(value) == 0.0 else float(value)
        self.zero = zero or self.value == 0.0
        self.epsilon = epsilon or self.value == math.nextafter(0.0, 1.0)

    def encodedLength(self) -> int:
        return 8

    def encode(self, destination: bytearray, offset: int = 0) -> None:
        destination[offset : offset + 8] = struct.pack(">d", self.value)

    def isZero(self) -> bool:
        return self.zero

    def isEpsilon(self) -> bool:
        return self.epsilon

    def add(self, addend: object) -> "_FakeJavaFloatInterval":
        if not isinstance(addend, _FakeJavaFloatInterval):
            raise ValueError("logical-time implementation mismatch")
        result = self.value + addend.value
        if addend.value == math.nextafter(0.0, 1.0) and result == self.value:
            result = math.nextafter(self.value, float("1.7976931348623157e+308"))
        return _FakeJavaFloatInterval(result)

    def subtract(self, subtrahend: object) -> "_FakeJavaFloatInterval":
        if not isinstance(subtrahend, _FakeJavaFloatInterval):
            raise ValueError("logical-time implementation mismatch")
        if subtrahend.value > self.value:
            raise ValueError("logical-time interval subtraction precedes zero")
        result = self.value - subtrahend.value
        if subtrahend.value == math.nextafter(0.0, 1.0) and result == self.value and self.value > 0.0:
            result = math.nextafter(self.value, 0.0)
        return _FakeJavaFloatInterval(result)

    def setToDifference(self, minuend: object, subtrahend: object) -> None:
        if not isinstance(minuend, _FakeJavaFloatTime) or not isinstance(subtrahend, _FakeJavaFloatTime):
            raise ValueError("logical-time implementation mismatch")
        if subtrahend.value > minuend.value:
            raise ValueError("logical-time difference would be negative")
        self.value = minuend.value - subtrahend.value
        self.zero = self.value == 0.0
        self.epsilon = self.value == math.nextafter(0.0, 1.0)

    def implementationName(self) -> str:
        return "HLAfloat64Time"

    def getInterval(self) -> float:
        return self.value

    def toString(self) -> str:
        return str(self.value)


class _FakeJavaFloatTimeFactory:
    def getName(self) -> str:
        return "HLAfloat64Time"

    def makeInitial(self) -> _FakeJavaFloatTime:
        return _FakeJavaFloatTime(0.0, initial=True)

    def makeFinal(self) -> _FakeJavaFloatTime:
        return _FakeJavaFloatTime(float("1.7976931348623157e+308"), final=True)

    def makeZero(self) -> _FakeJavaFloatInterval:
        return _FakeJavaFloatInterval(0.0, zero=True)

    def makeEpsilon(self) -> _FakeJavaFloatInterval:
        return _FakeJavaFloatInterval(math.nextafter(0.0, 1.0), epsilon=True)

    def makeLogicalTime(self, value: float) -> _FakeJavaFloatTime:
        return _FakeJavaFloatTime(value)

    def makeLogicalTimeInterval(self, value: float) -> _FakeJavaFloatInterval:
        return _FakeJavaFloatInterval(value)

    def decodeLogicalTime(self, encoded: bytes, offset: int) -> _FakeJavaFloatTime:
        return _FakeJavaFloatTime(struct.unpack(">d", bytes(encoded)[offset : offset + 8])[0])

    def decodeLogicalTimeInterval(self, encoded: bytes, offset: int) -> _FakeJavaFloatInterval:
        return _FakeJavaFloatInterval(struct.unpack(">d", bytes(encoded)[offset : offset + 8])[0])


class _FakeJavaRangeBounds:
    def __init__(self, lower: int, upper: int) -> None:
        self.lower = lower
        self.upper = upper

    def getLowerBound(self) -> int:
        return self.lower

    def getUpperBound(self) -> int:
        return self.upper


class _FakeJavaAmbassador:
    def __init__(self) -> None:
        self.connected = False
        self.callback_proxy: _FakeCallbackProxy | None = None
        self.callback_model: object | None = None
        self.connect_arguments: tuple[object, ...] = ()
        self.connect_error_name: str | None = None
        self.callbacks_enabled = True
        self.federation_executions: dict[str, str] = {"Fake Federation": "HLAinteger64Time"}
        self.object_instances: dict[str, bytes] = {}
        self._next_object_instance = 1
        self.reserved_object_instance_names: set[str] = set()
        self.time_factory = _FakeJavaTimeFactory()
        self.current_time = self.time_factory.makeInitial()
        self.current_lookahead = self.time_factory.makeZero()
        self.time_regulating = False
        self.time_constrained = False
        self.asynchronous_delivery = False
        self.regions: dict[bytes, set[bytes]] = {}
        self.region_bounds: dict[tuple[bytes, bytes], _FakeJavaRangeBounds] = {}
        self._next_region = 1
        self.convey_region_designator_sets = False
        self.object_class_relevance_advisory = False
        self.attribute_relevance_advisory = False
        self.attribute_scope_advisory = False
        self.interaction_relevance_advisory = False
        self.automatic_resign_directive = "NO_ACTION"
        self.service_reporting = False
        self.exception_reporting = False
        self.last_attribute_value_update_request: tuple[bytes, frozenset[bytes], bytes] | None = None
        self.save_status_queried = False
        self.last_save_label: str | None = None
        self.last_save_time: object | None = None
        self.save_begun = False
        self.save_completed = False
        self.save_not_completed = False
        self.save_aborted = False
        self.restore_status_queried = False
        self.last_restore_label: str | None = None
        self.restore_completed = False
        self.restore_not_completed = False
        self.restore_aborted = False

    def connect(self, callback_proxy: _FakeCallbackProxy, callback_model: object, *arguments: object) -> _FakeJavaConfigurationResult:
        if self.connect_error_name is not None:
            raise _FakeJavaError(self.connect_error_name, "Java provider error")
        if self.connected:
            raise _FakeJavaError("AlreadyConnected", "already connected")
        self.connected = True
        self.callback_proxy = callback_proxy
        self.callback_model = callback_model
        self.connect_arguments = arguments
        return _FakeJavaConfigurationResult()

    def disconnect(self) -> None:
        self.connected = False

    def evokeCallback(self, seconds: float) -> bool:
        return seconds >= 0.0

    def evokeMultipleCallbacks(self, minimum: float, maximum: float) -> bool:
        return minimum <= maximum

    def enableCallbacks(self) -> None:
        self.callbacks_enabled = True

    def disableCallbacks(self) -> None:
        self.callbacks_enabled = False

    def listFederationExecutions(self) -> None:
        if not self.connected:
            raise _FakeJavaError("NotConnected", "not connected")
        self.callback_proxy.reportFederationExecutions(
            [
                type(
                    "Information",
                    (),
                    {
                        "federationExecutionName": name,
                        "logicalTimeImplementationName": time_name,
                    },
                )()
                for name, time_name in self.federation_executions.items()
            ]
        )  # type: ignore[union-attr]

    def listFederationExecutionMembers(self, federation_name: str) -> None:
        if not self.connected:
            raise _FakeJavaError("NotConnected", "not connected")
        if federation_name not in self.federation_executions:
            self.callback_proxy.reportFederationExecutionDoesNotExist(federation_name)  # type: ignore[union-attr]
            return
        self.callback_proxy.reportFederationExecutionMembers(federation_name, [])  # type: ignore[union-attr]

    def createFederationExecution(self, *arguments: object) -> None:
        if len(arguments) == 3:
            name, modules, time_name = arguments
            self.last_fom_modules = tuple(modules)  # type: ignore[arg-type]
            self.last_mim_module = None
        else:
            name, modules, mim_module, time_name = arguments
            self.last_fom_modules = tuple(modules)  # type: ignore[arg-type]
            self.last_mim_module = mim_module
        self.federation_executions[str(name)] = str(time_name)

    def destroyFederationExecution(self, name: str) -> None:
        self.federation_executions.pop(name, None)

    def joinFederationExecution(self, *arguments: object) -> bytes:
        if len(arguments) in (2, 3) and (len(arguments) == 2 or not isinstance(arguments[2], str)):
            federate_type, federation_name = arguments[:2]
            federate_name = federate_type
        else:
            federate_name, federate_type, federation_name = arguments[:3]
        self.federation_executions[str(federation_name)] = "HLAinteger64Time"
        return f"{federation_name}:{federate_name}:{federate_type}".encode()

    def resignFederationExecution(self, resign_action: object) -> None:
        self.last_resign_action = resign_action

    def registerFederationSynchronizationPoint(
        self, label: str, tag: bytes, synchronization_set: object = None
    ) -> None:
        self.last_synchronization_set = synchronization_set
        self.callback_proxy.synchronizationPointRegistrationSucceeded(label)  # type: ignore[union-attr]
        self.callback_proxy.announceSynchronizationPoint(label, bytes(tag))  # type: ignore[union-attr]

    def emitAdditionalCallbacksForTest(self) -> None:
        self.callback_proxy.federateResigned("fake RTI removed this federate")  # type: ignore[union-attr]
        self.callback_proxy.flushQueueGrant(self.current_time, self.current_time)  # type: ignore[union-attr]
        self.callback_proxy.requestRetraction(b"fake-retraction")  # type: ignore[union-attr]

    def synchronizationPointAchieved(self, label: str, successfully: bool) -> None:
        self.callback_proxy.federationSynchronized(label, [])  # type: ignore[union-attr]

    def queryFederationSaveStatus(self) -> None:
        self.save_status_queried = True

    def requestFederationSave(self, label: str, time: object | None = None) -> None:
        self.last_save_label = label
        self.last_save_time = time

    def federateSaveBegun(self) -> None:
        self.save_begun = True

    def federateSaveComplete(self) -> None:
        self.save_completed = True

    def federateSaveNotComplete(self) -> None:
        self.save_not_completed = True

    def abortFederationSave(self) -> None:
        self.save_aborted = True

    def queryFederationRestoreStatus(self) -> None:
        self.restore_status_queried = True

    def requestFederationRestore(self, label: str) -> None:
        self.last_restore_label = label

    def federateRestoreComplete(self) -> None:
        self.restore_completed = True

    def federateRestoreNotComplete(self) -> None:
        self.restore_not_completed = True

    def abortFederationRestore(self) -> None:
        self.restore_aborted = True

    def getTimeFactory(self) -> _FakeJavaTimeFactory:
        return self.time_factory

    def enableTimeRegulation(self, lookahead: _FakeJavaInterval) -> None:
        self.last_lookahead = lookahead
        self.current_lookahead = lookahead
        self.time_regulating = True
        self.callback_proxy.timeRegulationEnabled(self.current_time)  # type: ignore[union-attr]

    def disableTimeRegulation(self) -> None:
        self.time_regulating = False

    def enableTimeConstrained(self) -> None:
        self.time_constrained = True
        self.callback_proxy.timeConstrainedEnabled(self.current_time)  # type: ignore[union-attr]

    def disableTimeConstrained(self) -> None:
        self.time_constrained = False

    def enableAsynchronousDelivery(self) -> None:
        self.asynchronous_delivery = True

    def disableAsynchronousDelivery(self) -> None:
        self.asynchronous_delivery = False

    def modifyLookahead(self, lookahead: _FakeJavaInterval) -> None:
        self.current_lookahead = lookahead

    def queryLookahead(self) -> _FakeJavaInterval:
        return self.current_lookahead

    def timeAdvanceRequest(self, time: _FakeJavaTime) -> None:
        self.last_time_request = time
        self.current_time = time
        self.callback_proxy.timeAdvanceGrant(self.current_time)  # type: ignore[union-attr]

    def timeAdvanceRequestAvailable(self, time: _FakeJavaTime) -> None:
        self.timeAdvanceRequest(time)

    def nextMessageRequest(self, time: _FakeJavaTime) -> None:
        self.timeAdvanceRequest(time)

    def nextMessageRequestAvailable(self, time: _FakeJavaTime) -> None:
        self.timeAdvanceRequest(time)

    def flushQueueRequest(self, time: _FakeJavaTime) -> None:
        self.timeAdvanceRequest(time)

    def queryLogicalTime(self) -> _FakeJavaTime:
        return self.current_time

    def queryGALT(self) -> _FakeJavaTimeQueryReturn:
        return _FakeJavaTimeQueryReturn(True, self.current_time)

    def queryLITS(self) -> _FakeJavaTimeQueryReturn:
        return _FakeJavaTimeQueryReturn(True, self.current_time)

    def createRegion(self, dimensions: set[bytes]) -> bytes:
        region = f"region:{self._next_region}".encode()
        self._next_region += 1
        self.regions[region] = set(dimensions)
        return region

    def commitRegionModifications(self, regions: set[bytes]) -> None:
        for region in regions:
            if region not in self.regions:
                raise _FakeJavaError("InvalidRegion", "unknown region")

    def deleteRegion(self, region: bytes) -> None:
        self.regions.pop(region)

    def getDimensionHandleSet(self, region: bytes) -> set[bytes]:
        return set(self.regions[region])

    def getRangeBounds(self, region: bytes, dimension: bytes) -> _FakeJavaRangeBounds:
        return self.region_bounds.get((region, dimension), _FakeJavaRangeBounds(0, 0))

    def setRangeBounds(
        self, region: bytes, dimension: bytes, bounds: _FakeJavaRangeBounds
    ) -> None:
        self.region_bounds[(region, dimension)] = _FakeJavaRangeBounds(
            bounds.getLowerBound(), bounds.getUpperBound()
        )

    def getConveyRegionDesignatorSetsSwitch(self) -> bool:
        return self.convey_region_designator_sets

    def setConveyRegionDesignatorSetsSwitch(self, switch_value: bool) -> None:
        self.convey_region_designator_sets = bool(switch_value)

    def getObjectClassRelevanceAdvisorySwitch(self) -> bool:
        return self.object_class_relevance_advisory

    def setObjectClassRelevanceAdvisorySwitch(self, switch_value: bool) -> None:
        self.object_class_relevance_advisory = bool(switch_value)

    def getAttributeRelevanceAdvisorySwitch(self) -> bool:
        return self.attribute_relevance_advisory

    def setAttributeRelevanceAdvisorySwitch(self, switch_value: bool) -> None:
        self.attribute_relevance_advisory = bool(switch_value)

    def getAttributeScopeAdvisorySwitch(self) -> bool:
        return self.attribute_scope_advisory

    def setAttributeScopeAdvisorySwitch(self, switch_value: bool) -> None:
        self.attribute_scope_advisory = bool(switch_value)

    def getInteractionRelevanceAdvisorySwitch(self) -> bool:
        return self.interaction_relevance_advisory

    def setInteractionRelevanceAdvisorySwitch(self, switch_value: bool) -> None:
        self.interaction_relevance_advisory = bool(switch_value)

    def getAutomaticResignDirective(self) -> str:
        return self.automatic_resign_directive

    def setAutomaticResignDirective(self, resign_action: object) -> None:
        value = getattr(resign_action, "name", str(resign_action))
        self.automatic_resign_directive = str(value).split(":")[-1]

    def getServiceReportingSwitch(self) -> bool:
        return self.service_reporting

    def setServiceReportingSwitch(self, switch_value: bool) -> None:
        self.service_reporting = bool(switch_value)

    def getExceptionReportingSwitch(self) -> bool:
        return self.exception_reporting

    def setExceptionReportingSwitch(self, switch_value: bool) -> None:
        self.exception_reporting = bool(switch_value)

    def getSendServiceReportsToFileSwitch(self) -> bool:
        return False

    def getAutoProvideSwitch(self) -> bool:
        return False

    def getDelaySubscriptionEvaluationSwitch(self) -> bool:
        return False

    def getAdvisoriesUseKnownClassSwitch(self) -> bool:
        return False

    def getAllowRelaxedDDMSwitch(self) -> bool:
        return False

    def getNonRegulatedGrantSwitch(self) -> bool:
        return False

    @staticmethod
    def _handle(kind: str, name: str) -> bytes:
        return f"{kind}:{name}".encode()

    @staticmethod
    def _handle_name(handle: bytes, kind: str) -> str:
        prefix = f"{kind}:".encode()
        if not handle.startswith(prefix):
            raise _FakeJavaError("CouldNotDecode", f"not a {kind} handle")
        return handle[len(prefix) :].decode()

    def getObjectClassHandle(self, name: str) -> bytes:
        return self._handle("object", name)

    def getObjectClassName(self, handle: bytes) -> str:
        return self._handle_name(handle, "object")

    def getAttributeHandle(self, object_class: bytes, name: str) -> bytes:
        return self._handle("attribute", f"{self.getObjectClassName(object_class)}:{name}")

    def getAttributeName(self, object_class: bytes, attribute: bytes) -> str:
        prefix = f"{self.getObjectClassName(object_class)}:".encode()
        return self._handle_name(attribute, "attribute").removeprefix(prefix.decode())

    def getInteractionClassHandle(self, name: str) -> bytes:
        return self._handle("interaction", name)

    def getInteractionClassName(self, handle: bytes) -> str:
        return self._handle_name(handle, "interaction")

    def getParameterHandle(self, interaction_class: bytes, name: str) -> bytes:
        return self._handle("parameter", f"{self.getInteractionClassName(interaction_class)}:{name}")

    def getParameterName(self, interaction_class: bytes, parameter: bytes) -> str:
        prefix = f"{self.getInteractionClassName(interaction_class)}:".encode()
        return self._handle_name(parameter, "parameter").removeprefix(prefix.decode())

    def getTransportationTypeHandle(self, name: str) -> bytes:
        return self._handle("transport", name)

    def getTransportationTypeName(self, handle: bytes) -> str:
        return self._handle_name(handle, "transport")

    def getDimensionHandle(self, name: str) -> bytes:
        return self._handle("dimension", name)

    def getDimensionName(self, handle: bytes) -> str:
        return self._handle_name(handle, "dimension")

    def getFederateHandle(self, name: str) -> bytes:
        return self._handle("federate", name)

    def getFederateName(self, handle: bytes) -> str:
        return self._handle_name(handle, "federate")

    def getKnownObjectClassHandle(self, object_instance: bytes) -> bytes:
        self._handle_name(object_instance, "instance")
        return self._handle("object", "HLAobjectRoot.Employee.Server")

    def getUpdateRateValue(self, designator: str) -> float:
        if not designator:
            raise _FakeJavaError("InvalidUpdateRateDesignator", "empty designator")
        return 1.0

    def getUpdateRateValueForAttribute(self, object_instance: bytes, attribute: bytes) -> float:
        self._handle_name(object_instance, "instance")
        self._handle_name(attribute, "attribute")
        return 1.0

    def getOrderType(self, name: str) -> str:
        if name in ("Receive", "RECEIVE"):
            return "RECEIVE"
        if name in ("TimeStamp", "TIMESTAMP"):
            return "TIMESTAMP"
        raise _FakeJavaError("InvalidOrderName", "unknown order")

    def getOrderName(self, order_type: object) -> str:
        name = str(order_type).split(":")[-1]
        if name == "RECEIVE":
            return "Receive"
        if name == "TIMESTAMP":
            return "TimeStamp"
        raise _FakeJavaError("InvalidOrderType", "unknown order")

    def getAvailableDimensionsForObjectClass(self, object_class: bytes) -> set[bytes]:
        self._handle_name(object_class, "object")
        return {self._handle("dimension", "SodaFlavor")}

    def getAvailableDimensionsForInteractionClass(self, interaction_class: bytes) -> set[bytes]:
        self._handle_name(interaction_class, "interaction")
        return {self._handle("dimension", "SodaFlavor")}

    def getDimensionUpperBound(self, dimension: bytes) -> int:
        self._handle_name(dimension, "dimension")
        return 100

    def normalizeServiceGroup(self, service_group: object) -> int:
        name = str(service_group).split(":")[-1]
        return {
            "FEDERATION_MANAGEMENT": 0,
            "DECLARATION_MANAGEMENT": 1,
            "OBJECT_MANAGEMENT": 2,
            "OWNERSHIP_MANAGEMENT": 3,
            "TIME_MANAGEMENT": 4,
            "DATA_DISTRIBUTION_MANAGEMENT": 5,
            "SUPPORT_SERVICES": 6,
        }[name]

    def normalizeFederateHandle(self, federate: bytes) -> int:
        self._handle_name(federate, "federate")
        return 1

    def normalizeObjectClassHandle(self, object_class: bytes) -> int:
        self._handle_name(object_class, "object")
        return 2

    def normalizeInteractionClassHandle(self, interaction_class: bytes) -> int:
        self._handle_name(interaction_class, "interaction")
        return 3

    def normalizeObjectInstanceHandle(self, object_instance: bytes) -> int:
        self._handle_name(object_instance, "instance")
        return 4

    def getFederateHandleFactory(self) -> object:
        return self

    def getObjectClassHandleFactory(self) -> object:
        return self

    def getObjectInstanceHandleFactory(self) -> object:
        return self

    def getAttributeHandleFactory(self) -> object:
        return self

    def getInteractionClassHandleFactory(self) -> object:
        return self

    def getParameterHandleFactory(self) -> object:
        return self

    def getTransportationTypeHandleFactory(self) -> object:
        return self

    def getDimensionHandleFactory(self) -> object:
        return self

    def getRegionHandleFactory(self) -> object:
        return self

    def getFederateHandleSetFactory(self) -> object:
        return self

    def getDimensionHandleSetFactory(self) -> object:
        return self

    def getRegionHandleSetFactory(self) -> object:
        return self

    def getAttributeHandleSetFactory(self) -> object:
        return self

    def getAttributeHandleValueMapFactory(self) -> object:
        return self

    def getParameterHandleValueMapFactory(self) -> object:
        return self

    def create(self) -> object:
        return set()

    def decode(self, buffer: bytes, offset: int) -> bytes:
        return bytes(buffer[offset:])

    def publishObjectClassAttributes(self, object_class: bytes, attributes: object) -> None:
        self.last_declaration_call = ("publish_object", object_class, frozenset(attributes))

    def unpublishObjectClass(self, object_class: bytes) -> None:
        self.last_declaration_call = ("unpublish_object", object_class)

    def unpublishObjectClassAttributes(self, object_class: bytes, attributes: object) -> None:
        self.last_declaration_call = ("unpublish_object_attributes", object_class, frozenset(attributes))

    def publishObjectClassDirectedInteractions(self, object_class: bytes, interactions: object) -> None:
        self.last_declaration_call = ("publish_object_directed", object_class, frozenset(interactions))

    def unpublishObjectClassDirectedInteractions(self, object_class: bytes, interactions: object = None) -> None:
        self.last_declaration_call = (
            "unpublish_object_directed",
            object_class,
            None if interactions is None else frozenset(interactions),
        )

    def subscribeObjectClassAttributes(
        self, object_class: bytes, attributes: object, active: bool, update_rate: str
    ) -> None:
        self.last_declaration_call = (
            "subscribe_object",
            object_class,
            frozenset(attributes),
            active,
            update_rate,
        )

    def subscribeObjectClassDirectedInteractions(
        self, object_class: bytes, interactions: object, universally: bool
    ) -> None:
        self.last_declaration_call = (
            "subscribe_object_directed",
            object_class,
            frozenset(interactions),
            universally,
        )

    def unsubscribeObjectClass(self, object_class: bytes) -> None:
        self.last_declaration_call = ("unsubscribe_object", object_class)

    def unsubscribeObjectClassAttributes(self, object_class: bytes, attributes: object) -> None:
        self.last_declaration_call = ("unsubscribe_object_attributes", object_class, frozenset(attributes))

    def unsubscribeObjectClassDirectedInteractions(self, object_class: bytes, interactions: object = None) -> None:
        self.last_declaration_call = (
            "unsubscribe_object_directed",
            object_class,
            None if interactions is None else frozenset(interactions),
        )

    def subscribeObjectClassAttributesWithRegions(
        self, object_class: bytes, pairs: object, active: bool, update_rate: str
    ) -> None:
        self.last_declaration_call = (
            "subscribe_object_regions",
            object_class,
            tuple(pairs),
            active,
            update_rate,
        )

    def unsubscribeObjectClassAttributesWithRegions(self, object_class: bytes, pairs: object) -> None:
        self.last_declaration_call = ("unsubscribe_object_regions", object_class, tuple(pairs))

    def publishInteractionClass(self, interaction_class: bytes) -> None:
        self.last_declaration_call = ("publish_interaction", interaction_class)

    def unpublishInteractionClass(self, interaction_class: bytes) -> None:
        self.last_declaration_call = ("unpublish_interaction", interaction_class)

    def subscribeInteractionClass(self, interaction_class: bytes, active: bool) -> None:
        self.last_declaration_call = ("subscribe_interaction", interaction_class, active)

    def subscribeInteractionClassWithRegions(
        self, interaction_class: bytes, regions: set[bytes], active: bool
    ) -> None:
        self.last_declaration_call = (
            "subscribe_interaction_regions", interaction_class, frozenset(regions), active
        )

    def unsubscribeInteractionClass(self, interaction_class: bytes) -> None:
        self.last_declaration_call = ("unsubscribe_interaction", interaction_class)

    def unsubscribeInteractionClassWithRegions(
        self, interaction_class: bytes, regions: set[bytes]
    ) -> None:
        self.last_declaration_call = (
            "unsubscribe_interaction_regions", interaction_class, frozenset(regions)
        )

    def registerObjectInstance(self, object_class: bytes, object_instance_name: str | None = None) -> bytes:
        name = object_instance_name or f"generated-{self._next_object_instance}"
        self._next_object_instance += 1
        if object_instance_name is not None:
            self.reserved_object_instance_names.discard(object_instance_name)
        handle = self._handle("instance", name)
        self.object_instances[name] = handle
        return handle

    def registerObjectInstanceWithRegions(
        self,
        object_class: bytes,
        pairs: object,
        object_instance_name: str | None = None,
    ) -> bytes:
        self.last_object_region_pairs = tuple(pairs)
        return self.registerObjectInstance(object_class, object_instance_name)

    def associateRegionsForUpdates(self, handle: bytes, pairs: object) -> None:
        self.last_object_region_association = (handle, tuple(pairs))

    def unassociateRegionsForUpdates(self, handle: bytes, pairs: object) -> None:
        self.last_object_region_unassociation = (handle, tuple(pairs))

    def getObjectInstanceHandle(self, name: str) -> bytes:
        return self.object_instances[name]

    def getObjectInstanceName(self, handle: bytes) -> str:
        return self._handle_name(handle, "instance")

    def deleteObjectInstance(self, handle: bytes, tag: bytes) -> None:
        name = self._handle_name(handle, "instance")
        del self.object_instances[name]
        self.last_object_instance_deletion = (handle, bytes(tag))

    def deleteObjectInstanceWithTime(self, handle: bytes, tag: bytes, time: object) -> bytes:
        self.last_timestamped_delete = (handle, bytes(tag), time)
        return b"retraction:delete"

    def updateAttributeValues(
        self, handle: bytes, values: dict[bytes, bytes], tag: bytes
    ) -> None:
        self.last_attribute_update = (handle, dict(values), bytes(tag))

    def updateAttributeValuesWithTime(
        self, handle: bytes, values: dict[bytes, bytes], tag: bytes, time: object
    ) -> bytes:
        self.last_timestamped_update = (handle, dict(values), bytes(tag), time)
        return b"retraction:update"

    def requestAttributeValueUpdate(
        self, object_or_class: bytes, attributes: object, tag: bytes
    ) -> None:
        self.last_attribute_value_update_request = (
            object_or_class,
            frozenset(attributes),
            bytes(tag),
        )

    def requestAttributeValueUpdateWithRegions(
        self, object_class: bytes, pairs: object, tag: bytes
    ) -> None:
        self.last_regional_update_request = (object_class, tuple(pairs), bytes(tag))

    def changeAttributeOrderType(self, object_instance: bytes, attributes: object, order_type: object) -> None:
        self.last_order_service = ("attribute", object_instance, frozenset(attributes), order_type)

    def changeDefaultAttributeOrderType(self, object_class: bytes, attributes: object, order_type: object) -> None:
        self.last_order_service = ("default-attribute", object_class, frozenset(attributes), order_type)

    def changeInteractionOrderType(self, interaction_class: bytes, order_type: object) -> None:
        self.last_order_service = ("interaction", interaction_class, order_type)

    def requestAttributeTransportationTypeChange(
        self, object_instance: bytes, attributes: object, transportation_type: bytes
    ) -> None:
        self.last_transportation_service = (
            "attribute-request", object_instance, frozenset(attributes), transportation_type
        )
        self.callback_proxy.confirmAttributeTransportationTypeChange(
            object_instance, attributes, transportation_type
        )  # type: ignore[union-attr]

    def changeDefaultAttributeTransportationType(
        self, object_class: bytes, attributes: object, transportation_type: bytes
    ) -> None:
        self.last_transportation_service = (
            "default-attribute", object_class, frozenset(attributes), transportation_type
        )

    def queryAttributeTransportationType(self, object_instance: bytes, attribute: bytes) -> None:
        self.last_transportation_service = ("attribute-query", object_instance, attribute)
        self.callback_proxy.reportAttributeTransportationType(
            object_instance, attribute, self._handle("transport", "HLAreliable")
        )  # type: ignore[union-attr]

    def requestInteractionTransportationTypeChange(
        self, interaction_class: bytes, transportation_type: bytes
    ) -> None:
        self.last_transportation_service = ("interaction-request", interaction_class, transportation_type)
        self.callback_proxy.confirmInteractionTransportationTypeChange(
            interaction_class, transportation_type
        )  # type: ignore[union-attr]

    def queryInteractionTransportationType(self, federate: bytes, interaction_class: bytes) -> None:
        self.last_transportation_service = ("interaction-query", federate, interaction_class)
        self.callback_proxy.reportInteractionTransportationType(
            federate, interaction_class, self._handle("transport", "HLAreliable")
        )  # type: ignore[union-attr]

    def queryAttributeOwnership(self, object_instance: bytes, attributes: object) -> None:
        self.last_ownership_query = (object_instance, frozenset(attributes))

    def isAttributeOwnedByFederate(self, object_instance: bytes, attribute: bytes) -> bool:
        self.last_ownership_check = (object_instance, attribute)
        return True

    def unconditionalAttributeOwnershipDivestiture(
        self, object_instance: bytes, attributes: object, tag: bytes
    ) -> None:
        self.last_ownership_service = ("divest", object_instance, frozenset(attributes), bytes(tag))

    def attributeOwnershipAcquisition(
        self, object_instance: bytes, attributes: object, tag: bytes
    ) -> None:
        self.last_ownership_service = ("acquire", object_instance, frozenset(attributes), bytes(tag))

    def attributeOwnershipAcquisitionIfAvailable(
        self, object_instance: bytes, attributes: object, tag: bytes
    ) -> None:
        self.last_ownership_service = ("acquire_if_available", object_instance, frozenset(attributes), bytes(tag))

    def negotiatedAttributeOwnershipDivestiture(
        self, object_instance: bytes, attributes: object, tag: bytes
    ) -> None:
        self.last_ownership_service = ("negotiated_divest", object_instance, frozenset(attributes), bytes(tag))

    def confirmDivestiture(self, object_instance: bytes, attributes: object, tag: bytes) -> None:
        self.last_ownership_service = ("confirm_divest", object_instance, frozenset(attributes), bytes(tag))

    def cancelNegotiatedAttributeOwnershipDivestiture(
        self, object_instance: bytes, attributes: object
    ) -> None:
        self.last_ownership_service = ("cancel_negotiated_divest", object_instance, frozenset(attributes))

    def cancelAttributeOwnershipAcquisition(self, object_instance: bytes, attributes: object) -> None:
        self.last_ownership_service = ("cancel", object_instance, frozenset(attributes))

    def attributeOwnershipReleaseDenied(
        self, object_instance: bytes, attributes: object, tag: bytes
    ) -> None:
        self.last_ownership_service = ("release_denied", object_instance, frozenset(attributes), bytes(tag))

    def attributeOwnershipDivestitureIfWanted(
        self, object_instance: bytes, attributes: object, tag: bytes
    ) -> set[bytes]:
        self.last_ownership_service = ("divest_if_wanted", object_instance, frozenset(attributes), bytes(tag))
        return set(attributes)

    def sendInteraction(self, handle: bytes, values: dict[bytes, bytes], tag: bytes) -> None:
        self.last_interaction = (handle, dict(values), bytes(tag))

    def sendInteractionWithTime(
        self, handle: bytes, values: dict[bytes, bytes], tag: bytes, time: object
    ) -> bytes:
        self.last_timestamped_interaction = (handle, dict(values), bytes(tag), time)
        return b"retraction:interaction"

    def sendDirectedInteraction(
        self,
        interaction_class: bytes,
        object_instance: bytes,
        values: dict[bytes, bytes],
        tag: bytes,
        time: object = None,
    ) -> bytes | None:
        record = (interaction_class, object_instance, dict(values), bytes(tag), time)
        if time is None:
            self.last_directed_interaction = record
            if self.callback_proxy is not None:
                self.callback_proxy.receiveDirectedInteraction(
                    interaction_class,
                    object_instance,
                    values,
                    bytes(tag),
                    b"transportation:HLAreliable",
                    b"federate:mock-producing-federate",
                )
            return None
        self.last_timestamped_directed_interaction = record
        return b"retraction:directed-interaction"

    def sendInteractionWithRegions(
        self, handle: bytes, values: dict[bytes, bytes], regions: set[bytes], tag: bytes
    ) -> None:
        self.last_interaction = (handle, dict(values), frozenset(regions), bytes(tag))

    def sendInteractionWithRegionsWithTime(
        self,
        handle: bytes,
        values: dict[bytes, bytes],
        regions: set[bytes],
        tag: bytes,
        time: object,
    ) -> bytes:
        self.last_timestamped_regional_interaction = (
            handle,
            dict(values),
            frozenset(regions),
            bytes(tag),
            time,
        )
        return b"retraction:regional-interaction"

    def retract(self, retraction: bytes) -> None:
        self.last_retracted = bytes(retraction)

    def reserveObjectInstanceName(self, name: str) -> None:
        self.reserved_object_instance_names.add(name)

    def releaseObjectInstanceName(self, name: str) -> None:
        self.reserved_object_instance_names.remove(name)

    def reserveMultipleObjectInstanceNames(self, names: object) -> None:
        self.last_name_batch = frozenset(names)
        self.callback_proxy.multipleObjectInstanceNameReservationSucceeded(names)

    def releaseMultipleObjectInstanceNames(self, names: object) -> None:
        self.last_released_name_batch = frozenset(names)

    def localDeleteObjectInstance(self, object_instance: bytes) -> None:
        self.last_local_delete = bytes(object_instance)


class _FakeJavaDataElement:
    def __init__(self, kind: str, value: object) -> None:
        self.kind = kind
        self.value = value

    def getOctetBoundary(self) -> int:
        return 4

    def getEncodedLength(self) -> int:
        return len(self.toByteArray())

    def getValue(self) -> object:
        return self.value

    def setValue(self, value: object) -> "_FakeJavaDataElement":
        self.value = value
        return self

    def toByteArray(self) -> bytes:
        if self.kind == "integer32":
            return struct.pack(">i", int(self.value))
        if self.kind == "unsigned32":
            return struct.pack(">I", int(self.value) & 0xFFFFFFFF)
        if self.kind == "boolean":
            return struct.pack(">I", 1 if self.value else 0)
        payload = str(self.value).encode("utf-16-be")
        return struct.pack(">I", len(payload) // 2) + payload

    def decode(self, bytes_: bytes) -> "_FakeJavaDataElement":
        if self.kind == "integer32":
            self.value = struct.unpack(">i", bytes_)[0]
        elif self.kind == "unsigned32":
            self.value = struct.unpack(">i", bytes_)[0]
        elif self.kind == "boolean":
            self.value = bool(struct.unpack(">I", bytes_)[0])
        else:
            payload_length = struct.unpack(">I", bytes_[:4])[0] * 2
            self.value = bytes_[4 : 4 + payload_length].decode("utf-16-be")
        return self


class _FakeJavaEncoderFactory:
    def createHLAinteger32BE(self, value: int = 0) -> _FakeJavaDataElement:
        return _FakeJavaDataElement("integer32", value)

    def createHLAunsignedInteger32BE(self, value: int = 0) -> _FakeJavaDataElement:
        return _FakeJavaDataElement("unsigned32", value)

    def createHLAboolean(self, value: bool = False) -> _FakeJavaDataElement:
        return _FakeJavaDataElement("boolean", value)

    def createHLAunicodeString(self, value: str = "") -> _FakeJavaDataElement:
        return _FakeJavaDataElement("unicode", value)


class _FakeJavaFactory:
    def __init__(self, ambassador: _FakeJavaAmbassador) -> None:
        self.ambassador = ambassador
        self.encoder_factory = _FakeJavaEncoderFactory()

    def getRtiAmbassador(self) -> _FakeJavaAmbassador:
        return self.ambassador

    def getEncoderFactory(self) -> object:
        return self.encoder_factory

    def rtiName(self) -> str:
        return "Fake Java RTI"

    def rtiVersion(self) -> str:
        return "2025.test"


class _FakeJavaRuntime:
    def __init__(self) -> None:
        self.ambassador = _FakeJavaAmbassador()
        self.factory = _FakeJavaFactory(self.ambassador)
        self.configuration: JavaProviderConfiguration | None = None

    def get_rti_factory(self, configuration: JavaProviderConfiguration) -> _FakeJavaFactory:
        self.configuration = configuration
        return self.factory

    def callback_model(self, callback_model: CallbackModel) -> str:
        return f"java:{callback_model.name}"

    def bind_federate_ambassador(self, federate_ambassador: FederateAmbassador) -> JavaCallbackBinding:
        proxy = _FakeCallbackProxy(federate_ambassador)
        return JavaCallbackBinding(proxy=proxy, target=proxy)

    def rti_configuration(self, configuration: RtiConfiguration) -> tuple[str, str, str, str]:
        return (
            "java-configuration",
            configuration.configurationName(),
            configuration.rtiAddress(),
            configuration.additionalSettings(),
        )

    def credentials(self, credentials: HLAnoCredentials) -> tuple[str, str, bytes]:
        return ("java-credentials", credentials.getType(), credentials.getData())

    def exception_name(self, error: BaseException) -> str | None:
        return getattr(error, "name", None)

    def federate_handle_bytes(self, handle: object) -> bytes:
        return bytes(handle)  # type: ignore[arg-type]

    def handle_bytes(self, handle: object) -> bytes:
        if isinstance(handle, (bytes, bytearray)):
            return bytes(handle)
        destination = bytearray(int(handle.encodedLength()))  # type: ignore[union-attr]
        handle.encode(destination, 0)  # type: ignore[union-attr]
        return bytes(destination)

    def decode_handle(
        self, ambassador: object, factory_method_name: str, encoded_value: bytes
    ) -> bytes:
        return encoded_value

    def cast_handle(self, handle: object, interface_name: str) -> object:
        return handle

    def attribute_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> frozenset[bytes]:
        return frozenset(encoded_values)

    def federate_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> frozenset[bytes]:
        return frozenset(encoded_values)

    def fom_module_url(self, value: str) -> str:
        return f"java-url:{value}"

    def fom_module_urls(self, values: tuple[str, ...]) -> tuple[str, ...]:
        return tuple(self.fom_module_url(value) for value in values)

    def interaction_class_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> frozenset[bytes]:
        return frozenset(encoded_values)

    def object_instance_name_set(self, encoded_values: tuple[str, ...]) -> frozenset[str]:
        return frozenset(encoded_values)

    def attribute_handle_value_map(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[bytes, bytes], ...],
    ) -> dict[bytes, bytes]:
        return dict(encoded_values)

    def parameter_handle_value_map(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[bytes, bytes], ...],
    ) -> dict[bytes, bytes]:
        return dict(encoded_values)

    def attribute_set_region_set_pair_list(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[tuple[bytes, ...], tuple[bytes, ...]], ...],
    ) -> tuple[tuple[tuple[bytes, ...], tuple[bytes, ...]], ...]:
        return encoded_values

    def logical_time_factory(self, ambassador: object) -> _FakeJavaTimeFactory:
        return self.ambassador.time_factory

    def decode_logical_time(self, ambassador: object, encoded_value: bytes) -> _FakeJavaTime:
        return self.ambassador.time_factory.decodeLogicalTime(encoded_value, 0)

    def decode_logical_interval(self, ambassador: object, encoded_value: bytes) -> _FakeJavaInterval:
        return self.ambassador.time_factory.decodeLogicalTimeInterval(encoded_value, 0)

    def dimension_handle_set(
        self, ambassador: object, encoded_values: tuple[bytes, ...]
    ) -> set[bytes]:
        return set(encoded_values)

    def region_handle_set(
        self, ambassador: object, encoded_values: tuple[bytes, ...]
    ) -> set[bytes]:
        return set(encoded_values)

    def range_bounds(self, lower_bound: int, upper_bound: int) -> _FakeJavaRangeBounds:
        return _FakeJavaRangeBounds(lower_bound, upper_bound)

    def resign_action(self, resign_action_name: str) -> str:
        return f"java:{resign_action_name}"

    def order_type(self, order_type_name: str) -> str:
        return f"java:{order_type_name}"

    def service_group(self, service_group_name: str) -> str:
        return f"java:{service_group_name}"

    def byte_array(self, value: bytes) -> bytes:
        return value


class _RecordingFederateAmbassador(FederateAmbassador):
    def __init__(self) -> None:
        self.connection_losses: list[str] = []
        self.federation_execution_reports: list[FederationExecutionInformationSet] = []
        self.federation_execution_member_reports: list[
            tuple[str, FederationExecutionMemberInformationSet]
        ] = []
        self.missing_federation_executions: list[str] = []
        self.time_regulation: list[HLAinteger64Time] = []
        self.time_constrained: list[HLAinteger64Time] = []
        self.time_grants: list[HLAinteger64Time] = []
        self.attribute_value_requests: list[tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]] = []
        self.scope_entries: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.scope_exits: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.updates_on: list[tuple[ObjectInstanceHandle, AttributeHandleSet, str | None]] = []
        self.updates_off: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.attribute_transport_confirmations: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet, TransportationTypeHandle]
        ] = []
        self.attribute_transport_reports: list[
            tuple[ObjectInstanceHandle, AttributeHandle, TransportationTypeHandle]
        ] = []
        self.interaction_transport_confirmations: list[
            tuple[InteractionClassHandle, TransportationTypeHandle]
        ] = []
        self.interaction_transport_reports: list[
            tuple[FederateHandle, InteractionClassHandle, TransportationTypeHandle]
        ] = []
        self.sync_registration: list[str] = []
        self.sync_announcements: list[tuple[str, bytes]] = []
        self.sync_completions: list[tuple[str, FederateHandleSet]] = []
        self.resignations: list[str] = []
        self.flush_grants: list[tuple[object, object]] = []
        self.request_retractions: list[MessageRetractionHandle] = []
        self.directed_interactions: list[tuple[object, ...]] = []
        self.timestamped_directed_interactions: list[tuple[object, ...]] = []
        self.timed_reflections: list[tuple[object, ...]] = []
        self.timed_interactions: list[tuple[object, ...]] = []
        self.name_batch_successes: list[ObjectInstanceNameSet] = []
        self.name_batch_failures: list[ObjectInstanceNameSet] = []
        self.ownership_assumptions: list[tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]] = []
        self.ownership_divestiture_confirmations: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]
        ] = []
        self.ownership_acquisitions: list[tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]] = []
        self.ownership_unavailable: list[tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]] = []
        self.ownership_release_requests: list[tuple[ObjectInstanceHandle, AttributeHandleSet, bytes]] = []
        self.ownership_acquisition_cancellations: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet]
        ] = []
        self.ownership_reports: list[
            tuple[ObjectInstanceHandle, AttributeHandleSet, FederateHandle]
        ] = []
        self.ownership_not_owned: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.ownership_owned_by_rti: list[tuple[ObjectInstanceHandle, AttributeHandleSet]] = []
        self.save_status_reports: list[tuple[FederateHandleSaveStatusPair, ...]] = []
        self.save_initiations: list[str] = []
        self.save_completions = 0
        self.save_failures: list[SaveFailureReason] = []
        self.restore_status_reports: list[tuple[FederateRestoreStatus, ...]] = []
        self.directed_interactions: list[tuple[InteractionClassHandle, ObjectInstanceHandle, ParameterHandleValueMap, bytes, TransportationTypeHandle, FederateHandle]] = []
        self.timestamped_directed_interactions: list[tuple[object, ...]] = []

    def connectionLost(self, faultDescription: str) -> None:
        self.connection_losses.append(faultDescription)

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        self.federation_execution_reports.append(report)

    def reportFederationExecutionMembers(
        self,
        federationExecutionName: str,
        report: FederationExecutionMemberInformationSet,
    ) -> None:
        self.federation_execution_member_reports.append((federationExecutionName, report))

    def reportFederationExecutionDoesNotExist(self, federationExecutionName: str) -> None:
        self.missing_federation_executions.append(federationExecutionName)

    def federateResigned(self, reasonForResignDescription: str) -> None:
        self.resignations.append(reasonForResignDescription)

    def provideAttributeValueUpdate(
        self, objectInstance, attributes, userSuppliedTag
    ) -> None:
        self.attribute_value_requests.append((objectInstance, attributes, userSuppliedTag))

    def attributesInScope(self, objectInstance, attributes) -> None:
        self.scope_entries.append((objectInstance, attributes))

    def attributesOutOfScope(self, objectInstance, attributes) -> None:
        self.scope_exits.append((objectInstance, attributes))

    def turnUpdatesOnForObjectInstance(self, objectInstance, attributes, updateRateDesignator=None) -> None:
        self.updates_on.append((objectInstance, attributes, updateRateDesignator))

    def turnUpdatesOffForObjectInstance(self, objectInstance, attributes) -> None:
        self.updates_off.append((objectInstance, attributes))

    def confirmAttributeTransportationTypeChange(
        self, objectInstance, attributes, transportationType
    ) -> None:
        self.attribute_transport_confirmations.append((objectInstance, attributes, transportationType))

    def reportAttributeTransportationType(self, objectInstance, attribute, transportationType) -> None:
        self.attribute_transport_reports.append((objectInstance, attribute, transportationType))

    def confirmInteractionTransportationTypeChange(self, interactionClass, transportationType) -> None:
        self.interaction_transport_confirmations.append((interactionClass, transportationType))

    def reportInteractionTransportationType(self, federate, interactionClass, transportationType) -> None:
        self.interaction_transport_reports.append((federate, interactionClass, transportationType))

    def synchronizationPointRegistrationSucceeded(self, label: str) -> None:
        self.sync_registration.append(label)

    def announceSynchronizationPoint(self, label: str, tag: bytes) -> None:
        self.sync_announcements.append((label, tag))

    def federationSynchronized(self, label: str, failedToSyncSet: FederateHandleSet) -> None:
        self.sync_completions.append((label, failedToSyncSet))

    def receiveDirectedInteraction(
        self,
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        *timed,
    ) -> None:
        entry = (
            interactionClass,
            objectInstance,
            parameterValues,
            bytes(userSuppliedTag),
            transportationType,
            producingFederate,
        )
        if timed:
            self.timestamped_directed_interactions.append(entry + tuple(timed))
        else:
            self.directed_interactions.append(entry)

    def reflectAttributeValues(self, *arguments: object) -> None:
        self.timed_reflections.append(arguments)

    def receiveInteraction(self, *arguments: object) -> None:
        self.timed_interactions.append(arguments)

    def multipleObjectInstanceNameReservationSucceeded(self, objectInstanceNames) -> None:
        self.name_batch_successes.append(objectInstanceNames)

    def multipleObjectInstanceNameReservationFailed(self, objectInstanceNames) -> None:
        self.name_batch_failures.append(objectInstanceNames)

    def requestAttributeOwnershipAssumption(
        self, objectInstance, offeredAttributes, userSuppliedTag
    ) -> None:
        self.ownership_assumptions.append((objectInstance, offeredAttributes, userSuppliedTag))

    def requestDivestitureConfirmation(
        self, objectInstance, releasedAttributes, userSuppliedTag
    ) -> None:
        self.ownership_divestiture_confirmations.append(
            (objectInstance, releasedAttributes, userSuppliedTag)
        )

    def attributeOwnershipAcquisitionNotification(
        self, objectInstance, securedAttributes, userSuppliedTag
    ) -> None:
        self.ownership_acquisitions.append((objectInstance, securedAttributes, userSuppliedTag))

    def attributeOwnershipUnavailable(self, objectInstance, attributes, userSuppliedTag) -> None:
        self.ownership_unavailable.append((objectInstance, attributes, userSuppliedTag))

    def requestAttributeOwnershipRelease(
        self, objectInstance, candidateAttributes, userSuppliedTag
    ) -> None:
        self.ownership_release_requests.append(
            (objectInstance, candidateAttributes, userSuppliedTag)
        )

    def confirmAttributeOwnershipAcquisitionCancellation(self, objectInstance, attributes) -> None:
        self.ownership_acquisition_cancellations.append((objectInstance, attributes))

    def informAttributeOwnership(self, objectInstance, attributes, owner) -> None:
        self.ownership_reports.append((objectInstance, attributes, owner))

    def attributeIsNotOwned(self, objectInstance, attributes) -> None:
        self.ownership_not_owned.append((objectInstance, attributes))

    def attributeIsOwnedByRTI(self, objectInstance, attributes) -> None:
        self.ownership_owned_by_rti.append((objectInstance, attributes))

    def federationSaveStatusResponse(self, response) -> None:
        self.save_status_reports.append(response)

    def initiateFederateSave(self, label: str) -> None:
        self.save_initiations.append(label)

    def federationSaved(self) -> None:
        self.save_completions += 1

    def federationNotSaved(self, reason: SaveFailureReason) -> None:
        self.save_failures.append(reason)

    def federationRestoreStatusResponse(self, response) -> None:
        self.restore_status_reports.append(response)

    def receiveDirectedInteraction(
        self, interactionClass, objectInstance, parameterValues, userSuppliedTag,
        transportationType, producingFederate, *timed
    ) -> None:
        entry = (
            interactionClass,
            objectInstance,
            parameterValues,
            bytes(userSuppliedTag),
            transportationType,
            producingFederate,
        )
        if timed:
            self.timestamped_directed_interactions.append(entry + tuple(timed))
        else:
            self.directed_interactions.append(entry)

    def timeRegulationEnabled(self, time: HLAinteger64Time) -> None:
        self.time_regulation.append(time)

    def timeConstrainedEnabled(self, time: HLAinteger64Time) -> None:
        self.time_constrained.append(time)

    def flushQueueGrant(self, time: HLAinteger64Time, optimisticTime: HLAinteger64Time) -> None:
        self.flush_grants.append((time, optimisticTime))

    def timeAdvanceGrant(self, time: HLAinteger64Time) -> None:
        self.time_grants.append(time)

    def requestRetraction(self, retraction: MessageRetractionHandle) -> None:
        self.request_retractions.append(retraction)


class JavaProviderTest(unittest.TestCase):
    def setUp(self) -> None:
        self.runtime = _FakeJavaRuntime()
        self.configuration = JavaProviderConfiguration(
            classpath=("vendor-rti.jar",),
            rti_factory_name="Fake Java RTI",
        )
        self.factory = JavaRtiFactory(self.configuration, runtime=self.runtime)

    def test_java_provider_adapts_to_the_shared_python_contract(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()

        result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)

        self.assertEqual(self.factory.rtiName(), "Fake Java RTI")
        self.assertEqual(self.factory.rtiVersion(), "2025.test")
        self.assertTrue(result.configurationUsed)
        self.assertEqual(result.additionalSettingsResultCode.name, "SETTINGS_APPLIED")
        self.assertEqual(self.runtime.ambassador.callback_model, "java:HLA_EVOKED")
        self.runtime.ambassador.callback_proxy.connectionLost("network unavailable")  # type: ignore[union-attr]
        self.assertEqual(callbacks.connection_losses, ["network unavailable"])
        ambassador.joinFederationExecution("observer", "Fake Federation")
        ambassador.registerFederationSynchronizationPoint("sync", b"tag")
        self.assertEqual(callbacks.sync_registration, ["sync"])
        self.assertEqual(callbacks.sync_announcements, [("sync", b"tag")])
        ambassador.synchronizationPointAchieved("sync")
        self.assertEqual(callbacks.sync_completions, [("sync", FederateHandleSet())])
        self.runtime.ambassador.emitAdditionalCallbacksForTest()
        self.assertEqual(callbacks.resignations, ["fake RTI removed this federate"])
        self.assertEqual(len(callbacks.flush_grants), 1)
        self.assertEqual(callbacks.flush_grants[0][0].getTime(), 0)
        self.assertEqual(callbacks.flush_grants[0][1].getTime(), 0)
        self.assertEqual(len(callbacks.request_retractions), 1)
        self.assertTrue(callbacks.request_retractions[0].isValid())
        self.assertEqual(callbacks.request_retractions[0].encodedValue, b"fake-retraction")
        ambassador.disconnect()

    def test_java_callback_proxy_converts_all_ownership_callbacks(self) -> None:
        callbacks = _RecordingFederateAmbassador()
        proxy = _FederateAmbassadorCallback(callbacks, lambda value: bytes(value))
        object_instance = b"object-instance"
        attributes = (b"attribute-one", bytearray(b"attribute-two"))
        tag = (0, 255, 7)

        proxy.requestAttributeOwnershipAssumption(object_instance, attributes, tag)
        proxy.requestDivestitureConfirmation(object_instance, attributes, tag)
        proxy.attributeOwnershipAcquisitionNotification(object_instance, attributes, tag)
        proxy.attributeOwnershipUnavailable(object_instance, attributes, tag)
        proxy.requestAttributeOwnershipRelease(object_instance, attributes, tag)
        proxy.confirmAttributeOwnershipAcquisitionCancellation(object_instance, attributes)
        proxy.informAttributeOwnership(object_instance, attributes, b"owner")
        proxy.attributeIsNotOwned(object_instance, attributes)
        proxy.attributeIsOwnedByRTI(object_instance, attributes)

        expected_instance = ObjectInstanceHandle(object_instance)
        expected_attributes = AttributeHandleSet(
            [AttributeHandle(b"attribute-one"), AttributeHandle(b"attribute-two")]
        )
        expected_tag = b"\x00\xff\x07"
        self.assertEqual(
            callbacks.ownership_assumptions,
            [(expected_instance, expected_attributes, expected_tag)],
        )
        self.assertEqual(
            callbacks.ownership_divestiture_confirmations,
            [(expected_instance, expected_attributes, expected_tag)],
        )
        self.assertEqual(
            callbacks.ownership_acquisitions,
            [(expected_instance, expected_attributes, expected_tag)],
        )
        self.assertEqual(
            callbacks.ownership_unavailable,
            [(expected_instance, expected_attributes, expected_tag)],
        )
        self.assertEqual(
            callbacks.ownership_release_requests,
            [(expected_instance, expected_attributes, expected_tag)],
        )
        self.assertEqual(
            callbacks.ownership_acquisition_cancellations,
            [(expected_instance, expected_attributes)],
        )
        self.assertEqual(
            callbacks.ownership_reports,
            [(expected_instance, expected_attributes, FederateHandle(b"owner"))],
        )
        self.assertEqual(
            callbacks.ownership_not_owned,
            [(expected_instance, expected_attributes)],
        )
        self.assertEqual(
            callbacks.ownership_owned_by_rti,
            [(expected_instance, expected_attributes)],
        )

    def test_java_callback_proxy_converts_save_and_restore_status_records(self) -> None:
        class _RawEnum:
            def __init__(self, name: str) -> None:
                self.name = name

        class _RawSaveStatus:
            def __init__(self, handle: bytes, status: str) -> None:
                self.handle = handle
                self.status = _RawEnum(status)

        class _RawRestoreStatus:
            def __init__(self, pre: bytes, post: bytes, status: str) -> None:
                self.preRestoreHandle = pre
                self.postRestoreHandle = post
                self.status = _RawEnum(status)

        callbacks = _RecordingFederateAmbassador()
        proxy = _FederateAmbassadorCallback(callbacks, lambda value: bytes(value))
        proxy.federationSaveStatusResponse(
            (_RawSaveStatus(b"save-federate", "FEDERATE_SAVING"),)
        )
        proxy.initiateFederateSave("save-label")
        proxy.federationSaved()
        proxy.federationNotSaved(_RawEnum("SAVE_ABORTED"))
        proxy.federationRestoreStatusResponse(
            (
                _RawRestoreStatus(
                    b"restore-pre",
                    b"restore-post",
                    "FEDERATE_RESTORING",
                ),
            )
        )

        self.assertEqual(
            callbacks.save_status_reports,
            [
                (
                    FederateHandleSaveStatusPair(
                        FederateHandle(b"save-federate"), SaveStatus.FEDERATE_SAVING
                    ),
                )
            ],
        )
        self.assertEqual(callbacks.save_initiations, ["save-label"])
        self.assertEqual(callbacks.save_completions, 1)
        self.assertEqual(callbacks.save_failures, [SaveFailureReason.SAVE_ABORTED])
        self.assertEqual(
            callbacks.restore_status_reports,
            [
                (
                    FederateRestoreStatus(
                        FederateHandle(b"restore-pre"),
                        FederateHandle(b"restore-post"),
                        RestoreStatus.FEDERATE_RESTORING,
                    ),
                )
            ],
        )

    def test_java_callback_proxy_converts_timed_region_designators(self) -> None:
        class _RawEntry:
            def __init__(self, key: bytes, value: tuple[int, ...]) -> None:
                self._key = key
                self._value = value

            def getKey(self) -> bytes:
                return self._key

            def getValue(self) -> tuple[int, ...]:
                return self._value

        class _RawMap:
            def __init__(self, *entries: _RawEntry) -> None:
                self._entries = entries

            def entrySet(self) -> tuple[_RawEntry, ...]:
                return self._entries

        class _RawTime:
            def __bytes__(self) -> bytes:
                return struct.pack(">q", 7)

            def implementationName(self) -> str:
                return "HLAinteger64Time"

            def isInitial(self) -> bool:
                return False

            def isFinal(self) -> bool:
                return False

            def getTime(self) -> int:
                return 7

            def toString(self) -> str:
                return "7"

        callbacks = _RecordingFederateAmbassador()
        proxy = _FederateAmbassadorCallback(callbacks, lambda value: bytes(value))
        proxy.reflectAttributeValues(
            b"object-instance",
            _RawMap(_RawEntry(b"attribute", (1, 2, 255))),
            (9, 10),
            b"transportation:HLAreliable",
            b"federate:producer",
            (b"region:callback",),
            _RawTime(),
            _FakeJavaEnum("TIMESTAMP"),
            _FakeJavaEnum("TIMESTAMP"),
            b"retraction:reflection",
        )
        proxy.receiveInteraction(
            b"interaction",
            _RawMap(_RawEntry(b"parameter", (3, 4))),
            (11,),
            b"transportation:HLAreliable",
            b"federate:producer",
            (b"region:callback",),
            _RawTime(),
            _FakeJavaEnum("TIMESTAMP"),
            _FakeJavaEnum("TIMESTAMP"),
            b"retraction:interaction",
        )

        reflection = callbacks.timed_reflections[-1]
        self.assertEqual(reflection[0], ObjectInstanceHandle(b"object-instance"))
        self.assertEqual(reflection[1], AttributeHandleValueMap({AttributeHandle(b"attribute"): b"\x01\x02\xff"}))
        self.assertEqual(reflection[2], b"\x09\x0a")
        self.assertEqual(reflection[5], RegionHandleSet([RegionHandle(b"region:callback")]))
        self.assertEqual(reflection[6].getTime(), 7)
        self.assertEqual(reflection[7], OrderType.TIMESTAMP)
        self.assertTrue(reflection[9].isValid())

        interaction = callbacks.timed_interactions[-1]
        self.assertEqual(interaction[0], InteractionClassHandle(b"interaction"))
        self.assertEqual(interaction[1], ParameterHandleValueMap({ParameterHandle(b"parameter"): b"\x03\x04"}))
        self.assertEqual(interaction[5], RegionHandleSet([RegionHandle(b"region:callback")]))
        self.assertEqual(interaction[6].getTime(), 7)
        self.assertEqual(interaction[7], OrderType.TIMESTAMP)
        self.assertTrue(interaction[9].isValid())

        proxy.reflectAttributeValues(
            b"object-instance",
            _RawMap(_RawEntry(b"attribute", (5,))),
            (12,),
            b"transportation:HLAreliable",
            b"federate:producer",
            None,
            _RawTime(),
            _FakeJavaEnum("TIMESTAMP"),
            _FakeJavaEnum("RECEIVE"),
            b"retraction:no-region",
        )
        self.assertIsNone(callbacks.timed_reflections[-1][5])
        self.assertEqual(callbacks.timed_reflections[-1][8], OrderType.RECEIVE)

    def test_java_provider_exposes_standard_handle_and_collection_factories(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")

        for factory, handle_type in (
            (ambassador.getFederateHandleFactory(), FederateHandle),
            (ambassador.getObjectClassHandleFactory(), ObjectClassHandle),
            (ambassador.getObjectInstanceHandleFactory(), ObjectInstanceHandle),
            (ambassador.getAttributeHandleFactory(), AttributeHandle),
            (ambassador.getInteractionClassHandleFactory(), InteractionClassHandle),
            (ambassador.getParameterHandleFactory(), ParameterHandle),
            (ambassador.getTransportationTypeHandleFactory(), TransportationTypeHandle),
            (ambassador.getDimensionHandleFactory(), DimensionHandle),
            (ambassador.getRegionHandleFactory(), RegionHandle),
        ):
            decoded = factory.decode(b"factory-encoded")
            self.assertIsInstance(decoded, handle_type)
            self.assertEqual(decoded.encodedValue, b"factory-encoded")

        attribute_set = ambassador.getAttributeHandleSetFactory().create()
        dimension_set = ambassador.getDimensionHandleSetFactory().create()
        federate_set = ambassador.getFederateHandleSetFactory().create()
        region_set = ambassador.getRegionHandleSetFactory().create()
        self.assertIsInstance(attribute_set, MutableAttributeHandleSet)
        self.assertIsInstance(dimension_set, MutableDimensionHandleSet)
        self.assertIsInstance(federate_set, MutableFederateHandleSet)
        self.assertIsInstance(region_set, MutableRegionHandleSet)
        attribute_set.add(AttributeHandle(b"attribute"))
        dimension_set.add(DimensionHandle(b"dimension"))
        federate_set.add(FederateHandle(b"federate"))
        region_set.add(RegionHandle(b"region"))

        attribute_values = ambassador.getAttributeHandleValueMapFactory().create()
        parameter_values = ambassador.getParameterHandleValueMapFactory().create()
        self.assertIsInstance(attribute_values, MutableAttributeHandleValueMap)
        self.assertIsInstance(parameter_values, MutableParameterHandleValueMap)
        attribute_values[AttributeHandle(b"attribute")] = bytearray(b"value")
        parameter_values[ParameterHandle(b"parameter")] = bytearray(b"value")
        self.assertEqual(attribute_values[AttributeHandle(b"attribute")], b"value")
        self.assertEqual(parameter_values[ParameterHandle(b"parameter")], b"value")

        ambassador.publishObjectClassAttributes(ObjectClassHandle(b"object"), attribute_set)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[1], b"object")
        ambassador.disconnect()

    def test_java_exceptions_are_translated_at_the_provider_edge(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_IMMEDIATE)

        with self.assertRaises(AlreadyConnected):
            ambassador.connect(callbacks, CallbackModel.HLA_IMMEDIATE)

    def test_java_exception_names_outside_the_initial_service_slice_remain_typed(self) -> None:
        self.runtime.ambassador.connect_error_name = "ConnectionFailed"

        with self.assertRaises(ConnectionFailed):
            self.factory.getRtiAmbassador().connect(
                _RecordingFederateAmbassador(),
                CallbackModel.HLA_EVOKED,
            )

    def test_java_provider_selects_each_standard_connect_overload(self) -> None:
        configuration = RtiConfiguration.createConfiguration().withConfigurationName("fake")
        credentials = HLAnoCredentials()
        expected_arguments = (
            ((), ()),
            ((configuration,), (("java-configuration", "fake", "", ""),)),
            ((credentials,), (("java-credentials", "HLAnoCredentials", b""),)),
            (
                (configuration, credentials),
                (
                    ("java-configuration", "fake", "", ""),
                    ("java-credentials", "HLAnoCredentials", b""),
                ),
            ),
        )
        for supplied, expected in expected_arguments:
            with self.subTest(supplied=supplied):
                ambassador = self.factory.getRtiAmbassador()
                ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED, *supplied)
                self.assertEqual(self.runtime.ambassador.connect_arguments, expected)
                ambassador.disconnect()

    def test_java_provider_converts_federation_execution_report_callbacks(self) -> None:
        callback = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.listFederationExecutions()

        self.assertEqual(
            callback.federation_execution_reports,
            [
                FederationExecutionInformationSet(
                    [FederationExecutionInformation("Fake Federation", "HLAinteger64Time")]
                )
            ],
        )
        ambassador.disconnect()

    def test_java_provider_forwards_federation_create_and_destroy_calls(self) -> None:
        callback = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)
        ambassador.createFederationExecution("Created Federation", "example.xml", "HLAinteger64Time")
        ambassador.listFederationExecutions()
        self.assertIn(
            FederationExecutionInformation("Created Federation", "HLAinteger64Time"),
            callback.federation_execution_reports[-1],
        )
        ambassador.destroyFederationExecution("Created Federation")
        ambassador.disconnect()

    def test_java_provider_maps_fom_mim_join_and_synchronization_overloads(self) -> None:
        callback = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)

        ambassador.createFederationExecution(
            "Module Federation", ["base.xml", "extension.xml"], "HLAinteger64Time"
        )
        self.assertEqual(
            self.runtime.ambassador.last_fom_modules,
            ("java-url:base.xml", "java-url:extension.xml"),
        )
        ambassador.createFederationExecutionWithMIM(
            "MIM Federation", ["base.xml"], "HLAstandardMIM.xml", "HLAinteger64Time"
        )
        self.assertEqual(self.runtime.ambassador.last_mim_module, "java-url:HLAstandardMIM.xml")

        unnamed = ambassador.joinFederationExecution(
            "observer", "Module Federation", additionalFomModules=["additional.xml"]
        )
        named = ambassador.joinFederationExecution(
            "observer",
            "Module Federation",
            federateName="named-observer",
            additionalFomModules=["additional.xml"],
        )
        self.assertEqual(unnamed.encodedValue, b"Module Federation:observer:observer")
        self.assertEqual(named.encodedValue, b"Module Federation:named-observer:observer")
        explicit_set = FederateHandleSet([FederateHandle(b"target")])
        ambassador.registerFederationSynchronizationPoint(
            "explicit-sync", b"tag", synchronizationSet=explicit_set
        )
        self.assertEqual(self.runtime.ambassador.last_synchronization_set, frozenset({b"target"}))
        ambassador.disconnect()

    def test_java_provider_converts_federation_member_callbacks(self) -> None:
        callback = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callback, CallbackModel.HLA_EVOKED)

        ambassador.listFederationExecutionMembers("Fake Federation")
        ambassador.listFederationExecutionMembers("Missing Federation")

        self.assertEqual(
            callback.federation_execution_member_reports,
            [("Fake Federation", FederationExecutionMemberInformationSet())],
        )
        self.assertEqual(callback.missing_federation_executions, ["Missing Federation"])
        ambassador.disconnect()

    def test_java_provider_adapts_join_handle_and_resign_action(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_EVOKED)
        handle = ambassador.joinFederationExecution(
            "observer", "Fake Federation", federateName="named-observer"
        )
        self.assertEqual(handle.encodedValue, b"Fake Federation:named-observer:observer")
        ambassador.resignFederationExecution(ResignAction.NO_ACTION)
        self.assertEqual(self.runtime.ambassador.last_resign_action, "java:NO_ACTION")
        ambassador.disconnect()

    def test_java_provider_adapts_logical_time_factory_and_callbacks(self) -> None:
        callbacks = _RecordingFederateAmbassador()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")

        time_factory = ambassador.getTimeFactory()
        self.assertIsInstance(time_factory, HLAinteger64TimeFactory)
        self.assertEqual(time_factory.implementationName(), "HLAinteger64Time")
        self.assertTrue(time_factory.makeInitial().isInitial())
        self.assertTrue(time_factory.makeFinal().isFinal())
        self.assertTrue(time_factory.makeZero().isZero())
        self.assertTrue(time_factory.makeEpsilon().isEpsilon())
        requested_time = time_factory.makeLogicalTime(5)
        lookahead = time_factory.makeLogicalTimeInterval(1)
        self.assertIsInstance(requested_time, HLAinteger64Time)
        self.assertIsInstance(lookahead, HLAinteger64Interval)
        self.assertEqual(time_factory.decodeLogicalTime(requested_time.toByteArray()).getTime(), 5)
        self.assertEqual(
            time_factory.decodeLogicalTimeInterval(lookahead.toByteArray()).getInterval(), 1
        )
        advanced = time_factory.add(requested_time, lookahead)
        self.assertEqual(advanced.getTime(), 6)
        self.assertEqual(time_factory.subtract(advanced, lookahead).getTime(), 5)
        self.assertEqual(time_factory.difference(advanced, requested_time).getInterval(), 1)

        ambassador.enableTimeRegulation(lookahead)
        ambassador.enableTimeConstrained()
        ambassador.timeAdvanceRequest(requested_time)
        self.assertEqual(callbacks.time_regulation[0].getTime(), 0)
        self.assertEqual(callbacks.time_constrained[0].getTime(), 0)
        self.assertEqual(callbacks.time_grants[0].getTime(), 5)
        for request, value in (
            (ambassador.timeAdvanceRequestAvailable, 6),
            (ambassador.nextMessageRequest, 7),
            (ambassador.nextMessageRequestAvailable, 8),
            (ambassador.flushQueueRequest, 9),
        ):
            request(time_factory.makeLogicalTime(value))
        self.assertEqual(
            [grant.getTime() for grant in callbacks.time_grants], [5, 6, 7, 8, 9]
        )
        self.assertEqual(self.runtime.ambassador.last_lookahead.value, 1)
        self.assertEqual(self.runtime.ambassador.last_time_request.value, 9)
        self.assertEqual(ambassador.queryLogicalTime().getTime(), 9)
        self.assertTrue(ambassador.queryGALT().timeIsValid)
        self.assertEqual(ambassador.queryGALT().time.getTime(), 9)  # type: ignore[union-attr]
        self.assertTrue(ambassador.queryLITS().timeIsValid)
        self.assertEqual(ambassador.queryLITS().time.getTime(), 9)  # type: ignore[union-attr]
        self.assertEqual(ambassador.queryLookahead().getInterval(), 1)
        modified_lookahead = time_factory.makeLogicalTimeInterval(2)
        ambassador.modifyLookahead(modified_lookahead)
        self.assertEqual(ambassador.queryLookahead().getInterval(), 2)
        ambassador.enableAsynchronousDelivery()
        self.assertTrue(self.runtime.ambassador.asynchronous_delivery)
        ambassador.disableAsynchronousDelivery()
        self.assertFalse(self.runtime.ambassador.asynchronous_delivery)
        ambassador.disableTimeConstrained()
        ambassador.disableTimeRegulation()
        self.assertFalse(self.runtime.ambassador.time_constrained)
        self.assertFalse(self.runtime.ambassador.time_regulating)
        ambassador.disconnect()

    def test_java_provider_preserves_floating_logical_time_values(self) -> None:
        self.runtime.ambassador.time_factory = _FakeJavaFloatTimeFactory()
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")

        time_factory = ambassador.getTimeFactory()
        self.assertIsInstance(time_factory, HLAfloat64TimeFactory)
        self.assertEqual(time_factory.implementationName(), "HLAfloat64Time")
        requested_time = time_factory.makeLogicalTime(12.5)
        lookahead = time_factory.makeLogicalTimeInterval(0.25)
        self.assertIsInstance(requested_time, HLAfloat64Time)
        self.assertIsInstance(lookahead, HLAfloat64Interval)
        self.assertEqual(requested_time.getTime(), 12.5)
        self.assertEqual(lookahead.getInterval(), 0.25)
        epsilon = time_factory.makeEpsilon()
        self.assertEqual(epsilon.getInterval(), math.nextafter(0.0, 1.0))
        self.assertTrue(epsilon.isEpsilon())
        negative_zero = time_factory.makeLogicalTime(-0.0)
        self.assertEqual(negative_zero.getTime(), 0.0)
        self.assertTrue(negative_zero.isInitial())
        self.assertEqual(
            time_factory.decodeLogicalTime(requested_time.toByteArray()).getTime(), 12.5
        )
        self.assertEqual(
            time_factory.decodeLogicalTimeInterval(lookahead.toByteArray()).getInterval(), 0.25
        )
        base = time_factory.makeLogicalTime(1.0)
        advanced = time_factory.add(base, epsilon)
        self.assertEqual(advanced.getTime(), math.nextafter(1.0, math.inf))
        self.assertEqual(time_factory.subtract(advanced, epsilon).getTime(), 1.0)
        self.assertEqual(
            time_factory.difference(advanced, base).getInterval(),
            math.nextafter(1.0, math.inf) - 1.0,
        )
        ambassador.enableTimeRegulation(lookahead)
        tiny_time = time_factory.makeLogicalTime(math.nextafter(0.0, 1.0))
        ambassador.timeAdvanceRequest(tiny_time)
        self.assertEqual(callbacks.time_grants[-1].getTime(), math.nextafter(0.0, 1.0))
        ambassador.disconnect()

    def test_java_provider_adapts_region_values_and_ddm_lifecycle(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")
        dimension = ambassador.getDimensionHandle("SodaFlavor")
        region = ambassador.createRegion(DimensionHandleSet([dimension]))
        self.assertIsInstance(region, RegionHandle)
        self.assertEqual(ambassador.getDimensionHandleSet(region), DimensionHandleSet([dimension]))
        bounds = RangeBounds(1, 3)
        ambassador.setRangeBounds(region, dimension, bounds)
        queried = ambassador.getRangeBounds(region, dimension)
        self.assertEqual((queried.getLowerBound(), queried.getUpperBound()), (1, 3))
        self.assertFalse(ambassador.getConveyRegionDesignatorSetsSwitch())
        ambassador.setConveyRegionDesignatorSetsSwitch(True)
        self.assertTrue(ambassador.getConveyRegionDesignatorSetsSwitch())
        for getter, setter in (
            (
                ambassador.getObjectClassRelevanceAdvisorySwitch,
                ambassador.setObjectClassRelevanceAdvisorySwitch,
            ),
            (
                ambassador.getAttributeRelevanceAdvisorySwitch,
                ambassador.setAttributeRelevanceAdvisorySwitch,
            ),
            (
                ambassador.getAttributeScopeAdvisorySwitch,
                ambassador.setAttributeScopeAdvisorySwitch,
            ),
            (
                ambassador.getInteractionRelevanceAdvisorySwitch,
                ambassador.setInteractionRelevanceAdvisorySwitch,
            ),
        ):
            initial = getter()
            setter(not initial)
            self.assertEqual(getter(), not initial)
            setter(initial)
            self.assertEqual(getter(), initial)
        initial_resign = ambassador.getAutomaticResignDirective()
        alternate_resign = next(action for action in ResignAction if action is not initial_resign)
        ambassador.setAutomaticResignDirective(alternate_resign)
        self.assertEqual(ambassador.getAutomaticResignDirective(), alternate_resign)
        ambassador.setAutomaticResignDirective(initial_resign)
        self.assertEqual(ambassador.getAutomaticResignDirective(), initial_resign)
        for getter, setter in (
            (ambassador.getServiceReportingSwitch, ambassador.setServiceReportingSwitch),
            (ambassador.getExceptionReportingSwitch, ambassador.setExceptionReportingSwitch),
        ):
            initial = getter()
            setter(not initial)
            self.assertEqual(getter(), not initial)
            setter(initial)
            self.assertEqual(getter(), initial)
        for getter in (
            ambassador.getSendServiceReportsToFileSwitch,
            ambassador.getAutoProvideSwitch,
            ambassador.getDelaySubscriptionEvaluationSwitch,
            ambassador.getAdvisoriesUseKnownClassSwitch,
            ambassador.getAllowRelaxedDDMSwitch,
            ambassador.getNonRegulatedGrantSwitch,
        ):
            self.assertIsInstance(getter(), bool)
        ambassador.commitRegionModifications(RegionHandleSet([region]))
        ambassador.deleteRegion(region)
        with self.assertRaises(RTIinternalError):
            ambassador.getDimensionHandleSet(region)
        ambassador.disconnect()

    def test_java_provider_forwards_regional_interaction_services(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")
        interaction = ambassador.getInteractionClassHandle(
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
        )
        parameter = ambassador.getParameterHandle(interaction, "TemperatureOk")
        dimension = ambassador.getDimensionHandle("SodaFlavor")
        region = ambassador.createRegion(DimensionHandleSet([dimension]))
        regions = RegionHandleSet([region])
        ambassador.subscribeInteractionClassWithRegions(interaction, regions, active=False)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "subscribe_interaction_regions")
        ambassador.sendInteractionWithRegions(
            interaction,
            ParameterHandleValueMap({parameter: b"regional"}),
            regions,
            b"regional-tag",
        )
        self.assertEqual(self.runtime.ambassador.last_interaction[2], frozenset([region.encodedValue]))
        ambassador.unsubscribeInteractionClassWithRegions(interaction, regions)
        self.assertEqual(
            self.runtime.ambassador.last_declaration_call[0], "unsubscribe_interaction_regions"
        )
        ambassador.disconnect()

    def test_java_provider_forwards_regional_object_pair_vectors(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        callbacks = _RecordingFederateAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
        dimension = ambassador.getDimensionHandle("ServerId")
        region = ambassador.createRegion(DimensionHandleSet([dimension]))
        pairs = AttributeSetRegionSetPairList(
            [AttributeSetRegionSetPair(AttributeHandleSet([attribute]), RegionHandleSet([region]))]
        )
        ambassador.subscribeObjectClassAttributesWithRegions(
            object_class, pairs, active=False, updateRateDesignator="High"
        )
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "subscribe_object_regions")
        instance = ambassador.registerObjectInstanceWithRegions(object_class, pairs)
        self.assertIsInstance(instance, ObjectInstanceHandle)
        self.runtime.ambassador.callback_proxy.provideAttributeValueUpdate(
            instance.encodedValue,
            [attribute.encodedValue],
            b"provided",
        )
        self.assertEqual(callbacks.attribute_value_requests[0][0], instance)
        self.assertEqual(callbacks.attribute_value_requests[0][1], AttributeHandleSet([attribute]))
        self.assertEqual(callbacks.attribute_value_requests[0][2], b"provided")
        self.runtime.ambassador.callback_proxy.attributesInScope(
            instance.encodedValue, [attribute.encodedValue]
        )
        self.runtime.ambassador.callback_proxy.attributesOutOfScope(
            instance.encodedValue, [attribute.encodedValue]
        )
        self.runtime.ambassador.callback_proxy.turnUpdatesOnForObjectInstance(
            instance.encodedValue, [attribute.encodedValue], "High"
        )
        self.runtime.ambassador.callback_proxy.turnUpdatesOffForObjectInstance(
            instance.encodedValue, [attribute.encodedValue]
        )
        self.assertEqual(callbacks.scope_entries[-1], (instance, AttributeHandleSet([attribute])))
        self.assertEqual(callbacks.scope_exits[-1], (instance, AttributeHandleSet([attribute])))
        self.assertEqual(callbacks.updates_on[-1][2], "High")
        self.assertEqual(callbacks.updates_off[-1], (instance, AttributeHandleSet([attribute])))
        ambassador.requestAttributeValueUpdate(object_class, AttributeHandleSet([attribute]), b"class-request")
        self.assertEqual(self.runtime.ambassador.last_attribute_value_update_request[2], b"class-request")
        ambassador.requestAttributeValueUpdate(instance, AttributeHandleSet([attribute]), b"instance-request")
        self.assertEqual(self.runtime.ambassador.last_attribute_value_update_request[2], b"instance-request")
        ambassador.associateRegionsForUpdates(instance, pairs)
        ambassador.unassociateRegionsForUpdates(instance, pairs)
        ambassador.requestAttributeValueUpdateWithRegions(object_class, pairs, b"request")
        transportation = ambassador.getTransportationTypeHandle("HLAreliable")
        ambassador.changeAttributeOrderType(instance, AttributeHandleSet([attribute]), OrderType.TIMESTAMP)
        ambassador.changeDefaultAttributeOrderType(
            object_class, AttributeHandleSet([attribute]), OrderType.RECEIVE
        )
        ambassador.changeInteractionOrderType(
            ambassador.getInteractionClassHandle("HLAinteractionRoot.CustomerTransactions"),
            OrderType.RECEIVE,
        )
        ambassador.requestAttributeTransportationTypeChange(
            instance, AttributeHandleSet([attribute]), transportation
        )
        ambassador.changeDefaultAttributeTransportationType(
            object_class, AttributeHandleSet([attribute]), transportation
        )
        ambassador.queryAttributeTransportationType(instance, attribute)
        interaction = ambassador.getInteractionClassHandle("HLAinteractionRoot.CustomerTransactions")
        federate = ambassador.getFederateHandle("mock-owner")
        ambassador.requestInteractionTransportationTypeChange(interaction, transportation)
        ambassador.queryInteractionTransportationType(federate, interaction)
        self.assertEqual(callbacks.attribute_transport_confirmations[-1][2], transportation)
        self.assertEqual(callbacks.attribute_transport_reports[-1][2], transportation)
        self.assertEqual(callbacks.interaction_transport_confirmations[-1][1], transportation)
        self.assertEqual(callbacks.interaction_transport_reports[-1][0], federate)
        ambassador.unsubscribeObjectClassAttributesWithRegions(object_class, pairs)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unsubscribe_object_regions")
        self.assertEqual(self.runtime.ambassador.last_regional_update_request[2], b"request")
        ambassador.disconnect()

    def test_java_provider_forwards_ownership_query_and_status(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
        instance = ambassador.registerObjectInstance(object_class)
        ambassador.queryAttributeOwnership(instance, AttributeHandleSet([attribute]))
        self.assertEqual(self.runtime.ambassador.last_ownership_query[1], {attribute.encodedValue})
        self.assertTrue(ambassador.isAttributeOwnedByFederate(instance, attribute))
        self.assertEqual(self.runtime.ambassador.last_ownership_check[1], attribute.encodedValue)
        ambassador.unconditionalAttributeOwnershipDivestiture(instance, AttributeHandleSet([attribute]), b"divest")
        self.assertEqual(self.runtime.ambassador.last_ownership_service[0], "divest")
        ambassador.attributeOwnershipAcquisition(instance, AttributeHandleSet([attribute]), b"acquire")
        self.assertEqual(self.runtime.ambassador.last_ownership_service[0], "acquire")
        ambassador.attributeOwnershipAcquisitionIfAvailable(
            instance, AttributeHandleSet([attribute]), b"available"
        )
        self.assertEqual(self.runtime.ambassador.last_ownership_service[0], "acquire_if_available")
        ambassador.cancelAttributeOwnershipAcquisition(instance, AttributeHandleSet([attribute]))
        self.assertEqual(self.runtime.ambassador.last_ownership_service[0], "cancel")
        ambassador.negotiatedAttributeOwnershipDivestiture(
            instance, AttributeHandleSet([attribute]), b"negotiated"
        )
        ambassador.confirmDivestiture(instance, AttributeHandleSet([attribute]), b"confirm")
        ambassador.cancelNegotiatedAttributeOwnershipDivestiture(
            instance, AttributeHandleSet([attribute])
        )
        ambassador.attributeOwnershipReleaseDenied(instance, AttributeHandleSet([attribute]), b"denied")
        self.assertEqual(
            ambassador.attributeOwnershipDivestitureIfWanted(
                instance, AttributeHandleSet([attribute]), b"wanted"
            ),
            AttributeHandleSet([attribute]),
        )
        ambassador.disconnect()

    def test_java_provider_forwards_save_and_restore_service_lifecycle(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")

        ambassador.queryFederationSaveStatus()
        ambassador.requestFederationSave("fake-save")
        ambassador.federateSaveBegun()
        ambassador.federateSaveComplete()
        ambassador.federateSaveNotComplete()
        ambassador.abortFederationSave()
        self.assertTrue(self.runtime.ambassador.save_status_queried)
        self.assertEqual(self.runtime.ambassador.last_save_label, "fake-save")
        self.assertTrue(self.runtime.ambassador.save_begun)
        self.assertTrue(self.runtime.ambassador.save_completed)
        self.assertTrue(self.runtime.ambassador.save_not_completed)
        self.assertTrue(self.runtime.ambassador.save_aborted)
        timed_save_time = ambassador.getTimeFactory().makeLogicalTime(5)
        ambassador.requestFederationSave("fake-timed-save", timed_save_time)
        self.assertEqual(self.runtime.ambassador.last_save_label, "fake-timed-save")
        self.assertIsNotNone(self.runtime.ambassador.last_save_time)
        self.assertEqual(self.runtime.ambassador.last_save_time.value, 5)

        ambassador.queryFederationRestoreStatus()
        ambassador.requestFederationRestore("fake-restore")
        ambassador.federateRestoreComplete()
        ambassador.federateRestoreNotComplete()
        ambassador.abortFederationRestore()
        self.assertTrue(self.runtime.ambassador.restore_status_queried)
        self.assertEqual(self.runtime.ambassador.last_restore_label, "fake-restore")
        self.assertTrue(self.runtime.ambassador.restore_completed)
        self.assertTrue(self.runtime.ambassador.restore_not_completed)
        self.assertTrue(self.runtime.ambassador.restore_aborted)
        ambassador.disconnect()

    def test_java_provider_rebuilds_typed_handles_through_the_standard_factories(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_EVOKED)

        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
        interaction = ambassador.getInteractionClassHandle(
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
        )
        directed = ambassador.getInteractionClassHandle("HLAinteractionRoot.ServerAction.TakeOrder")
        parameter = ambassador.getParameterHandle(interaction, "TemperatureOk")
        transportation = ambassador.getTransportationTypeHandle("HLAreliable")
        dimension = ambassador.getDimensionHandle("ServerId")

        self.assertIsInstance(object_class, ObjectClassHandle)
        self.assertIsInstance(attribute, AttributeHandle)
        self.assertIsInstance(interaction, InteractionClassHandle)
        self.assertIsInstance(parameter, ParameterHandle)
        self.assertIsInstance(transportation, TransportationTypeHandle)
        self.assertIsInstance(dimension, DimensionHandle)
        self.assertEqual(ambassador.getObjectClassName(object_class), "HLAobjectRoot.Employee.Server")
        self.assertEqual(ambassador.getAttributeName(object_class, attribute), "Efficiency")
        self.assertEqual(
            ambassador.getInteractionClassName(interaction),
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
        )
        self.assertEqual(ambassador.getParameterName(interaction, parameter), "TemperatureOk")
        self.assertEqual(ambassador.getTransportationTypeName(transportation), "HLAreliable")
        self.assertEqual(ambassador.getDimensionName(dimension), "ServerId")
        with self.assertRaises(TypeError):
            ambassador.getObjectClassName(attribute)  # type: ignore[arg-type]
        ambassador.disconnect()

    def test_java_provider_forwards_support_lookup_helpers(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")
        federate = ambassador.getFederateHandle("observer")
        self.assertEqual(ambassador.getFederateName(federate), "observer")
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        interaction = ambassador.getInteractionClassHandle(
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
        )
        attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
        instance = ambassador.registerObjectInstance(object_class)
        self.assertEqual(
            ambassador.getKnownObjectClassHandle(instance),
            ObjectClassHandle(b"object:HLAobjectRoot.Employee.Server"),
        )
        self.assertEqual(ambassador.getUpdateRateValue("HLAdefaultUpdateRate"), 1.0)
        self.assertEqual(ambassador.getUpdateRateValueForAttribute(instance, attribute), 1.0)
        self.assertEqual(ambassador.getOrderType("Receive"), OrderType.RECEIVE)
        self.assertEqual(ambassador.getOrderName(OrderType.TIMESTAMP), "TimeStamp")
        self.assertEqual(
            ambassador.getAvailableDimensionsForObjectClass(object_class),
            DimensionHandleSet([DimensionHandle(b"dimension:SodaFlavor")]),
        )
        self.assertEqual(
            ambassador.getAvailableDimensionsForInteractionClass(interaction),
            DimensionHandleSet([DimensionHandle(b"dimension:SodaFlavor")]),
        )
        self.assertEqual(
            ambassador.getDimensionUpperBound(DimensionHandle(b"dimension:SodaFlavor")),
            100,
        )
        self.assertEqual(ambassador.normalizeServiceGroup(ServiceGroup.OBJECT_MANAGEMENT), 2)
        self.assertEqual(ambassador.normalizeFederateHandle(federate), 1)
        self.assertEqual(ambassador.normalizeObjectClassHandle(object_class), 2)
        self.assertEqual(ambassador.normalizeInteractionClassHandle(interaction), 3)
        self.assertEqual(ambassador.normalizeObjectInstanceHandle(instance), 4)
        ambassador.disconnect()

    def test_java_provider_forwards_timestamped_services_and_retraction(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_EVOKED)
        ambassador.joinFederationExecution("observer", "Fake Federation")
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        attribute = ambassador.getAttributeHandle(object_class, "Efficiency")
        region = ambassador.createRegion(
            DimensionHandleSet([DimensionHandle(b"dimension:SodaFlavor")])
        )
        region_pairs = AttributeSetRegionSetPairList(
            [AttributeSetRegionSetPair(AttributeHandleSet([attribute]), RegionHandleSet([region]))]
        )
        instance = ambassador.registerObjectInstanceWithRegions(object_class, region_pairs)
        interaction = ambassador.getInteractionClassHandle(
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
        )
        parameter = ambassador.getParameterHandle(interaction, "TemperatureOk")
        time = ambassador.getTimeFactory().makeLogicalTime(7)
        update = ambassador.updateAttributeValuesWithTime(
            instance,
            AttributeHandleValueMap({attribute: b"timed"}),
            time,
            b"update-tag",
        )
        interaction_retraction = ambassador.sendInteractionWithTime(
            interaction,
            ParameterHandleValueMap({parameter: b"served"}),
            time,
            b"interaction-tag",
        )
        regional = ambassador.sendInteractionWithRegionsWithTime(
            interaction,
            ParameterHandleValueMap({parameter: b"regional"}),
            RegionHandleSet([region]),
            time,
            b"regional-tag",
        )
        deletion = ambassador.deleteObjectInstanceWithTime(instance, time, b"delete-tag")
        for retraction in (update, interaction_retraction, regional, deletion):
            self.assertIsInstance(retraction, MessageRetractionHandle)
            self.assertTrue(retraction.isValid())
        self.assertEqual(
            self.runtime.ambassador.last_object_region_pairs,
            (((attribute.encodedValue,), (region.encodedValue,)),),
        )
        ambassador.retract(update)
        self.assertEqual(self.runtime.ambassador.last_retracted, update.encodedValue)
        ambassador.disconnect()

    def test_java_provider_adapts_basic_declaration_services(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_EVOKED)
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        attributes = AttributeHandleSet([ambassador.getAttributeHandle(object_class, "Efficiency")])
        interaction = ambassador.getInteractionClassHandle(
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
        )
        directed = ambassador.getInteractionClassHandle("HLAinteractionRoot.ServerAction.TakeOrder")

        ambassador.publishObjectClassAttributes(object_class, attributes)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "publish_object")
        ambassador.subscribeObjectClassAttributes(
            object_class, attributes, active=False, updateRateDesignator="slow"
        )
        self.assertEqual(
            self.runtime.ambassador.last_declaration_call[0:1]
            + self.runtime.ambassador.last_declaration_call[-2:],
            ("subscribe_object", False, "slow"),
        )
        ambassador.unpublishObjectClassAttributes(object_class, attributes)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unpublish_object_attributes")
        ambassador.unsubscribeObjectClassAttributes(object_class, attributes)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unsubscribe_object_attributes")
        ambassador.unpublishObjectClass(object_class)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unpublish_object")
        ambassador.unsubscribeObjectClass(object_class)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unsubscribe_object")
        directed_set = InteractionClassHandleSet([directed])
        ambassador.publishObjectClassDirectedInteractions(object_class, directed_set)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "publish_object_directed")
        ambassador.subscribeObjectClassDirectedInteractions(
            object_class, directed_set, universally=True
        )
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "subscribe_object_directed")
        ambassador.unpublishObjectClassDirectedInteractions(object_class, directed_set)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unpublish_object_directed")
        ambassador.unsubscribeObjectClassDirectedInteractions(object_class)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unsubscribe_object_directed")
        ambassador.publishInteractionClass(interaction)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "publish_interaction")
        ambassador.subscribeInteractionClass(interaction, active=False)
        self.assertEqual(self.runtime.ambassador.last_declaration_call, ("subscribe_interaction", interaction.encodedValue, False))
        ambassador.unpublishInteractionClass(interaction)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unpublish_interaction")
        ambassador.unsubscribeInteractionClass(interaction)
        self.assertEqual(self.runtime.ambassador.last_declaration_call[0], "unsubscribe_interaction")
        with self.assertRaises(TypeError):
            ambassador.publishObjectClassAttributes(object_class, frozenset())  # type: ignore[arg-type]
        ambassador.disconnect()

    def test_java_provider_adapts_named_and_generated_object_registration(self) -> None:
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(_RecordingFederateAmbassador(), CallbackModel.HLA_EVOKED)
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        ambassador.reserveObjectInstanceName("named-server")
        named = ambassador.registerObjectInstance(
            object_class, objectInstanceName="named-server"
        )
        generated = ambassador.registerObjectInstance(object_class)

        self.assertIsInstance(named, ObjectInstanceHandle)
        self.assertIsInstance(generated, ObjectInstanceHandle)
        self.assertEqual(ambassador.getObjectInstanceName(named), "named-server")
        self.assertEqual(ambassador.getObjectInstanceHandle("named-server"), named)
        self.assertEqual(ambassador.getObjectInstanceName(generated), "generated-2")
        ambassador.localDeleteObjectInstance(generated)
        self.assertEqual(self.runtime.ambassador.last_local_delete, generated.encodedValue)
        ambassador.deleteObjectInstance(named, b"\x00deleted\xff")
        self.assertEqual(
            self.runtime.ambassador.last_object_instance_deletion,
            (named.encodedValue, b"\x00deleted\xff"),
        )
        ambassador.updateAttributeValues(
            generated,
            AttributeHandleValueMap(
                {ambassador.getAttributeHandle(object_class, "Efficiency"): b"updated"}
            ),
            b"update-tag",
        )
        self.assertEqual(
            self.runtime.ambassador.last_attribute_update,
            (
                generated.encodedValue,
                {b"attribute:HLAobjectRoot.Employee.Server:Efficiency": b"updated"},
                b"update-tag",
            ),
        )
        interaction = ambassador.getInteractionClassHandle(
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"
        )
        ambassador.sendInteraction(
            interaction,
            ParameterHandleValueMap(
                {ambassador.getParameterHandle(interaction, "TemperatureOk"): b"served"}
            ),
            b"interaction-tag",
        )
        self.assertEqual(
            self.runtime.ambassador.last_interaction,
            (
                interaction.encodedValue,
                {
                    b"parameter:HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed:TemperatureOk": b"served"
                },
                b"interaction-tag",
            ),
        )
        directed = ambassador.getInteractionClassHandle("HLAinteractionRoot.ServerAction.TakeOrder")
        callbacks = _RecordingFederateAmbassador()
        ambassador.disconnect()
        ambassador = self.factory.getRtiAmbassador()
        ambassador.connect(callbacks, CallbackModel.HLA_IMMEDIATE)
        batch_names = ObjectInstanceNameSet(["batch-one", "batch-two"])
        ambassador.reserveMultipleObjectInstanceNames(batch_names)
        self.assertEqual(self.runtime.ambassador.last_name_batch, frozenset(batch_names))
        self.assertEqual(callbacks.name_batch_successes[-1], batch_names)
        ambassador.releaseMultipleObjectInstanceNames(batch_names)
        self.assertEqual(self.runtime.ambassador.last_released_name_batch, frozenset(batch_names))
        object_class = ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server")
        instance = ambassador.registerObjectInstance(object_class)
        ambassador.sendDirectedInteraction(directed, instance, ParameterHandleValueMap(), b"directed")
        self.assertEqual(callbacks.directed_interactions[-1][0], directed)
        self.assertEqual(callbacks.directed_interactions[-1][1], instance)
        directed_retraction = ambassador.sendDirectedInteractionWithTime(
            directed,
            instance,
            ParameterHandleValueMap(),
            ambassador.getTimeFactory().makeLogicalTime(9),
            b"timed-directed",
        )
        self.assertIsInstance(directed_retraction, MessageRetractionHandle)
        ambassador.reserveObjectInstanceName("unused-server")
        ambassador.releaseObjectInstanceName("unused-server")
        with self.assertRaises(TypeError):
            ambassador.getObjectInstanceName(object_class)  # type: ignore[arg-type]
        ambassador.disconnect()

    def test_java_encoder_factory_adapts_the_shared_data_element_contract(self) -> None:
        encoder = self.factory.getEncoderFactory()
        integer = encoder.createHLAinteger32BE(-2)
        unsigned = encoder.createHLAunsignedInteger32BE(0x1234ABCD)
        unsigned_maximum = encoder.createHLAunsignedInteger32BE(0xFFFFFFFF)
        boolean = encoder.createHLAboolean(True)
        unicode = encoder.createHLAunicodeString("A😀")

        self.assertEqual(integer.toByteArray(), b"\xff\xff\xff\xfe")
        self.assertEqual(unsigned.toByteArray(), b"\x124\xab\xcd")
        self.assertEqual(unsigned_maximum.toByteArray(), b"\xff\xff\xff\xff")
        self.assertEqual(unsigned_maximum.getValue(), 0xFFFFFFFF)
        self.assertEqual(boolean.toByteArray(), b"\x00\x00\x00\x01")
        self.assertEqual(unicode.toByteArray(), b"\x00\x00\x00\x03\x00A\xd8=\xde\x00")
        self.assertIs(integer.decode(b"\x00\x00\x00\x07"), integer)
        self.assertEqual(integer.getValue(), 7)
        self.assertIs(unicode.setValue("reset"), unicode)
        self.assertEqual(unicode.getValue(), "reset")

        self.assertIs(self.factory.unwrap_java_factory(), self.runtime.factory)
        self.assertIs(self.factory.unwrap_java_encoder_factory(), self.runtime.factory.encoder_factory)

    def test_environment_configuration_does_not_reuse_the_standard_factory_name(self) -> None:
        configuration = JavaProviderConfiguration.from_environment(
            {
                "UMBRA_JAVA_RTI_CLASSPATH": os.pathsep.join(("first.jar", "second.jar")),
                "UMBRA_JAVA_RTI_FACTORY_NAME": "Vendor RTI",
                "UMBRA_JAVA_RTI_JVM_PATH": "java.dll",
                "HLA_RTI_FACTORY_NAME": "must-not-be-read-here",
            }
        )

        self.assertEqual(configuration.classpath, ("first.jar", "second.jar"))
        self.assertEqual(configuration.rti_factory_name, "Vendor RTI")
        self.assertEqual(configuration.jvm_path, "java.dll")
