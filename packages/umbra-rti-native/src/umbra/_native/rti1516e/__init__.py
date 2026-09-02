"""Direct C++ IEEE 1516.1-2010 provider entry point.

The extension is deliberately edition-specific: it binds ``rti1516e`` and
returns objects from the provider-neutral ``hla.rti1516e`` contract. The
current native implementation contains a connection lifecycle slice, a
provider-owned federation/declaration/object/interaction/ownership/
synchronization callback slice, the official scalar/composite encoding
families, and both standard reference logical-time representations. The
module's primary contract is surface/factory bindability; unsupported RTI/MOM
services remain explicit rather than being silently routed to the 2025
provider.
"""

from __future__ import annotations

from abc import update_abstractmethods
from typing import Any, Iterable, Mapping

from hla.rti1516e import (
    AttributeHandleFactory,
    AttributeHandle,
    AttributeHandleSetFactory,
    AttributeHandleValueMapFactory,
    AttributeHandleValueMap,
    AttributeSetRegionSetPairListFactory,
    AttributeRegionAssociation,
    AttributeSetRegionSetPairList,
    MutableAttributeHandleSet,
    MutableAttributeHandleValueMap,
    MutableDimensionHandleSet,
    MutableFederateHandleSet,
    MutableRegionHandleSet,
    MutableParameterHandleValueMap,
    DimensionHandle,
    DimensionHandleFactory,
    DimensionHandleSetFactory,
    FederateHandleFactory,
    FederateHandleSetFactory,
    InteractionClassHandleFactory,
    ObjectClassHandleFactory,
    ObjectInstanceHandleFactory,
    ParameterHandleFactory,
    ParameterHandleValueMapFactory,
    RegionHandleSetFactory,
    CallbackModel,
    FederateAmbassador,
    FederateHandle,
    FederateHandleSet,
    InteractionClassHandle,
    ObjectClassHandle,
    ObjectInstanceHandle,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RTIAMBASSADOR_METHODS,
    RTIambassador,
    SupplementalReflectInfo,
    SupplementalReceiveInfo,
    TransportationTypeHandle,
    TransportationTypeHandleFactory,
    RtiFactory,
)
from hla.rti1516e import ResignAction
from hla.rti1516e.exceptions import exceptionForName

from . import _native_2010
from .encoding import Native2010EncoderFactory
from .time import Native2010Float64TimeFactory, Native2010TimeFactory


def _call_native(function: Any, *args: object) -> Any:
    try:
        return function(*args)
    except _native_2010.Native2010RtiError as error:
        name, separator, message = str(error).partition(": ")
        raise exceptionForName(
            name, message if separator else name, error
        ) from error


class _Native2010CallbackBridge:
    """Turn the native callback's encoded-handle payloads into 2010 values."""

    def __init__(self, target: FederateAmbassador) -> None:
        self._target = target

    def discoverObjectInstance(
        self,
        object_value: bytes,
        class_value: bytes,
        object_name: str,
        producing_federate: bytes,
    ) -> None:
        self._target.discoverObjectInstance(
            ObjectInstanceHandle(object_value),
            ObjectClassHandle(class_value),
            object_name,
            FederateHandle(producing_federate),
        )

    def reflectAttributeValues(
        self,
        object_value: bytes,
        values: Mapping[bytes, bytes],
        tag: bytes,
        producing_federate: bytes,
    ) -> None:
        decoded = AttributeHandleValueMap(
            (AttributeHandle(handle), bytes(value))
            for handle, value in values.items()
        )
        supplement = SupplementalReflectInfo(
            FederateHandle(producing_federate) if producing_federate else None
        )
        self._target.reflectAttributeValues(
            ObjectInstanceHandle(object_value),
            decoded,
            bytes(tag),
            OrderType.RECEIVE,
            TransportationTypeHandle(b"transport:HLAdefaultReliable"),
            supplement,
        )

    def receiveInteraction(
        self,
        interaction_value: bytes,
        values: Mapping[bytes, bytes],
        tag: bytes,
        producing_federate: bytes,
    ) -> None:
        decoded = ParameterHandleValueMap(
            (ParameterHandle(handle), bytes(value))
            for handle, value in values.items()
        )
        supplement = SupplementalReceiveInfo(
            FederateHandle(producing_federate) if producing_federate else None
        )
        self._target.receiveInteraction(
            InteractionClassHandle(interaction_value),
            decoded,
            bytes(tag),
            OrderType.RECEIVE,
            TransportationTypeHandle(b"transport:HLAdefaultReliable"),
            supplement,
        )

    def informAttributeOwnership(
        self,
        object_value: bytes,
        attribute_value: bytes,
        owner_value: bytes,
    ) -> None:
        self._target.informAttributeOwnership(
            ObjectInstanceHandle(object_value),
            AttributeHandle(attribute_value),
            FederateHandle(owner_value),
        )

    def synchronizationPointRegistrationSucceeded(self, label: str) -> None:
        self._target.synchronizationPointRegistrationSucceeded(str(label))

    def announceSynchronizationPoint(self, label: str, tag: bytes) -> None:
        self._target.announceSynchronizationPoint(str(label), bytes(tag))

    def federationSynchronized(self, label: str, failed_to_sync: Iterable[bytes]) -> None:
        self._target.federationSynchronized(
            str(label),
            FederateHandleSet(FederateHandle(bytes(value)) for value in failed_to_sync),
        )


