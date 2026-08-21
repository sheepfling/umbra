"""JPype-specific mechanics kept behind the Java provider boundary."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from threading import Lock
from typing import Any, Callable, Protocol

from hla.rti1516_2025 import (
    CallbackModel,
    FederateAmbassador,
    FederationExecutionInformation,
    FederationExecutionInformationSet,
    FederationExecutionMemberInformation,
    FederationExecutionMemberInformationSet,
    FederateHandle,
    FederateHandleSet,
    FederateHandleSaveStatusPair,
    FederateRestoreStatus,
    AttributeHandle,
    AttributeHandleSet,
    AttributeHandleValueMap,
    RestoreFailureReason,
    RestoreStatus,
    SaveStatus,
    SaveFailureReason,
    InteractionClassHandle,
    InteractionClassHandleSet,
    ObjectClassHandle,
    ObjectInstanceHandle,
    ObjectInstanceNameSet,
    HLAfloat64Interval,
    HLAfloat64Time,
    HLAinteger64Interval,
    HLAinteger64Time,
    LogicalTime,
    LogicalTimeInterval,
    MessageRetractionHandle,
    OrderType,
    ParameterHandle,
    ParameterHandleValueMap,
    RegionHandle,
    RegionHandleSet,
    SynchronizationPointFailureReason,
    TransportationTypeHandle,
    RtiConfiguration,
)
from hla.rti1516_2025.auth import Credentials, HLAnoCredentials
from hla.rti1516_2025.exceptions import RTIinternalError

from .config import JavaProviderConfiguration


_JVM_CONFIGURATION_LOCK = Lock()
_STARTED_JVM_CONFIGURATION: JavaProviderConfiguration | None = None


def _java_time_value(value: object) -> object:
    """Read an exact floating value when a Java provider exposes one."""

    exact = getattr(value, "getTimeValue", None)
    if exact is not None:
        return exact()
    legacy = getattr(value, "getTime", None)
    if legacy is not None:
        return legacy()
    standard = getattr(value, "getValue", None)
    return standard() if standard is not None else None


def _java_time_implementation_name(value: object, numeric: object) -> str:
    """Identify one of the standard time carriers without vendor extensions.

    The compact test fixture retains the older ``implementationName`` helper,
    but IEEE 1516.1-2025's ``LogicalTime`` interface deliberately does not.
    Its two standard time implementations expose distinct primitive ``getValue``
    return types, which JPype presents as ``int`` and ``float`` respectively.
    """

    legacy = getattr(value, "implementationName", None)
    if callable(legacy):
        return str(legacy())
    return "HLAfloat64Time" if isinstance(numeric, float) else "HLAinteger64Time"


@dataclass(frozen=True, slots=True)
class JavaCallbackBinding:
    """Keep both the Java proxy and its Python callback target alive."""

    proxy: object
    target: object


class JavaRuntime(Protocol):
    """Small seam that permits adapter tests without a JVM or vendor JAR."""

    def get_rti_factory(self, configuration: JavaProviderConfiguration) -> object:
        """Return the selected Java ``RtiFactory`` object."""

    def callback_model(self, callback_model: CallbackModel) -> object:
        """Return the matching Java ``CallbackModel`` enum member."""

    def bind_federate_ambassador(
        self,
        federate_ambassador: FederateAmbassador,
    ) -> JavaCallbackBinding:
        """Implement Java's callback interface with a retained Python proxy."""

    def rti_configuration(self, configuration: RtiConfiguration) -> object:
        """Create the matching Java configuration value."""

    def credentials(self, credentials: Credentials) -> object:
        """Create the matching Java credentials value."""

    def exception_name(self, error: BaseException) -> str | None:
        """Return the simple Java exception type name, when available."""

    def federate_handle_bytes(self, handle: object) -> bytes:
        """Copy a Java ``FederateHandle`` into its standard encoded form."""

    def handle_bytes(self, handle: object) -> bytes:
        """Copy any standard Java handle into its opaque encoded form."""

    def decode_handle(
        self,
        ambassador: object,
        factory_method_name: str,
        encoded_value: bytes,
    ) -> object:
        """Rebuild a Java handle through its standard per-handle factory."""

    def cast_handle(self, handle: object, interface_name: str) -> object:
        """Pin an opaque Java handle to one standard interface for overloads."""

    def attribute_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        """Build a Java ``AttributeHandleSet`` through its standard factory."""

    def interaction_class_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        """Build a Java interaction-class set for directed interaction services."""

    def object_instance_name_set(self, encoded_values: tuple[str, ...]) -> object:
        """Build a Java ``Set<String>`` for batch object-name services."""

    def federate_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        """Build a Java ``FederateHandleSet`` through its standard factory."""

    def fom_module_url(self, value: str) -> object:
        """Convert a Python FOM designator into its standard Java string form."""

    def fom_module_urls(self, values: tuple[str, ...]) -> object:
        """Convert Python FOM designators into the standard Java ``String[]``."""

    def attribute_handle_value_map(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[bytes, bytes], ...],
    ) -> object:
        """Build a Java ``AttributeHandleValueMap`` through its standard factory."""

    def parameter_handle_value_map(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[bytes, bytes], ...],
    ) -> object:
        """Build a Java ``ParameterHandleValueMap`` through its standard factory."""

    def attribute_set_region_set_pair_list(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[tuple[bytes, ...], tuple[bytes, ...]], ...],
    ) -> object:
        """Build Java attribute-set/region-set pairs through the selected value types."""

    def logical_time_factory(self, ambassador: object) -> object:
        """Return the selected Java logical-time factory."""

    def decode_logical_time(self, ambassador: object, encoded_value: bytes) -> object:
        """Decode a logical time through the selected Java factory."""

    def decode_logical_interval(self, ambassador: object, encoded_value: bytes) -> object:
        """Decode an interval through the selected Java factory."""

    def dimension_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        """Build a Java DimensionHandleSet through the selected RTI factory."""

    def region_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        """Build a Java RegionHandleSet through the selected RTI factory."""

    def range_bounds(self, lower_bound: int, upper_bound: int) -> object:
        """Build a Java RangeBounds value."""

    def resign_action(self, resign_action_name: str) -> object:
        """Return the matching Java ``ResignAction`` enum member."""

    def order_type(self, order_type_name: str) -> object:
        """Return the matching Java ``OrderType`` enum member."""

    def service_group(self, service_group_name: str) -> object:
        """Return the matching Java ``ServiceGroup`` enum member."""

    def byte_array(self, value: bytes) -> object:
        """Create a Java byte array without exposing JPype publicly."""

    def data_element_factory(self, factory: object) -> object:
        """Create a Java ``DataElementFactory`` proxy for a Python factory."""

    def data_element_array(self, values: tuple[object, ...]) -> object:
        """Create a Java ``DataElement[]`` for encoder varargs."""


class _FederateAmbassadorCallback:
    """The implemented callback subset presented to Java through ``JProxy``."""

    def __init__(
        self,
        target: FederateAmbassador,
        federate_handle_bytes: Callable[[object], bytes],
    ) -> None:
        self._target = target
        self._federate_handle_bytes = federate_handle_bytes

    def connectionLost(self, fault_description: object) -> None:
        self._target.connectionLost(str(fault_description))

    def startRegistrationForObjectClass(self, object_class: object) -> None:
        self._target.startRegistrationForObjectClass(
            ObjectClassHandle(self._federate_handle_bytes(object_class))
        )

    def stopRegistrationForObjectClass(self, object_class: object) -> None:
        self._target.stopRegistrationForObjectClass(
            ObjectClassHandle(self._federate_handle_bytes(object_class))
        )

    def turnInteractionsOn(self, interaction_class: object) -> None:
        self._target.turnInteractionsOn(
            InteractionClassHandle(self._federate_handle_bytes(interaction_class))
        )

    def turnInteractionsOff(self, interaction_class: object) -> None:
        self._target.turnInteractionsOff(
            InteractionClassHandle(self._federate_handle_bytes(interaction_class))
        )

    def discoverObjectInstance(
        self,
        object_instance: object,
        object_class: object,
        object_instance_name: object,
        producing_federate: object,
    ) -> None:
        self._target.discoverObjectInstance(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            ObjectClassHandle(self._federate_handle_bytes(object_class)),
            str(object_instance_name),
            FederateHandle(self._federate_handle_bytes(producing_federate)),
        )

    def objectInstanceNameReservationSucceeded(self, object_instance_name: object) -> None:
        self._target.objectInstanceNameReservationSucceeded(str(object_instance_name))

    def objectInstanceNameReservationFailed(self, object_instance_name: object) -> None:
        self._target.objectInstanceNameReservationFailed(str(object_instance_name))

    def multipleObjectInstanceNameReservationSucceeded(self, object_instance_names: object) -> None:
        self._target.multipleObjectInstanceNameReservationSucceeded(
            ObjectInstanceNameSet(str(name) for name in object_instance_names)
        )

    def multipleObjectInstanceNameReservationFailed(self, object_instance_names: object) -> None:
        self._target.multipleObjectInstanceNameReservationFailed(
            ObjectInstanceNameSet(str(name) for name in object_instance_names)
        )

    def provideAttributeValueUpdate(
        self, object_instance: object, attributes: object, user_supplied_tag: object
    ) -> None:
        self._target.provideAttributeValueUpdate(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
            bytes(int(value) & 0xFF for value in user_supplied_tag),  # type: ignore[union-attr]
        )

    def attributesInScope(self, object_instance: object, attributes: object) -> None:
        self._target.attributesInScope(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
        )

    def attributesOutOfScope(self, object_instance: object, attributes: object) -> None:
        self._target.attributesOutOfScope(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
        )

    def turnUpdatesOnForObjectInstance(
        self, object_instance: object, attributes: object, update_rate_designator: object = None
    ) -> None:
        arguments = (
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
        )
        if update_rate_designator is not None:
            arguments += (str(update_rate_designator),)
        self._target.turnUpdatesOnForObjectInstance(*arguments)

    def turnUpdatesOffForObjectInstance(self, object_instance: object, attributes: object) -> None:
        self._target.turnUpdatesOffForObjectInstance(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
        )

    def confirmAttributeTransportationTypeChange(
        self, object_instance: object, attributes: object, transportation_type: object
    ) -> None:
        self._target.confirmAttributeTransportationTypeChange(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
        )

    def reportAttributeTransportationType(
        self, object_instance: object, attribute: object, transportation_type: object
    ) -> None:
        self._target.reportAttributeTransportationType(
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            AttributeHandle(self._federate_handle_bytes(attribute)),
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
        )

    def confirmInteractionTransportationTypeChange(
        self, interaction_class: object, transportation_type: object
    ) -> None:
        self._target.confirmInteractionTransportationTypeChange(
            InteractionClassHandle(self._federate_handle_bytes(interaction_class)),
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
        )

    def reportInteractionTransportationType(
        self, federate: object, interaction_class: object, transportation_type: object
    ) -> None:
        self._target.reportInteractionTransportationType(
            FederateHandle(self._federate_handle_bytes(federate)),
            InteractionClassHandle(self._federate_handle_bytes(interaction_class)),
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
        )

    def removeObjectInstance(
        self,
        object_instance: object,
        user_supplied_tag: object,
        producing_federate: object,
        time: object = None,
        sent_order_type: object = None,
        received_order_type: object = None,
        optional_retraction: object = None,
    ) -> None:
        arguments = (
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            bytes(user_supplied_tag),  # type: ignore[arg-type]
            FederateHandle(self._federate_handle_bytes(producing_federate)),
        )
        self._invoke_callback(
            "removeObjectInstance",
            arguments,
            time,
            sent_order_type,
            received_order_type,
            optional_retraction,
        )

    def reflectAttributeValues(
        self,
        object_instance: object,
        attribute_values: object,
        user_supplied_tag: object,
        transportation_type: object,
        producing_federate: object,
        optional_sent_regions: object,
        time: object = None,
        sent_order_type: object = None,
        received_order_type: object = None,
        optional_retraction: object = None,
    ) -> None:
        arguments = (
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            AttributeHandleValueMap(
                (
                    AttributeHandle(self._federate_handle_bytes(entry.getKey())),
                    bytes(int(value) & 0xFF for value in entry.getValue()),
                )
                for entry in attribute_values.entrySet()  # type: ignore[union-attr]
            ),
            bytes(int(value) & 0xFF for value in user_supplied_tag),  # type: ignore[union-attr]
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
            FederateHandle(self._federate_handle_bytes(producing_federate)),
        )
        if optional_sent_regions is not None:
            arguments += (self._region_handle_set(optional_sent_regions),)
        self._invoke_callback(
            "reflectAttributeValues",
            arguments,
            time,
            sent_order_type,
            received_order_type,
            optional_retraction,
            include_region_placeholder=optional_sent_regions is None,
        )

    def receiveInteraction(
        self,
        interaction_class: object,
        parameter_values: object,
        user_supplied_tag: object,
        transportation_type: object,
        producing_federate: object,
        optional_sent_regions: object,
        time: object = None,
        sent_order_type: object = None,
        received_order_type: object = None,
        optional_retraction: object = None,
    ) -> None:
        arguments = (
            InteractionClassHandle(self._federate_handle_bytes(interaction_class)),
            ParameterHandleValueMap(
                (
                    ParameterHandle(self._federate_handle_bytes(entry.getKey())),
                    bytes(int(value) & 0xFF for value in entry.getValue()),
                )
                for entry in parameter_values.entrySet()  # type: ignore[union-attr]
            ),
            bytes(int(value) & 0xFF for value in user_supplied_tag),  # type: ignore[union-attr]
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
            FederateHandle(self._federate_handle_bytes(producing_federate)),
        )
        if optional_sent_regions is not None:
            arguments += (self._region_handle_set(optional_sent_regions),)
        self._invoke_callback(
            "receiveInteraction",
            arguments,
            time,
            sent_order_type,
            received_order_type,
            optional_retraction,
            include_region_placeholder=optional_sent_regions is None,
        )

    def receiveDirectedInteraction(
        self,
        interaction_class: object,
        object_instance: object,
        parameter_values: object,
        user_supplied_tag: object,
        transportation_type: object,
        producing_federate: object,
        time: object = None,
        sent_order_type: object = None,
        received_order_type: object = None,
        optional_retraction: object = None,
    ) -> None:
        arguments = (
            InteractionClassHandle(self._federate_handle_bytes(interaction_class)),
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            ParameterHandleValueMap(
                (
                    ParameterHandle(self._federate_handle_bytes(entry.getKey())),
                    bytes(int(value) & 0xFF for value in entry.getValue()),
                )
                for entry in parameter_values.entrySet()  # type: ignore[union-attr]
            ),
            bytes(int(value) & 0xFF for value in user_supplied_tag),  # type: ignore[union-attr]
            TransportationTypeHandle(self._federate_handle_bytes(transportation_type)),
            FederateHandle(self._federate_handle_bytes(producing_federate)),
        )
        self._invoke_callback(
            "receiveDirectedInteraction",
            arguments,
            time,
            sent_order_type,
            received_order_type,
            optional_retraction,
        )

    def _invoke_callback(
        self,
        callback_name: str,
        arguments: tuple[object, ...],
        time: object,
        sent_order_type: object,
        received_order_type: object,
        optional_retraction: object,
        *,
        include_region_placeholder: bool = False,
    ) -> None:
        if time is None:
            self._target_method(callback_name)(*arguments)
            return
        timed_arguments = arguments
        if include_region_placeholder:
            timed_arguments += (None,)
        timed_arguments += (
            self._logical_time(time),
            self._order_type(sent_order_type),
            self._order_type(received_order_type),
            None
            if optional_retraction is None
            else MessageRetractionHandle(self._federate_handle_bytes(optional_retraction)),
        )
        callback = self._target_method(callback_name)
        try:
            callback(*timed_arguments)
        except TypeError:
            # Preserve source compatibility for applications that implement
            # only the receive-order callback arity.
            callback(*arguments)

    @staticmethod
    def _order_type(value: object) -> OrderType:
        name = getattr(value, "name", None)
        if callable(name):
            name = name()
        if name is None:
            name = str(value).split(":")[-1]
        try:
            return OrderType[str(name).split(".")[-1]]
        except KeyError as error:
            raise RTIinternalError(f"Java RTI returned an unknown OrderType: {name}") from error

    def _region_handle_set(self, value: object) -> RegionHandleSet:
        return RegionHandleSet(
            RegionHandle(self._federate_handle_bytes(handle))
            for handle in value  # type: ignore[union-attr]
        )

    def _attribute_handle_set(self, value: object) -> AttributeHandleSet:
        return AttributeHandleSet(
            AttributeHandle(self._federate_handle_bytes(handle))
            for handle in value  # type: ignore[union-attr]
        )

    def _interaction_class_handle_set(self, value: object) -> InteractionClassHandleSet:
        return InteractionClassHandleSet(
            InteractionClassHandle(self._federate_handle_bytes(handle))
            for handle in value  # type: ignore[union-attr]
        )

    def _ownership_set_callback(
        self,
        callback_name: str,
        object_instance: object,
        attributes: object,
        optional_tag: object = None,
        optional_owner: object = None,
    ) -> None:
        arguments: tuple[object, ...] = (
            ObjectInstanceHandle(self._federate_handle_bytes(object_instance)),
            self._attribute_handle_set(attributes),
        )
        if optional_owner is not None:
            arguments += (FederateHandle(self._federate_handle_bytes(optional_owner)),)
        elif optional_tag is not None:
            arguments += (bytes(int(value) & 0xFF for value in optional_tag),)  # type: ignore[union-attr]
        self._target_method(callback_name)(*arguments)

    def _target_method(self, callback_name: str) -> Callable[..., Any]:
        return getattr(self._target, callback_name)

    def requestAttributeOwnershipAssumption(
        self, object_instance: object, offered_attributes: object, user_supplied_tag: object
    ) -> None:
        self._ownership_set_callback(
            "requestAttributeOwnershipAssumption",
            object_instance,
            offered_attributes,
            user_supplied_tag,
        )

    def requestDivestitureConfirmation(
        self, object_instance: object, released_attributes: object, user_supplied_tag: object
    ) -> None:
        self._ownership_set_callback(
            "requestDivestitureConfirmation",
            object_instance,
            released_attributes,
            user_supplied_tag,
        )

    def attributeOwnershipAcquisitionNotification(
        self, object_instance: object, secured_attributes: object, user_supplied_tag: object
    ) -> None:
        self._ownership_set_callback(
            "attributeOwnershipAcquisitionNotification",
            object_instance,
            secured_attributes,
            user_supplied_tag,
        )

    def attributeOwnershipUnavailable(
        self, object_instance: object, attributes: object, user_supplied_tag: object
    ) -> None:
        self._ownership_set_callback(
            "attributeOwnershipUnavailable", object_instance, attributes, user_supplied_tag
        )

    def requestAttributeOwnershipRelease(
        self, object_instance: object, candidate_attributes: object, user_supplied_tag: object
    ) -> None:
        self._ownership_set_callback(
            "requestAttributeOwnershipRelease",
            object_instance,
            candidate_attributes,
            user_supplied_tag,
        )

    def confirmAttributeOwnershipAcquisitionCancellation(
        self, object_instance: object, attributes: object
    ) -> None:
        self._ownership_set_callback(
            "confirmAttributeOwnershipAcquisitionCancellation", object_instance, attributes
        )

    def informAttributeOwnership(
        self, object_instance: object, attributes: object, owner: object
    ) -> None:
        self._ownership_set_callback(
            "informAttributeOwnership", object_instance, attributes, optional_owner=owner
        )

    def attributeIsNotOwned(self, object_instance: object, attributes: object) -> None:
        self._ownership_set_callback("attributeIsNotOwned", object_instance, attributes)

    def attributeIsOwnedByRTI(self, object_instance: object, attributes: object) -> None:
        self._ownership_set_callback("attributeIsOwnedByRTI", object_instance, attributes)

    def _logical_time(self, value: object) -> LogicalTime:
        numeric = _java_time_value(value)
        implementation = _java_time_implementation_name(value, numeric)
        value_type = HLAinteger64Time if implementation == "HLAinteger64Time" else HLAfloat64Time
        return value_type(
            self._federate_handle_bytes(value),
            implementation,
            bool(value.isInitial()),
            bool(value.isFinal()),
            numeric,
            str(value.toString()),
        )

    def timeRegulationEnabled(self, time: object) -> None:
        self._target.timeRegulationEnabled(self._logical_time(time))

    def timeConstrainedEnabled(self, time: object) -> None:
        self._target.timeConstrainedEnabled(self._logical_time(time))

    def flushQueueGrant(self, time: object, optimistic_time: object) -> None:
        self._target.flushQueueGrant(
            self._logical_time(time), self._logical_time(optimistic_time)
        )

    def timeAdvanceGrant(self, time: object) -> None:
        self._target.timeAdvanceGrant(self._logical_time(time))

    def requestRetraction(self, retraction: object) -> None:
        self._target.requestRetraction(
            MessageRetractionHandle(self._federate_handle_bytes(retraction))
        )

    def reportFederationExecutions(self, report: object) -> None:
        self._target.reportFederationExecutions(
            FederationExecutionInformationSet(
                FederationExecutionInformation(
                    str(getattr(information, "federationExecutionName")),
                    str(getattr(information, "logicalTimeImplementationName")),
                )
                for information in report  # type: ignore[union-attr]
            )
        )

    def reportFederationExecutionMembers(self, federation_name: object, report: object) -> None:
        self._target.reportFederationExecutionMembers(
            str(federation_name),
            FederationExecutionMemberInformationSet(
                FederationExecutionMemberInformation(
                    str(getattr(information, "federateName")),
                    str(getattr(information, "federateType")),
                )
                for information in report  # type: ignore[union-attr]
            ),
        )

    def reportFederationExecutionDoesNotExist(self, federation_name: object) -> None:
        self._target.reportFederationExecutionDoesNotExist(str(federation_name))

    def federateResigned(self, reason: object) -> None:
        self._target.federateResigned(str(reason))

    def federationSynchronized(self, synchronization_point_label: object, failed_to_sync_set: object) -> None:
        self._target.federationSynchronized(
            str(synchronization_point_label),
            FederateHandleSet(
                FederateHandle(self._federate_handle_bytes(handle))
                for handle in failed_to_sync_set  # type: ignore[union-attr]
            ),
        )

    def synchronizationPointRegistrationSucceeded(self, synchronization_point_label: object) -> None:
        self._target.synchronizationPointRegistrationSucceeded(str(synchronization_point_label))

    def announceSynchronizationPoint(
        self, synchronization_point_label: object, user_supplied_tag: object
    ) -> None:
        self._target.announceSynchronizationPoint(
            str(synchronization_point_label),
            bytes(int(value) & 0xFF for value in user_supplied_tag),  # type: ignore[union-attr]
        )

    def synchronizationPointRegistrationFailed(
        self,
        synchronization_point_label: object,
        reason: object,
    ) -> None:
        name = getattr(reason, "name", reason)
        if callable(name):
            name = name()
        self._target.synchronizationPointRegistrationFailed(
            str(synchronization_point_label),
            SynchronizationPointFailureReason[str(name)],
        )

    def federationSaveStatusResponse(self, response: object) -> None:
        records = []
        for pair in response:  # type: ignore[union-attr]
            status = getattr(pair, "status")
            name = getattr(status, "name", status)
            if callable(name):
                name = name()
            records.append(
                FederateHandleSaveStatusPair(
                    FederateHandle(self._federate_handle_bytes(getattr(pair, "handle"))),
                    SaveStatus[str(name)],
                )
            )
        self._target.federationSaveStatusResponse(tuple(records))

    def initiateFederateSave(self, label: object, time: object | None = None) -> None:
        if time is None:
            self._target.initiateFederateSave(str(label))
        else:
            self._target.initiateFederateSave(str(label), self._logical_time(time))

    def federationSaved(self) -> None:
        self._target.federationSaved()

    def federationNotSaved(self, reason: object) -> None:
        name = getattr(reason, "name", reason)
        if callable(name):
            name = name()
        self._target.federationNotSaved(SaveFailureReason[str(name)])

    def federationRestoreStatusResponse(self, response: object) -> None:
        records = []
        for item in response:  # type: ignore[union-attr]
            status = getattr(item, "status")
            name = getattr(status, "name", status)
            if callable(name):
                name = name()
            records.append(
                FederateRestoreStatus(
                    FederateHandle(self._federate_handle_bytes(getattr(item, "preRestoreHandle"))),
                    FederateHandle(self._federate_handle_bytes(getattr(item, "postRestoreHandle"))),
                    RestoreStatus[str(name)],
                )
            )
        self._target.federationRestoreStatusResponse(tuple(records))

    def requestFederationRestoreSucceeded(self, label: object) -> None:
        self._target.requestFederationRestoreSucceeded(str(label))

    def requestFederationRestoreFailed(self, label: object) -> None:
        self._target.requestFederationRestoreFailed(str(label))

    def federationRestoreBegun(self) -> None:
        self._target.federationRestoreBegun()

    def initiateFederateRestore(self, label: object, federate_name: object, handle: object) -> None:
        self._target.initiateFederateRestore(
            str(label), str(federate_name), FederateHandle(self._federate_handle_bytes(handle))
        )

    def federationRestored(self) -> None:
        self._target.federationRestored()

    def federationNotRestored(self, reason: object) -> None:
        name = getattr(reason, "name", reason)
        if callable(name):
            name = name()
        self._target.federationNotRestored(RestoreFailureReason[str(name)])