class _Native2010AttributeHandleSetFactory(AttributeHandleSetFactory):
    def create(self) -> MutableAttributeHandleSet:
        return MutableAttributeHandleSet()


class _Native2010AttributeHandleValueMapFactory(AttributeHandleValueMapFactory):
    def create(self, capacity: int = 0) -> MutableAttributeHandleValueMap:
        del capacity
        return MutableAttributeHandleValueMap()


class _Native2010ParameterHandleValueMapFactory(ParameterHandleValueMapFactory):
    def create(self, capacity: int = 0) -> MutableParameterHandleValueMap:
        del capacity
        return MutableParameterHandleValueMap()


def _decode_handle(handle_type: type[Any], buffer: object, offset: int = 0) -> Any:
    encoded = bytes(buffer)
    if offset < 0 or offset > len(encoded):
        raise ValueError("offset is outside the encoded handle")
    return handle_type(encoded[offset:])


class _Native2010FederateHandleFactory(FederateHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> FederateHandle:
        return _decode_handle(FederateHandle, buffer, offset)


class _Native2010ObjectClassHandleFactory(ObjectClassHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> ObjectClassHandle:
        return _decode_handle(ObjectClassHandle, buffer, offset)


class _Native2010ObjectInstanceHandleFactory(ObjectInstanceHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> ObjectInstanceHandle:
        return _decode_handle(ObjectInstanceHandle, buffer, offset)


class _Native2010AttributeHandleFactory(AttributeHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> AttributeHandle:
        return _decode_handle(AttributeHandle, buffer, offset)


class _Native2010InteractionClassHandleFactory(InteractionClassHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> InteractionClassHandle:
        return _decode_handle(InteractionClassHandle, buffer, offset)


class _Native2010ParameterHandleFactory(ParameterHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> ParameterHandle:
        return _decode_handle(ParameterHandle, buffer, offset)


class _Native2010DimensionHandleFactory(DimensionHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> DimensionHandle:
        return _decode_handle(DimensionHandle, buffer, offset)


class _Native2010TransportationTypeHandleFactory(TransportationTypeHandleFactory):
    def decode(self, buffer: object, offset: int = 0) -> TransportationTypeHandle:
        return _decode_handle(TransportationTypeHandle, buffer, offset)

    def getHLAdefaultReliable(self) -> TransportationTypeHandle:
        return TransportationTypeHandle(b"transport:HLAdefaultReliable")

    def getHLAdefaultBestEffort(self) -> TransportationTypeHandle:
        return TransportationTypeHandle(b"transport:HLAdefaultBestEffort")


class _Native2010DimensionHandleSetFactory(DimensionHandleSetFactory):
    def create(self) -> MutableDimensionHandleSet:
        return MutableDimensionHandleSet()


class _Native2010FederateHandleSetFactory(FederateHandleSetFactory):
    def create(self) -> MutableFederateHandleSet:
        return MutableFederateHandleSet()


class _Native2010RegionHandleSetFactory(RegionHandleSetFactory):
    def create(self) -> MutableRegionHandleSet:
        return MutableRegionHandleSet()


class _Native2010AttributeSetRegionSetPairListFactory(AttributeSetRegionSetPairListFactory):
    def create(self, capacity: int = 0) -> AttributeSetRegionSetPairList:
        del capacity
        return AttributeSetRegionSetPairList()


class Native2010RTIambassador(RTIambassador):
    """Python façade over the bounded native 2010 C++ ambassador."""

    def __init__(self, implementation: Any | None = None) -> None:
        self._implementation = implementation or _native_2010.Native2010Ambassador()
        self._federate_ambassador: FederateAmbassador | None = None

    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        localSettingsDesignator: str = "",
    ) -> None:
        if not isinstance(federateAmbassador, FederateAmbassador):
            raise TypeError("federateAmbassador must be FederateAmbassador")
        if not isinstance(callbackModel, CallbackModel):
            raise TypeError("callbackModel must be CallbackModel")
        bridge = _Native2010CallbackBridge(federateAmbassador)
        _call_native(
            self._implementation.connect,
            bridge,
            callbackModel.name,
            str(localSettingsDesignator),
        )
        self._federate_ambassador = federateAmbassador

    def disconnect(self) -> None:
        _call_native(self._implementation.disconnect)
        self._federate_ambassador = None

    def getTimeFactory(self) -> Native2010TimeFactory:
        return Native2010TimeFactory()

    def getFloat64TimeFactory(self) -> Native2010Float64TimeFactory:
        """Return the explicit HLAfloat64Time reference factory."""

        return Native2010Float64TimeFactory()

    def createFederationExecution(
        self, federationExecutionName: str, fomModule: object, logicalTimeImplementationName: str = ""
    ) -> None:
        del logicalTimeImplementationName
        _call_native(
            self._implementation.create_federation_execution,
            str(federationExecutionName),
        )

    def destroyFederationExecution(self, federationExecutionName: str) -> None:
        _call_native(self._implementation.destroy_federation_execution, str(federationExecutionName))

    def registerFederationSynchronizationPoint(self, label: str, tag: bytes, *args: object) -> None:
        if args:
            raise NotImplementedError(
                "IEEE 1516.1-2010 native reference slice does not yet support an explicit synchronization set"
            )
        _call_native(
            self._implementation.register_federation_synchronization_point,
            str(label),
            bytes(tag),
        )

    def synchronizationPointAchieved(self, label: str, successfully: bool = True) -> None:
        _call_native(
            self._implementation.synchronization_point_achieved,
            str(label),
            bool(successfully),
        )

    def joinFederationExecution(self, *args: object) -> FederateHandle:
        if len(args) == 2:
            federate_name = ""
            federate_type, federation = args
        elif len(args) >= 3:
            federate_name, federate_type, federation = args[:3]
        else:
            raise TypeError("joinFederationExecution expects 2 or 3 arguments")
        encoded = _call_native(
            self._implementation.join_federation_execution,
            str(federate_name),
            str(federate_type),
            str(federation),
        )
        return FederateHandle(bytes(encoded))

    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        if not isinstance(resignAction, ResignAction):
            raise TypeError("resignAction must be ResignAction")
        _call_native(self._implementation.resign_federation_execution, resignAction.name)

    def getFederateHandle(self, theName: str) -> FederateHandle:
        return FederateHandle(bytes(_call_native(
            self._implementation.get_federate_handle,
            str(theName),
        )))

    def getFederateName(self, theHandle: FederateHandle) -> str:
        return str(_call_native(
            self._implementation.get_federate_name,
            theHandle.encodedValue,
        ))

    def getObjectClassHandle(self, theName: str) -> ObjectClassHandle:
        return ObjectClassHandle(
            bytes(_call_native(self._implementation.get_object_class_handle, str(theName)))
        )

    def getObjectClassName(self, theHandle: ObjectClassHandle) -> str:
        return str(_call_native(self._implementation.get_object_class_name, theHandle.encodedValue))

    def getAttributeHandle(self, whichClass: ObjectClassHandle, theAttributeName: str) -> AttributeHandle:
        return AttributeHandle(
            bytes(_call_native(
                self._implementation.get_attribute_handle,
                whichClass.encodedValue,
                str(theAttributeName),
            ))
        )

    def getAttributeName(self, whichClass: ObjectClassHandle, theHandle: AttributeHandle) -> str:
        return str(_call_native(
            self._implementation.get_attribute_name,
            whichClass.encodedValue,
            theHandle.encodedValue,
        ))

    def getInteractionClassHandle(self, theName: str) -> InteractionClassHandle:
        return InteractionClassHandle(
            bytes(_call_native(self._implementation.get_interaction_class_handle, str(theName)))
        )

    def getInteractionClassName(self, theHandle: InteractionClassHandle) -> str:
        return str(_call_native(
            self._implementation.get_interaction_class_name,
            theHandle.encodedValue,
        ))

    def getParameterHandle(
        self, whichClass: InteractionClassHandle, theName: str
    ) -> ParameterHandle:
        return ParameterHandle(bytes(_call_native(
            self._implementation.get_parameter_handle,
            whichClass.encodedValue,
            str(theName),
        )))

    def getParameterName(
        self, whichClass: InteractionClassHandle, theHandle: ParameterHandle
    ) -> str:
        return str(_call_native(
            self._implementation.get_parameter_name,
            whichClass.encodedValue,
            theHandle.encodedValue,
        ))

    def publishObjectClassAttributes(self, theClass: ObjectClassHandle, attributeList: Iterable[AttributeHandle]) -> None:
        _call_native(
            self._implementation.publish_object_class_attributes,
            theClass.encodedValue,
            {attribute.encodedValue for attribute in attributeList},
        )

    def unpublishObjectClass(self, theClass: ObjectClassHandle) -> None:
        _call_native(self._implementation.unpublish_object_class_attributes, theClass.encodedValue, set())

    def unpublishObjectClassAttributes(self, theClass: ObjectClassHandle, attributeList: Iterable[AttributeHandle]) -> None:
        _call_native(
            self._implementation.unpublish_object_class_attributes,
            theClass.encodedValue,
            {attribute.encodedValue for attribute in attributeList},
        )

    def subscribeObjectClassAttributes(
        self,
        theClass: ObjectClassHandle,
        attributeList: Iterable[AttributeHandle],
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        del active, updateRateDesignator
        _call_native(
            self._implementation.subscribe_object_class_attributes,
            theClass.encodedValue,
            {attribute.encodedValue for attribute in attributeList},
        )

    def unsubscribeObjectClass(self, theClass: ObjectClassHandle) -> None:
        _call_native(self._implementation.unsubscribe_object_class_attributes, theClass.encodedValue, set())

    def unsubscribeObjectClassAttributes(self, theClass: ObjectClassHandle, attributeList: Iterable[AttributeHandle]) -> None:
        _call_native(
            self._implementation.unsubscribe_object_class_attributes,
            theClass.encodedValue,
            {attribute.encodedValue for attribute in attributeList},
        )

    def publishInteractionClass(self, theInteraction: InteractionClassHandle) -> None:
        _call_native(
            self._implementation.publish_interaction_class,
            theInteraction.encodedValue,
        )

    def unpublishInteractionClass(self, theInteraction: InteractionClassHandle) -> None:
        _call_native(
            self._implementation.unpublish_interaction_class,
            theInteraction.encodedValue,
        )

    def subscribeInteractionClass(self, theClass: InteractionClassHandle) -> None:
        _call_native(
            self._implementation.subscribe_interaction_class,
            theClass.encodedValue,
        )

    def unsubscribeInteractionClass(self, theClass: InteractionClassHandle) -> None:
        _call_native(
            self._implementation.unsubscribe_interaction_class,
            theClass.encodedValue,
        )

    def registerObjectInstance(
        self, theClass: ObjectClassHandle, theObjectInstanceName: str | None = None
    ) -> ObjectInstanceHandle:
        encoded = _call_native(
            self._implementation.register_object_instance,
            theClass.encodedValue,
            theObjectInstanceName,
        )
        return ObjectInstanceHandle(bytes(encoded))

    def getObjectInstanceName(self, theHandle: ObjectInstanceHandle) -> str:
        return str(_call_native(self._implementation.get_object_instance_name, theHandle.encodedValue))

    def updateAttributeValues(
        self,
        theObject: ObjectInstanceHandle,
        theAttributeValues: Mapping[AttributeHandle, bytes],
        theUserSuppliedTag: bytes,
    ) -> None:
        _call_native(
            self._implementation.update_attribute_values,
            theObject.encodedValue,
            {handle.encodedValue: bytes(value) for handle, value in theAttributeValues.items()},
            bytes(theUserSuppliedTag),
        )

    def sendInteraction(
        self,
        theInteraction: InteractionClassHandle,
        theParameterValues: Mapping[ParameterHandle, bytes],
        theUserSuppliedTag: bytes,
    ) -> None:
        _call_native(
            self._implementation.send_interaction,
            theInteraction.encodedValue,
            {
                handle.encodedValue: bytes(value)
                for handle, value in theParameterValues.items()
            },
            bytes(theUserSuppliedTag),
        )

    def queryAttributeOwnership(
        self,
        theObject: ObjectInstanceHandle,
        theAttribute: AttributeHandle,
    ) -> None:
        _call_native(
            self._implementation.query_attribute_ownership,
            theObject.encodedValue,
            theAttribute.encodedValue,
        )

    def isAttributeOwnedByFederate(
        self,
        theObject: ObjectInstanceHandle,
        theAttribute: AttributeHandle,
    ) -> bool:
        return bool(_call_native(
            self._implementation.is_attribute_owned_by_federate,
            theObject.encodedValue,
            theAttribute.encodedValue,
        ))

    def getAttributeHandleSetFactory(self) -> AttributeHandleSetFactory:
        return _Native2010AttributeHandleSetFactory()

    def getAttributeHandleValueMapFactory(self) -> AttributeHandleValueMapFactory:
        return _Native2010AttributeHandleValueMapFactory()

    def getParameterHandleValueMapFactory(self) -> ParameterHandleValueMapFactory:
        return _Native2010ParameterHandleValueMapFactory()

    def getFederateHandleFactory(self) -> FederateHandleFactory:
        return _Native2010FederateHandleFactory()

    def getObjectClassHandleFactory(self) -> ObjectClassHandleFactory:
        return _Native2010ObjectClassHandleFactory()

    def getObjectInstanceHandleFactory(self) -> ObjectInstanceHandleFactory:
        return _Native2010ObjectInstanceHandleFactory()

    def getAttributeHandleFactory(self) -> AttributeHandleFactory:
        return _Native2010AttributeHandleFactory()

    def getInteractionClassHandleFactory(self) -> InteractionClassHandleFactory:
        return _Native2010InteractionClassHandleFactory()

    def getParameterHandleFactory(self) -> ParameterHandleFactory:
        return _Native2010ParameterHandleFactory()

    def getDimensionHandleFactory(self) -> DimensionHandleFactory:
        return _Native2010DimensionHandleFactory()

    def getDimensionHandleSetFactory(self) -> DimensionHandleSetFactory:
        return _Native2010DimensionHandleSetFactory()

    def getFederateHandleSetFactory(self) -> FederateHandleSetFactory:
        return _Native2010FederateHandleSetFactory()

    def getRegionHandleSetFactory(self) -> RegionHandleSetFactory:
        return _Native2010RegionHandleSetFactory()

    def getAttributeSetRegionSetPairListFactory(self) -> AttributeSetRegionSetPairListFactory:
        return _Native2010AttributeSetRegionSetPairListFactory()

    def getTransportationTypeHandleFactory(self) -> TransportationTypeHandleFactory:
        return _Native2010TransportationTypeHandleFactory()

    def getHLAversion(self) -> str:
        return "IEEE 1516.1-2010"


def _unsupported(name: str):
    def invoke(self: Native2010RTIambassador, *args: object, **kwargs: object) -> object:
        raise NotImplementedError(
            f"IEEE 1516.1-2010 native provider has not implemented {name}"
        )

    invoke.__name__ = name
    invoke.__qualname__ = f"Native2010RTIambassador.{name}"
    return invoke


for _name in RTIAMBASSADOR_METHODS:
    if _name not in Native2010RTIambassador.__dict__:
        setattr(Native2010RTIambassador, _name, _unsupported(_name))
update_abstractmethods(Native2010RTIambassador)


class Native2010RtiFactory(RtiFactory):
    """Discovered provider for the direct native 2010 route."""

    def getRtiAmbassador(self) -> Native2010RTIambassador:
        return Native2010RTIambassador()

    def getEncoderFactory(self) -> object:
        return Native2010EncoderFactory()

    def rtiName(self) -> str:
        return str(_native_2010.rti_name())

    def rtiVersion(self) -> str:
        return str(_native_2010.rti_version())


__all__ = [
    "Native2010RTIambassador",
    "Native2010RtiFactory",
    "Native2010EncoderFactory",
    "Native2010TimeFactory",
    "Native2010Float64TimeFactory",
]