class JPypeJavaRuntime:
    """Lazy production runtime; importing this module never starts a JVM."""

    def __init__(self) -> None:
        self._jpype: Any | None = None

    def get_rti_factory(self, configuration: JavaProviderConfiguration) -> object:
        jpype = self._ensure_jvm(configuration)
        factory_factory = jpype.JClass("hla.rti1516_2025.RtiFactoryFactory")
        try:
            if configuration.rti_factory_name is not None:
                return factory_factory.getRtiFactory(configuration.rti_factory_name)
            return factory_factory.getRtiFactory()
        except Exception as error:
            raise RTIinternalError(f"Java RtiFactoryFactory failed: {error}") from error

    def callback_model(self, callback_model: CallbackModel) -> object:
        jpype = self._require_started_jvm()
        callback_model_type = jpype.JClass("hla.rti1516_2025.CallbackModel")
        return getattr(callback_model_type, callback_model.name)

    def bind_federate_ambassador(
        self,
        federate_ambassador: FederateAmbassador,
    ) -> JavaCallbackBinding:
        jpype = self._require_started_jvm()
        callback_interface = jpype.JClass("hla.rti1516_2025.FederateAmbassador")
        target = _FederateAmbassadorCallback(federate_ambassador, self.federate_handle_bytes)
        return JavaCallbackBinding(
            proxy=jpype.JProxy(callback_interface, inst=target),
            target=target,
        )

    def rti_configuration(self, configuration: RtiConfiguration) -> object:
        jpype = self._require_started_jvm()
        result = jpype.JClass("hla.rti1516_2025.RtiConfiguration").createConfiguration()
        return result.withConfigurationName(configuration.configurationName()).withRtiAddress(
            configuration.rtiAddress()
        ).withAdditionalSettings(configuration.additionalSettings())

    def credentials(self, credentials: Credentials) -> object:
        jpype = self._require_started_jvm()
        if isinstance(credentials, HLAnoCredentials):
            return jpype.JClass("hla.rti1516_2025.auth.HLAnoCredentials")()
        byte_array = jpype.JArray(jpype.JByte)(credentials.getData())
        return jpype.JClass("hla.rti1516_2025.auth.Credentials")(credentials.getType(), byte_array)

    def exception_name(self, error: BaseException) -> str | None:
        """Return the Java error name, unwrapping proxy checked exceptions."""

        def simple_name(value: object) -> str:
            # JPype normally exposes getSimpleName(), but some external API
            # JAR/proxy combinations return the binary or fully-qualified
            # name.  The adapter's exception registry is keyed by the Java
            # simple name, so normalize both forms at this boundary.
            return str(value).rsplit(".", 1)[-1].rsplit("$", 1)[-1]

        try:
            name = simple_name(error.getClass().getSimpleName())  # type: ignore[attr-defined]
            if name != "UndeclaredThrowableException":
                return name
        except (AttributeError, TypeError):
            pass
        try:
            cause = error.getUndeclaredThrowable()  # type: ignore[attr-defined]
            return simple_name(cause.getClass().getSimpleName())
        except (AttributeError, TypeError):
            return None

    def federate_handle_bytes(self, handle: object) -> bytes:
        return self.handle_bytes(handle)

    def handle_bytes(self, handle: object) -> bytes:
        jpype = self._require_started_jvm()
        buffer = jpype.JArray(jpype.JByte)(int(handle.encodedLength()))
        handle.encode(buffer, 0)
        return bytes(int(value) & 0xFF for value in buffer)

    def decode_handle(
        self,
        ambassador: object,
        factory_method_name: str,
        encoded_value: bytes,
    ) -> object:
        handle_factory = getattr(ambassador, factory_method_name)()
        return handle_factory.decode(self.byte_array(encoded_value), 0)

    def cast_handle(self, handle: object, interface_name: str) -> object:
        jpype = self._require_started_jvm()
        return jpype.JObject(handle, jpype.JClass(interface_name))

    def logical_time_factory(self, ambassador: object) -> object:
        return getattr(ambassador, "getTimeFactory")()

    def decode_logical_time(self, ambassador: object, encoded_value: bytes) -> object:
        factory = self.logical_time_factory(ambassador)
        decoder = getattr(factory, "decodeTime", None)
        if decoder is None:
            decoder = getattr(factory, "decodeLogicalTime")
        return decoder(self.byte_array(encoded_value), 0)

    def decode_logical_interval(self, ambassador: object, encoded_value: bytes) -> object:
        factory = self.logical_time_factory(ambassador)
        decoder = getattr(factory, "decodeInterval", None)
        if decoder is None:
            decoder = getattr(factory, "decodeLogicalTimeInterval")
        return decoder(self.byte_array(encoded_value), 0)

    def dimension_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        handle_set = getattr(ambassador, "getDimensionHandleSetFactory")().create()
        for encoded_value in encoded_values:
            handle_set.add(self.decode_handle(ambassador, "getDimensionHandleFactory", encoded_value))
        return handle_set

    def region_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        handle_set = getattr(ambassador, "getRegionHandleSetFactory")().create()
        for encoded_value in encoded_values:
            handle_set.add(self.decode_handle(ambassador, "getRegionHandleFactory", encoded_value))
        return handle_set

    def range_bounds(self, lower_bound: int, upper_bound: int) -> object:
        return self._require_started_jvm().JClass("hla.rti1516_2025.RangeBounds")(
            int(lower_bound), int(upper_bound)
        )

    def attribute_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        handle_set = getattr(ambassador, "getAttributeHandleSetFactory")().create()
        for encoded_value in encoded_values:
            handle_set.add(
                self.decode_handle(ambassador, "getAttributeHandleFactory", encoded_value)
            )
        return handle_set

    def interaction_class_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        handle_set = getattr(ambassador, "getInteractionClassHandleSetFactory")().create()
        for encoded_value in encoded_values:
            handle_set.add(
                self.decode_handle(ambassador, "getInteractionClassHandleFactory", encoded_value)
            )
        return handle_set

    def object_instance_name_set(self, encoded_values: tuple[str, ...]) -> object:
        name_set = self._require_started_jvm().JClass("java.util.HashSet")()
        for value in encoded_values:
            name_set.add(str(value))
        return name_set

    def federate_handle_set(self, ambassador: object, encoded_values: tuple[bytes, ...]) -> object:
        handle_set = getattr(ambassador, "getFederateHandleSetFactory")().create()
        for encoded_value in encoded_values:
            handle_set.add(self.decode_handle(ambassador, "getFederateHandleFactory", encoded_value))
        return handle_set

    def fom_module_url(self, value: str) -> object:
        return str(value)

    def fom_module_urls(self, values: tuple[str, ...]) -> object:
        jpype = self._require_started_jvm()
        result = jpype.JArray(jpype.JString)(len(values))
        for index, value in enumerate(values):
            result[index] = self.fom_module_url(value)
        return result

    def attribute_handle_value_map(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[bytes, bytes], ...],
    ) -> object:
        value_map = getattr(ambassador, "getAttributeHandleValueMapFactory")().create(0)
        for encoded_handle, value in encoded_values:
            value_map.put(
                self.decode_handle(ambassador, "getAttributeHandleFactory", encoded_handle),
                self.byte_array(value),
            )
        return value_map

    def parameter_handle_value_map(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[bytes, bytes], ...],
    ) -> object:
        value_map = getattr(ambassador, "getParameterHandleValueMapFactory")().create(0)
        for encoded_handle, value in encoded_values:
            value_map.put(
                self.decode_handle(ambassador, "getParameterHandleFactory", encoded_handle),
                self.byte_array(value),
            )
        return value_map

    def attribute_set_region_set_pair_list(
        self,
        ambassador: object,
        encoded_values: tuple[tuple[tuple[bytes, ...], tuple[bytes, ...]], ...],
    ) -> object:
        jpype = self._require_started_jvm()
        pair_list_factory = getattr(
            ambassador, "getAttributeSetRegionSetPairListFactory", None
        )
        if pair_list_factory is None:
            # The compact fixture predates the IEEE factory and exposes a
            # concrete legacy list instead.
            pair_list = jpype.JClass("hla.rti1516_2025.AttributeSetRegionSetPairList")()
            pair_type = jpype.JClass("hla.rti1516_2025.AttributeSetRegionSetPair")
        else:
            pair_list = pair_list_factory().create(len(encoded_values))
            pair_type = jpype.JClass("hla.rti1516_2025.AttributeRegionAssociation")
        for encoded_attributes, encoded_regions in encoded_values:
            pair_list.add(
                pair_type(
                    self.attribute_handle_set(ambassador, encoded_attributes),
                    self.region_handle_set(ambassador, encoded_regions),
                )
            )
        return pair_list

    def resign_action(self, resign_action_name: str) -> object:
        return getattr(self._require_started_jvm().JClass("hla.rti1516_2025.ResignAction"), resign_action_name)

    def order_type(self, order_type_name: str) -> object:
        return getattr(self._require_started_jvm().JClass("hla.rti1516_2025.OrderType"), order_type_name)

    def service_group(self, service_group_name: str) -> object:
        return getattr(
            self._require_started_jvm().JClass("hla.rti1516_2025.ServiceGroup"),
            service_group_name,
        )

    def byte_array(self, value: bytes) -> object:
        jpype = self._require_started_jvm()
        return jpype.JArray(jpype.JByte)(value)

    def data_element_factory(self, factory: object) -> object:
        jpype = self._require_started_jvm()
        interface = jpype.JClass("hla.rti1516_2025.encoding.DataElementFactory")

        class _DataElementFactoryProxy:
            def createElement(self, index: int) -> object:
                element = factory.createElement(int(index))  # type: ignore[attr-defined]
                return getattr(element, "_implementation")

        return jpype.JProxy(interface, inst=_DataElementFactoryProxy())

    def data_element_array(self, values: tuple[object, ...]) -> object:
        jpype = self._require_started_jvm()
        element_type = jpype.JClass("hla.rti1516_2025.encoding.DataElement")
        return jpype.JArray(element_type)(list(values))

    def _ensure_jvm(self, configuration: JavaProviderConfiguration) -> Any:
        global _STARTED_JVM_CONFIGURATION

        jpype = self._load_jpype()
        with _JVM_CONFIGURATION_LOCK:
            if jpype.isJVMStarted():
                if _STARTED_JVM_CONFIGURATION == configuration:
                    return jpype
                if configuration.classpath or configuration.jvm_path or configuration.jvm_options:
                    raise RTIinternalError(
                        "The JVM is already running with a different or unknown Java RTI configuration"
                    )
                return jpype

            arguments = list(configuration.jvm_options)
            keyword_arguments: dict[str, object] = {
                "classpath": list(configuration.classpath),
                "convertStrings": configuration.convert_strings,
            }
            if configuration.jvm_path is not None:
                keyword_arguments["jvmpath"] = configuration.jvm_path
            try:
                jpype.startJVM(*arguments, **keyword_arguments)
            except Exception as error:
                raise RTIinternalError(f"Could not start the JVM for the Java RTI: {error}") from error
            _STARTED_JVM_CONFIGURATION = configuration
        return jpype

    def _require_started_jvm(self) -> Any:
        jpype = self._load_jpype()
        if not jpype.isJVMStarted():
            raise RTIinternalError("The Java RTI JVM has not been started")
        return jpype

    def _load_jpype(self) -> Any:
        if self._jpype is None:
            try:
                import jpype
            except ImportError as error:
                raise RTIinternalError(
                    "Install the optional dependency with 'umbra-rti-jpype[jpype]' to use a Java RTI"
                ) from error
            self._jpype = jpype
        return self._jpype
