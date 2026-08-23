"""Abstract IEEE 1516.1-2025 RTI and provider contracts."""

from __future__ import annotations

from abc import ABC, abstractmethod
from collections.abc import Iterable

from .byte_types import BytesLike, WritableBytes
from .encoding import EncoderFactory
from .values import *


class HandleFactory(ABC):
    """Provider-owned decoder for one opaque handle domain."""

    @abstractmethod
    def decode(self, encodedValue: BytesLike) -> EncodedHandle:
        """Decode and validate an encoded handle through the provider."""


class FederateHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`FederateHandle` values."""


class ObjectClassHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`ObjectClassHandle` values."""


class ObjectInstanceHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`ObjectInstanceHandle` values."""


class AttributeHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`AttributeHandle` values."""


class InteractionClassHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`InteractionClassHandle` values."""


class ParameterHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`ParameterHandle` values."""


class TransportationTypeHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`TransportationTypeHandle` values."""


class DimensionHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`DimensionHandle` values."""


class RegionHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned :class:`RegionHandle` values."""


class MessageRetractionHandleFactory(HandleFactory, ABC):
    """Factory for provider-owned timestamped message-retraction handles."""


class AttributeHandleSetFactory(ABC):
    """Provider-owned factory for mutable attribute-handle sets."""

    @abstractmethod
    def create(self) -> MutableAttributeHandleSet:
        """Create an empty mutable attribute-handle set."""


class DimensionHandleSetFactory(ABC):
    """Provider-owned factory for mutable dimension-handle sets."""

    @abstractmethod
    def create(self) -> MutableDimensionHandleSet:
        """Create an empty mutable dimension-handle set."""


class FederateHandleSetFactory(ABC):
    """Provider-owned factory for mutable federate-handle sets."""

    @abstractmethod
    def create(self) -> MutableFederateHandleSet:
        """Create an empty mutable federate-handle set."""


class RegionHandleSetFactory(ABC):
    """Provider-owned factory for mutable region-handle sets."""

    @abstractmethod
    def create(self) -> MutableRegionHandleSet:
        """Create an empty mutable region-handle set."""


class InteractionClassHandleSetFactory(ABC):
    """Provider-owned factory for mutable interaction-class handle sets."""

    @abstractmethod
    def create(self) -> MutableInteractionClassHandleSet:
        """Create an empty mutable interaction-class handle set."""


class AttributeHandleValueMapFactory(ABC):
    """Provider-owned factory for mutable attribute-value maps."""

    @abstractmethod
    def create(self) -> MutableAttributeHandleValueMap:
        """Create an empty mutable attribute-value map."""


class ParameterHandleValueMapFactory(ABC):
    """Provider-owned factory for mutable parameter-value maps."""

    @abstractmethod
    def create(self) -> MutableParameterHandleValueMap:
        """Create an empty mutable parameter-value map."""


class AttributeSetRegionSetPairListFactory(ABC):
    """Provider-owned factory for Java-shaped attribute/region pair lists."""

    @abstractmethod
    def create(self, capacity: int = 0) -> MutableAttributeSetRegionSetPairList:
        """Create an empty list with the supplied Java capacity hint."""


class LogicalTimeFactory(ABC):
    """Provider-owned factory for the selected federation logical-time type."""

    @abstractmethod
    def implementationName(self) -> str:
        """Return the selected implementation name."""

    @abstractmethod
    def makeInitial(self) -> LogicalTime:
        """Create the provider's initial logical time."""

    @abstractmethod
    def makeFinal(self) -> LogicalTime:
        """Create the provider's final logical time."""

    @abstractmethod
    def makeZero(self) -> LogicalTimeInterval:
        """Create the provider's zero interval."""

    @abstractmethod
    def makeEpsilon(self) -> LogicalTimeInterval:
        """Create the provider's epsilon interval."""

    @abstractmethod
    def decodeLogicalTime(self, encodedValue: BytesLike) -> LogicalTime:
        """Decode an encoded logical time through the provider."""

    @abstractmethod
    def decodeLogicalTimeInterval(self, encodedValue: BytesLike) -> LogicalTimeInterval:
        """Decode an encoded logical-time interval through the provider."""

    @abstractmethod
    def add(self, time: LogicalTime, addend: LogicalTimeInterval) -> LogicalTime:
        """Apply a provider's logical-time addition semantics and return a snapshot."""

    @abstractmethod
    def subtract(self, time: LogicalTime, subtrahend: LogicalTimeInterval) -> LogicalTime:
        """Apply a provider's logical-time subtraction semantics and return a snapshot."""

    @abstractmethod
    def difference(
        self, minuend: LogicalTime, subtrahend: LogicalTime
    ) -> LogicalTimeInterval:
        """Return a provider-computed nonnegative difference between two times."""


class HLAinteger64TimeFactory(LogicalTimeFactory, ABC):
    """Concrete factory contract for the standard integer reference time."""

    @abstractmethod
    def makeLogicalTime(self, value: int) -> HLAinteger64Time:
        """Create an integer reference time through the provider."""

    @abstractmethod
    def makeLogicalTimeInterval(self, value: int) -> HLAinteger64Interval:
        """Create an integer reference interval through the provider."""


class HLAfloat64TimeFactory(LogicalTimeFactory, ABC):
    """Concrete factory contract for the standard floating reference time."""

    @abstractmethod
    def makeLogicalTime(self, value: float) -> HLAfloat64Time:
        """Create a floating reference time through the provider."""

    @abstractmethod
    def makeLogicalTimeInterval(self, value: float) -> HLAfloat64Interval:
        """Create a floating reference interval through the provider."""

class FederateAmbassador(ABC):
    """Receive standard RTI callbacks.

    Java surface: ``hla.rti1516_2025.FederateAmbassador``.
    C++ surface: ``RTI::FederateAmbassador``.
    """

    def connectionLost(self, faultDescription: str) -> None:
        """Report connection loss when the provider supports that service."""

    def reportFederationExecutions(self, report: FederationExecutionInformationSet) -> None:
        """Report the federation executions requested through the RTI."""

    def reportFederationExecutionMembers(
        self,
        federationExecutionName: str,
        report: FederationExecutionMemberInformationSet,
    ) -> None:
        """Report federation members requested through the RTI."""

    def reportFederationExecutionDoesNotExist(self, federationExecutionName: str) -> None:
        """Report that a requested federation execution does not exist."""

    def federateResigned(self, reasonForResignDescription: str) -> None:
        """Report an RTI-originated removal from federation membership."""

    def synchronizationPointRegistrationSucceeded(self, synchronizationPointLabel: str) -> None:
        """Report successful synchronization-point registration."""

    def synchronizationPointRegistrationFailed(
        self,
        synchronizationPointLabel: str,
        reason: SynchronizationPointFailureReason,
    ) -> None:
        """Report registration failure through a typed standard reason."""

    def announceSynchronizationPoint(self, synchronizationPointLabel: str, userSuppliedTag: BytesLike) -> None:
        """Announce a synchronization point and its copied byte tag."""

    def federationSynchronized(
        self,
        synchronizationPointLabel: str,
        failedToSyncSet: FederateHandleSet,
    ) -> None:
        """Report that a synchronization point completed for this federate."""

    def federationSaveStatusResponse(
        self,
        response: tuple[FederateHandleSaveStatusPair, ...],
    ) -> None:
        """Report immutable ordered federation save-status entries."""

    def initiateFederateSave(self, label: str, time: LogicalTime | None = None) -> None:
        """Instruct this federate to begin a scalar or timestamped federation save."""

    def federationSaved(self) -> None:
        """Report successful completion of the current federation save."""

    def federationNotSaved(self, reason: SaveFailureReason) -> None:
        """Report failed or aborted federation save with a typed reason."""

    def federationRestoreStatusResponse(self, response: tuple[FederateRestoreStatus, ...]) -> None:
        """Report immutable ordered federation-restore status entries."""

    def requestFederationRestoreSucceeded(self, label: str) -> None:
        """Report acceptance of a federation restore request."""

    def requestFederationRestoreFailed(self, label: str) -> None:
        """Report that no matching federation restore snapshot is available."""

    def federationRestoreBegun(self) -> None:
        """Report that federation restore has begun."""

    def initiateFederateRestore(
        self, label: str, federateName: str, postRestoreFederateHandle: FederateHandle
    ) -> None:
        """Instruct this federate to restore with its post-restore handle."""

    def federationRestored(self) -> None:
        """Report successful completion of the current federation restore."""

    def federationNotRestored(self, reason: RestoreFailureReason) -> None:
        """Report failed or aborted federation restore with a typed reason."""

    def startRegistrationForObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Advise a publisher that object registration is now useful."""

    def stopRegistrationForObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Advise a publisher that object registration is no longer useful."""

    def turnInteractionsOn(self, interactionClass: InteractionClassHandle) -> None:
        """Advise a publisher that an interaction has an active subscriber."""

    def turnInteractionsOff(self, interactionClass: InteractionClassHandle) -> None:
        """Advise a publisher that an interaction has no active subscribers."""

    def discoverObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        objectClass: ObjectClassHandle,
        objectInstanceName: str,
        producingFederate: FederateHandle,
    ) -> None:
        """Report a newly known object instance with typed source handles."""

    def objectInstanceNameReservationSucceeded(self, objectInstanceName: str) -> None:
        """Report successful reservation of an object-instance name."""

    def objectInstanceNameReservationFailed(self, objectInstanceName: str) -> None:
        """Report that an object-instance name could not be reserved."""

    def multipleObjectInstanceNameReservationSucceeded(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        """Report successful reservation of a batch of object-instance names."""

    def multipleObjectInstanceNameReservationFailed(
        self, objectInstanceNames: ObjectInstanceNameSet
    ) -> None:
        """Report that a batch of object-instance names could not be reserved."""

    def provideAttributeValueUpdate(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike,
    ) -> None:
        """Request current values from the owner of an object instance."""

    def attributesInScope(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report attributes that became relevant for an object instance."""

    def attributesOutOfScope(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report attributes that are no longer relevant for an object instance."""

    def turnUpdatesOnForObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        updateRateDesignator: str | None = None,
    ) -> None:
        """Report that updates are relevant, optionally at a named rate."""

    def turnUpdatesOffForObjectInstance(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report that updates are no longer relevant for an object instance."""

    def confirmAttributeTransportationTypeChange(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Confirm an attribute transportation-type change."""

    def reportAttributeTransportationType(
        self,
        objectInstance: ObjectInstanceHandle,
        attribute: AttributeHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Report the transportation type currently used by an attribute."""

    def confirmInteractionTransportationTypeChange(
        self,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Confirm an interaction transportation-type change."""

    def reportInteractionTransportationType(
        self,
        federate: FederateHandle,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Report the transportation type used by an interaction."""

    def removeObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        userSuppliedTag: BytesLike,
        producingFederate: FederateHandle,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report removal, optionally including timestamp/order metadata."""

    def reflectAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: BytesLike,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        sentRegions: RegionHandleSet | None = None,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report values and optional regions/timestamp ordering metadata."""

    def receiveInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: BytesLike,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        sentRegions: RegionHandleSet | None = None,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report an interaction and optional regions/timestamp metadata."""

    def receiveDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: BytesLike,
        transportationType: TransportationTypeHandle,
        producingFederate: FederateHandle,
        time: LogicalTime | None = None,
        sentOrderType: OrderType | None = None,
        receivedOrderType: OrderType | None = None,
        retraction: MessageRetractionHandle | None = None,
    ) -> None:
        """Report a directed interaction and optional timestamp/order metadata."""

    def requestAttributeOwnershipAssumption(
        self,
        objectInstance: ObjectInstanceHandle,
        offeredAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike,
    ) -> None:
        """Request that this federate assume offered attribute ownership."""

    def requestDivestitureConfirmation(
        self,
        objectInstance: ObjectInstanceHandle,
        releasedAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike,
    ) -> None:
        """Confirm that a negotiated divestiture may complete."""

    def attributeOwnershipAcquisitionNotification(
        self,
        objectInstance: ObjectInstanceHandle,
        securedAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike,
    ) -> None:
        """Report attributes secured by an ownership acquisition."""

    def attributeOwnershipUnavailable(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike,
    ) -> None:
        """Report attributes unavailable for acquisition."""

    def requestAttributeOwnershipRelease(
        self,
        objectInstance: ObjectInstanceHandle,
        candidateAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike,
    ) -> None:
        """Request release of candidate attributes."""

    def confirmAttributeOwnershipAcquisitionCancellation(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Report cancellation of an ownership acquisition."""

    def informAttributeOwnership(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        owner: FederateHandle,
    ) -> None:
        """Report the federate that owns the supplied attributes."""

    def attributeIsNotOwned(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report that the supplied attributes are unowned."""

    def attributeIsOwnedByRTI(
        self, objectInstance: ObjectInstanceHandle, attributes: AttributeHandleSet
    ) -> None:
        """Report that the RTI owns the supplied attributes."""

    def timeRegulationEnabled(self, time: LogicalTime) -> None:
        """Report that time regulation was enabled at a provider-created time."""

    def timeConstrainedEnabled(self, time: LogicalTime) -> None:
        """Report that time constrained mode was enabled at a provider-created time."""

    def flushQueueGrant(self, time: LogicalTime, optimisticTime: LogicalTime) -> None:
        """Report completion of a flush-queue request and its optimistic bound."""

    def timeAdvanceGrant(self, time: LogicalTime) -> None:
        """Report completion of a time-advance request."""

    def requestRetraction(self, retraction: MessageRetractionHandle) -> None:
        """Request that a previously delivered timestamped message be retracted."""


class RTIambassador(ABC):
    """Abstract standard RTI service surface.

    Java surface: ``hla.rti1516_2025.RTIambassador``.
    C++ surface: ``RTI::RTIambassador``. Method names intentionally retain the
    Java spelling; providers select overloads and perform value conversion.
    """

    @abstractmethod
    def getHLAversion(self) -> str:
        """Return the standard HLA API version reported by the provider."""

    @abstractmethod
    def connect(
        self,
        federateAmbassador: FederateAmbassador,
        callbackModel: CallbackModel,
        configuration: RtiConfiguration | object | None = None,
        credentials: object | None = None,
    ) -> ConfigurationResult:
        """Connect using the Java overload represented by the supplied values."""

    @abstractmethod
    def disconnect(self) -> None:
        """Disconnect this ambassador."""

    @abstractmethod
    def evokeCallback(self, approximateMinimumTimeInSeconds: float) -> bool:
        """Deliver at most one queued callback."""

    @abstractmethod
    def evokeMultipleCallbacks(
        self,
        approximateMinimumTimeInSeconds: float,
        approximateMaximumTimeInSeconds: float,
    ) -> bool:
        """Deliver queued callbacks during the requested interval."""

    @abstractmethod
    def enableCallbacks(self) -> None:
        """Enable callback delivery."""

    @abstractmethod
    def disableCallbacks(self) -> None:
        """Disable callback delivery without discarding queued callbacks."""

    @abstractmethod
    def listFederationExecutions(self) -> None:
        """Request ``reportFederationExecutions`` through the callback model."""

    @abstractmethod
    def listFederationExecutionMembers(self, federationExecutionName: str) -> None:
        """Request member reporting or the missing-federation callback."""

    @abstractmethod
    def joinFederationExecution(
        self,
        federateType: str,
        federationExecutionName: str,
        *,
        federateName: str | None = None,
        additionalFomModules: Iterable[str] = (),
    ) -> FederateHandle:
        """Join through either Java name overload, optionally adding FOM URLs."""

    @abstractmethod
    def resignFederationExecution(self, resignAction: ResignAction) -> None:
        """Resign from the currently joined federation execution."""

    @abstractmethod
    def registerFederationSynchronizationPoint(
        self,
        synchronizationPointLabel: str,
        userSuppliedTag: BytesLike = b"",
        *,
        synchronizationSet: FederateHandleSet | None = None,
    ) -> None:
        """Register a point for current members or an explicit federate-handle set."""

    @abstractmethod
    def synchronizationPointAchieved(
        self,
        synchronizationPointLabel: str,
        successfully: bool = True,
    ) -> None:
        """Report this federate's result for an announced synchronization point."""

    @abstractmethod
    def queryFederationSaveStatus(self) -> None:
        """Request ``federationSaveStatusResponse`` for the joined federation."""

    @abstractmethod
    def requestFederationSave(self, label: str, time: LogicalTime | None = None) -> None:
        """Request an untimed or timestamped federation save."""

    @abstractmethod
    def federateSaveBegun(self) -> None:
        """Report that this federate has begun the instructed save."""

    @abstractmethod
    def federateSaveComplete(self) -> None:
        """Report that this federate completed the instructed save."""

    @abstractmethod
    def federateSaveNotComplete(self) -> None:
        """Report that this federate cannot complete the instructed save."""

    @abstractmethod
    def abortFederationSave(self) -> None:
        """Abort the currently active federation save."""

    @abstractmethod
    def queryFederationRestoreStatus(self) -> None:
        """Request ``federationRestoreStatusResponse`` for the joined federation."""

    @abstractmethod
    def requestFederationRestore(self, label: str) -> None:
        """Request restore from a completed scalar federation save."""

    @abstractmethod
    def federateRestoreComplete(self) -> None:
        """Report that this federate completed the instructed restore."""

    @abstractmethod
    def federateRestoreNotComplete(self) -> None:
        """Report that this federate cannot complete the instructed restore."""

    @abstractmethod
    def abortFederationRestore(self) -> None:
        """Abort the currently active federation restore."""

    @abstractmethod
    def getObjectClassHandle(self, objectClassName: str) -> ObjectClassHandle:
        """Resolve a FOM object-class name through the joined RTI."""

    @abstractmethod
    def getFederateHandle(self, federateName: str) -> FederateHandle:
        """Resolve an active federate name through the joined RTI."""

    @abstractmethod
    def getFederateName(self, federate: FederateHandle) -> str:
        """Resolve a federate handle through the joined RTI."""

    @abstractmethod
    def getObjectClassName(self, objectClass: ObjectClassHandle) -> str:
        """Resolve a portable object-class handle through the joined RTI."""

    @abstractmethod
    def getKnownObjectClassHandle(self, objectInstance: ObjectInstanceHandle) -> ObjectClassHandle:
        """Resolve the known object class for an object instance."""

    @abstractmethod
    def getAttributeHandle(
        self, objectClass: ObjectClassHandle, attributeName: str
    ) -> AttributeHandle:
        """Resolve an attribute name in its object-class domain."""

    @abstractmethod
    def getAttributeName(
        self, objectClass: ObjectClassHandle, attribute: AttributeHandle
    ) -> str:
        """Resolve an attribute handle in its object-class domain."""

    @abstractmethod
    def getUpdateRateValue(self, updateRateDesignator: str) -> float:
        """Resolve an FDD update-rate designator to seconds per update."""

    @abstractmethod
    def getUpdateRateValueForAttribute(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> float:
        """Resolve the effective update rate for a known object attribute."""

    @abstractmethod
    def getInteractionClassHandle(self, interactionClassName: str) -> InteractionClassHandle:
        """Resolve a FOM interaction-class name through the joined RTI."""

    @abstractmethod
    def getInteractionClassName(self, interactionClass: InteractionClassHandle) -> str:
        """Resolve a portable interaction-class handle through the joined RTI."""

    @abstractmethod
    def getParameterHandle(
        self, interactionClass: InteractionClassHandle, parameterName: str
    ) -> ParameterHandle:
        """Resolve a parameter name in its interaction-class domain."""

    @abstractmethod
    def getParameterName(
        self, interactionClass: InteractionClassHandle, parameter: ParameterHandle
    ) -> str:
        """Resolve a parameter handle in its interaction-class domain."""

    @abstractmethod
    def getOrderType(self, orderTypeName: str) -> OrderType:
        """Resolve the standard order-type name."""

    @abstractmethod
    def getOrderName(self, orderType: OrderType) -> str:
        """Resolve a standard order type to its FOM name."""

    @abstractmethod
    def getTransportationTypeHandle(
        self, transportationTypeName: str
    ) -> TransportationTypeHandle:
        """Resolve a provider-supported transportation-type name."""

    @abstractmethod
    def getTransportationTypeName(self, transportationType: TransportationTypeHandle) -> str:
        """Resolve a portable transportation-type handle through the RTI."""

    @abstractmethod
    def getDimensionHandle(self, dimensionName: str) -> DimensionHandle:
        """Resolve a FOM routing-space dimension name through the joined RTI."""

    @abstractmethod
    def getDimensionName(self, dimension: DimensionHandle) -> str:
        """Resolve a portable dimension handle through the joined RTI."""

    @abstractmethod
    def getAvailableDimensionsForObjectClass(
        self, objectClass: ObjectClassHandle
    ) -> DimensionHandleSet:
        """Return dimensions available to an object class."""

    @abstractmethod
    def getAvailableDimensionsForInteractionClass(
        self, interactionClass: InteractionClassHandle
    ) -> DimensionHandleSet:
        """Return dimensions available to an interaction class."""

    @abstractmethod
    def getDimensionUpperBound(self, dimension: DimensionHandle) -> int:
        """Return the provider-defined upper bound for a dimension."""

    @abstractmethod
    def normalizeServiceGroup(self, serviceGroup: ServiceGroup) -> int:
        """Return the standard normalized coordinate for a service group."""

    @abstractmethod
    def normalizeFederateHandle(self, federate: FederateHandle) -> int:
        """Return the provider normalized coordinate for a federate handle."""

    @abstractmethod
    def normalizeObjectClassHandle(self, objectClass: ObjectClassHandle) -> int:
        """Return the provider normalized coordinate for an object class."""

    @abstractmethod
    def normalizeInteractionClassHandle(self, interactionClass: InteractionClassHandle) -> int:
        """Return the provider normalized coordinate for an interaction class."""

    @abstractmethod
    def normalizeObjectInstanceHandle(self, objectInstance: ObjectInstanceHandle) -> int:
        """Return the provider normalized coordinate for an object instance."""

    @abstractmethod
    def publishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        """Publish a set of attributes for a FOM object class."""

    @abstractmethod
    def unpublishObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Stop publishing all attributes of a FOM object class."""

    @abstractmethod
    def unpublishObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        """Stop publishing the supplied object-class attributes."""

    @abstractmethod
    def subscribeObjectClassAttributes(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        """Subscribe actively or passively to an object-class attribute set."""

    @abstractmethod
    def subscribeObjectClassAttributesPassively(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        updateRateDesignator: str = "",
    ) -> None:
        """Exact Java standard passive object-attribute subscription overload."""

    @abstractmethod
    def unsubscribeObjectClass(self, objectClass: ObjectClassHandle) -> None:
        """Remove all subscriptions for a FOM object class."""

    @abstractmethod
    def unsubscribeObjectClassAttributes(
        self, objectClass: ObjectClassHandle, attributes: AttributeHandleSet
    ) -> None:
        """Remove subscriptions for the supplied object-class attributes."""

    @abstractmethod
    def subscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        active: bool = True,
        updateRateDesignator: str = "",
    ) -> None:
        """Subscribe to object attributes using attribute-to-region associations."""

    @abstractmethod
    def subscribeObjectClassAttributesPassivelyWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList
        | MutableAttributeSetRegionSetPairList,
        updateRateDesignator: str = "",
    ) -> None:
        """Exact Java standard passive regional object-attribute overload."""

    @abstractmethod
    def unsubscribeObjectClassAttributesWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        """Remove a regional object-class attribute subscription."""

    @abstractmethod
    def publishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
    ) -> None:
        """Publish directed interactions associated with an object class."""

    @abstractmethod
    def unpublishObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        """Remove all or a selected set of directed-interaction publications."""

    @abstractmethod
    def subscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet,
        *,
        universally: bool = False,
    ) -> None:
        """Subscribe to directed interactions for an object class."""

    @abstractmethod
    def subscribeObjectClassDirectedInteractionsUniversally(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | MutableInteractionClassHandleSet,
    ) -> None:
        """Exact Java standard universal directed-interaction subscription overload."""

    @abstractmethod
    def unsubscribeObjectClassDirectedInteractions(
        self,
        objectClass: ObjectClassHandle,
        interactionClasses: InteractionClassHandleSet | None = None,
    ) -> None:
        """Remove all or a selected set of directed-interaction subscriptions."""

    @abstractmethod
    def publishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        """Publish a FOM interaction class."""

    @abstractmethod
    def unpublishInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        """Stop publishing a FOM interaction class."""

    @abstractmethod
    def subscribeInteractionClass(
        self, interactionClass: InteractionClassHandle, *, active: bool = True
    ) -> None:
        """Subscribe actively or passively to a FOM interaction class."""

    @abstractmethod
    def subscribeInteractionClassPassively(
        self, interactionClass: InteractionClassHandle
    ) -> None:
        """Exact Java standard passive interaction subscription overload."""

    @abstractmethod
    def unsubscribeInteractionClass(self, interactionClass: InteractionClassHandle) -> None:
        """Remove a FOM interaction-class subscription."""

    @abstractmethod
    def reserveObjectInstanceName(self, objectInstanceName: str) -> None:
        """Asynchronously reserve a name for later named object registration."""

    @abstractmethod
    def releaseObjectInstanceName(self, objectInstanceName: str) -> None:
        """Release a name previously reserved by this federate."""

    @abstractmethod
    def reserveMultipleObjectInstanceNames(self, objectInstanceNames: ObjectInstanceNameSet) -> None:
        """Asynchronously reserve a batch of names for later object registration."""

    @abstractmethod
    def releaseMultipleObjectInstanceNames(self, objectInstanceNames: ObjectInstanceNameSet) -> None:
        """Release a batch of names previously reserved by this federate."""

    @abstractmethod
    def registerObjectInstance(
        self,
        objectClass: ObjectClassHandle,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        """Register a named or provider-named object instance."""

    @abstractmethod
    def registerObjectInstanceWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        *,
        objectInstanceName: str | None = None,
    ) -> ObjectInstanceHandle:
        """Register an object instance with attribute-to-region associations."""

    @abstractmethod
    def associateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        """Associate additional regions with an owned object instance."""

    @abstractmethod
    def unassociateRegionsForUpdates(
        self,
        objectInstance: ObjectInstanceHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
    ) -> None:
        """Remove regional associations from an owned object instance."""

    @abstractmethod
    def getObjectInstanceHandle(self, objectInstanceName: str) -> ObjectInstanceHandle:
        """Resolve a locally known object-instance name."""

    @abstractmethod
    def getObjectInstanceName(self, objectInstance: ObjectInstanceHandle) -> str:
        """Resolve a locally known object-instance handle."""

    @abstractmethod
    def deleteObjectInstance(
        self,
        objectInstance: ObjectInstanceHandle,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Delete a known object instance through the receive-order overload."""

    @abstractmethod
    def localDeleteObjectInstance(self, objectInstance: ObjectInstanceHandle) -> None:
        """Remove a locally owned object instance without sending a federation delete."""

    @abstractmethod
    def deleteObjectInstanceWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        """Delete an object instance with a provider timestamp."""

    @abstractmethod
    def updateAttributeValues(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Send receive-order values for published attributes of an object instance."""

    @abstractmethod
    def updateAttributeValuesWithTime(
        self,
        objectInstance: ObjectInstanceHandle,
        attributeValues: AttributeHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        """Send timestamp-ordered values for an object instance."""

    @abstractmethod
    def requestAttributeValueUpdate(
        self,
        objectClassOrInstance: ObjectClassHandle | ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Request current values by object class or by known instance.

        The Java API exposes these as overloads.  Python keeps one explicit
        entry point and dispatches from the type-distinct public handle.
        """

    @abstractmethod
    def requestAttributeValueUpdateWithRegions(
        self,
        objectClass: ObjectClassHandle,
        attributesAndRegions: AttributeSetRegionSetPairList,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Request current values for object attributes scoped by regions."""

    @abstractmethod
    def changeAttributeOrderType(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        orderType: OrderType,
    ) -> None:
        """Change the order type used for updates of an object instance."""

    @abstractmethod
    def changeDefaultAttributeOrderType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        orderType: OrderType,
    ) -> None:
        """Change the default order type for an object-class attribute set."""

    @abstractmethod
    def changeInteractionOrderType(
        self, interactionClass: InteractionClassHandle, orderType: OrderType
    ) -> None:
        """Change the order type used for a published interaction class."""

    @abstractmethod
    def requestAttributeTransportationTypeChange(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Request a change to an object's attribute transportation type."""

    @abstractmethod
    def changeDefaultAttributeTransportationType(
        self,
        objectClass: ObjectClassHandle,
        attributes: AttributeHandleSet,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Change the default transportation type for an object-class attribute set."""

    @abstractmethod
    def queryAttributeTransportationType(
        self, objectInstance: ObjectInstanceHandle, attribute: AttributeHandle
    ) -> None:
        """Request a report of an object's attribute transportation type."""

    @abstractmethod
    def requestInteractionTransportationTypeChange(
        self,
        interactionClass: InteractionClassHandle,
        transportationType: TransportationTypeHandle,
    ) -> None:
        """Request a change to an interaction's transportation type."""

    @abstractmethod
    def queryInteractionTransportationType(
        self, federate: FederateHandle, interactionClass: InteractionClassHandle
    ) -> None:
        """Request a report of a federate's interaction transportation type."""

    @abstractmethod
    def sendInteraction(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Send a receive-order interaction using published parameters."""

    @abstractmethod
    def sendInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        """Send a timestamp-ordered interaction."""

    @abstractmethod
    def sendDirectedInteraction(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Send a receive-order interaction directly to one object instance."""

    @abstractmethod
    def sendDirectedInteractionWithTime(
        self,
        interactionClass: InteractionClassHandle,
        objectInstance: ObjectInstanceHandle,
        parameterValues: ParameterHandleValueMap,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        """Send a timestamp-ordered directed interaction."""

    @abstractmethod
    def subscribeInteractionClassWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        regions: RegionHandleSet,
        *,
        active: bool = True,
    ) -> None:
        """Subscribe to an interaction class using DDM regions."""

    @abstractmethod
    def subscribeInteractionClassPassivelyWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        """Exact Java standard passive regional interaction subscription overload."""

    @abstractmethod
    def unsubscribeInteractionClassWithRegions(
        self, interactionClass: InteractionClassHandle, regions: RegionHandleSet
    ) -> None:
        """Remove a regional interaction subscription."""

    @abstractmethod
    def sendInteractionWithRegions(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Send a receive-order interaction with DDM regions."""

    @abstractmethod
    def sendInteractionWithRegionsWithTime(
        self,
        interactionClass: InteractionClassHandle,
        parameterValues: ParameterHandleValueMap,
        regions: RegionHandleSet,
        time: LogicalTime,
        userSuppliedTag: BytesLike = b"",
    ) -> MessageRetractionHandle:
        """Send a timestamp-ordered interaction with DDM regions."""

    @abstractmethod
    def retract(self, retraction: MessageRetractionHandle) -> None:
        """Retract a pending timestamped message."""

    @abstractmethod
    def getFederateHandleFactory(self) -> FederateHandleFactory:
        """Return the provider-owned federate-handle decoder factory."""

    @abstractmethod
    def getFederateHandleSetFactory(self) -> FederateHandleSetFactory:
        """Return the provider-owned federate-handle set factory."""

    @abstractmethod
    def getObjectClassHandleFactory(self) -> ObjectClassHandleFactory:
        """Return the provider-owned object-class-handle decoder factory."""

    @abstractmethod
    def getObjectInstanceHandleFactory(self) -> ObjectInstanceHandleFactory:
        """Return the provider-owned object-instance-handle decoder factory."""

    @abstractmethod
    def getAttributeHandleFactory(self) -> AttributeHandleFactory:
        """Return the provider-owned attribute-handle decoder factory."""

    @abstractmethod
    def getInteractionClassHandleFactory(self) -> InteractionClassHandleFactory:
        """Return the provider-owned interaction-class-handle decoder factory."""

    @abstractmethod
    def getInteractionClassHandleSetFactory(self) -> InteractionClassHandleSetFactory:
        """Return the provider-owned mutable interaction-class-set factory."""

    @abstractmethod
    def getParameterHandleFactory(self) -> ParameterHandleFactory:
        """Return the provider-owned parameter-handle decoder factory."""

    @abstractmethod
    def getTransportationTypeHandleFactory(self) -> TransportationTypeHandleFactory:
        """Return the provider-owned transportation-handle decoder factory."""

    @abstractmethod
    def getDimensionHandleFactory(self) -> DimensionHandleFactory:
        """Return the provider-owned dimension-handle decoder factory."""

    @abstractmethod
    def getRegionHandleFactory(self) -> RegionHandleFactory:
        """Return the provider-owned region-handle decoder factory."""

    @abstractmethod
    def getMessageRetractionHandleFactory(self) -> MessageRetractionHandleFactory:
        """Return the provider-owned message-retraction decoder factory."""

    @abstractmethod
    def getDimensionHandleSetFactory(self) -> DimensionHandleSetFactory:
        """Return the provider-owned mutable dimension-set factory."""

    @abstractmethod
    def getRegionHandleSetFactory(self) -> RegionHandleSetFactory:
        """Return the provider-owned mutable region-set factory."""

    @abstractmethod
    def getAttributeHandleSetFactory(self) -> AttributeHandleSetFactory:
        """Return the provider-owned mutable attribute-set factory."""

    @abstractmethod
    def getAttributeHandleValueMapFactory(self) -> AttributeHandleValueMapFactory:
        """Return the provider-owned mutable attribute-map factory."""

    @abstractmethod
    def getParameterHandleValueMapFactory(self) -> ParameterHandleValueMapFactory:
        """Return the provider-owned mutable parameter-map factory."""

    @abstractmethod
    def getAttributeSetRegionSetPairListFactory(self) -> AttributeSetRegionSetPairListFactory:
        """Return the provider-owned Java-shaped attribute/region pair-list factory."""

    @abstractmethod
    def createRegion(self, dimensions: DimensionHandleSet) -> RegionHandle:
        """Create a provider-owned region over the supplied dimensions."""

    @abstractmethod
    def commitRegionModifications(self, regions: RegionHandleSet) -> None:
        """Commit pending range-bound changes for provider-owned regions."""

    @abstractmethod
    def deleteRegion(self, region: RegionHandle) -> None:
        """Delete a provider-owned region that is no longer in use."""

    @abstractmethod
    def getDimensionHandleSet(self, region: RegionHandle) -> DimensionHandleSet:
        """Return the dimensions associated with a region."""

    @abstractmethod
    def getRangeBounds(self, region: RegionHandle, dimension: DimensionHandle) -> RangeBounds:
        """Return the current bounds for a region dimension."""

    @abstractmethod
    def setRangeBounds(
        self, region: RegionHandle, dimension: DimensionHandle, rangeBounds: RangeBounds
    ) -> None:
        """Set pending bounds for one region dimension."""

    @abstractmethod
    def getConveyRegionDesignatorSetsSwitch(self) -> bool:
        """Return whether callbacks include provider region-designator sets."""

    @abstractmethod
    def setConveyRegionDesignatorSetsSwitch(self, switchValue: bool) -> None:
        """Enable or disable region-designator sets in receive callbacks."""

    @abstractmethod
    def getObjectClassRelevanceAdvisorySwitch(self) -> bool:
        """Return the object-class relevance advisory switch state."""

    @abstractmethod
    def setObjectClassRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable object-class relevance advisories."""

    @abstractmethod
    def getAttributeRelevanceAdvisorySwitch(self) -> bool:
        """Return the attribute relevance advisory switch state."""

    @abstractmethod
    def setAttributeRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable attribute relevance advisories."""

    @abstractmethod
    def getAttributeScopeAdvisorySwitch(self) -> bool:
        """Return the attribute scope advisory switch state."""

    @abstractmethod
    def setAttributeScopeAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable attribute scope advisories."""

    @abstractmethod
    def getInteractionRelevanceAdvisorySwitch(self) -> bool:
        """Return the interaction relevance advisory switch state."""

    @abstractmethod
    def setInteractionRelevanceAdvisorySwitch(self, switchValue: bool) -> None:
        """Enable or disable interaction relevance advisories."""

    @abstractmethod
    def getAutomaticResignDirective(self) -> ResignAction:
        """Return the provider's automatic-resign directive."""

    @abstractmethod
    def setAutomaticResignDirective(self, resignAction: ResignAction) -> None:
        """Set the provider's automatic-resign directive."""

    @abstractmethod
    def getServiceReportingSwitch(self) -> bool:
        """Return the service-reporting switch state."""

    @abstractmethod
    def setServiceReportingSwitch(self, switchValue: bool) -> None:
        """Enable or disable service reporting."""

    @abstractmethod
    def getExceptionReportingSwitch(self) -> bool:
        """Return the exception-reporting switch state."""

    @abstractmethod
    def setExceptionReportingSwitch(self, switchValue: bool) -> None:
        """Enable or disable exception reporting."""

    @abstractmethod
    def getSendServiceReportsToFileSwitch(self) -> bool:
        """Return whether service reports are sent to a file."""

    @abstractmethod
    def setSendServiceReportsToFileSwitch(self, switchValue: bool) -> None:
        """Enable or disable service reports being sent to a file."""

    @abstractmethod
    def getAutoProvideSwitch(self) -> bool:
        """Return the provider's automatic attribute-provision switch."""

    @abstractmethod
    def getDelaySubscriptionEvaluationSwitch(self) -> bool:
        """Return the delayed-subscription-evaluation switch."""

    @abstractmethod
    def getAdvisoriesUseKnownClassSwitch(self) -> bool:
        """Return the known-class advisory switch."""

    @abstractmethod
    def getAllowRelaxedDDMSwitch(self) -> bool:
        """Return whether relaxed DDM is enabled."""

    @abstractmethod
    def getNonRegulatedGrantSwitch(self) -> bool:
        """Return whether non-regulated grants are enabled."""

    @abstractmethod
    def queryAttributeOwnership(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Request ownership reports for the supplied object attributes."""

    @abstractmethod
    def isAttributeOwnedByFederate(
        self,
        objectInstance: ObjectInstanceHandle,
        attribute: AttributeHandle,
    ) -> bool:
        """Return whether this federate currently owns an object attribute."""

    @abstractmethod
    def unconditionalAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Immediately divest owned attributes."""

    @abstractmethod
    def negotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Begin negotiated divestiture of owned attributes."""

    @abstractmethod
    def confirmDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        confirmedAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Confirm a pending negotiated divestiture."""

    @abstractmethod
    def cancelNegotiatedAttributeOwnershipDivestiture(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Cancel a pending negotiated divestiture."""

    @abstractmethod
    def attributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Request negotiated ownership acquisition."""

    @abstractmethod
    def attributeOwnershipAcquisitionIfAvailable(
        self,
        objectInstance: ObjectInstanceHandle,
        desiredAttributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Request ownership acquisition only for currently available attributes."""

    @abstractmethod
    def cancelAttributeOwnershipAcquisition(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
    ) -> None:
        """Cancel a pending ownership acquisition."""

    @abstractmethod
    def attributeOwnershipReleaseDenied(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> None:
        """Deny release of owned attributes."""

    @abstractmethod
    def attributeOwnershipDivestitureIfWanted(
        self,
        objectInstance: ObjectInstanceHandle,
        attributes: AttributeHandleSet,
        userSuppliedTag: BytesLike = b"",
    ) -> AttributeHandleSet:
        """Divest only attributes currently wanted by another federate."""

    @abstractmethod
    def getTimeFactory(self) -> LogicalTimeFactory:
        """Return the provider-created factory selected by the joined FOM."""

    @abstractmethod
    def enableTimeRegulation(self, lookahead: LogicalTimeInterval) -> None:
        """Enable time regulation with a provider-created lookahead interval."""

    @abstractmethod
    def disableTimeRegulation(self) -> None:
        """Disable time regulation for the joined federate."""

    @abstractmethod
    def enableTimeConstrained(self) -> None:
        """Enable time-constrained delivery."""

    @abstractmethod
    def disableTimeConstrained(self) -> None:
        """Disable time-constrained delivery."""

    @abstractmethod
    def timeAdvanceRequest(self, time: LogicalTime) -> None:
        """Request a receive-order time advance."""

    @abstractmethod
    def timeAdvanceRequestAvailable(self, time: LogicalTime) -> None:
        """Request an available receive-order time advance."""

    @abstractmethod
    def nextMessageRequest(self, time: LogicalTime) -> None:
        """Request the next-message time advance."""

    @abstractmethod
    def nextMessageRequestAvailable(self, time: LogicalTime) -> None:
        """Request an available next-message time advance."""

    @abstractmethod
    def flushQueueRequest(self, time: LogicalTime) -> None:
        """Flush queued timestamped messages through the requested time."""

    @abstractmethod
    def enableAsynchronousDelivery(self) -> None:
        """Enable asynchronous delivery of time-stamped interactions."""

    @abstractmethod
    def disableAsynchronousDelivery(self) -> None:
        """Disable asynchronous delivery of time-stamped interactions."""

    @abstractmethod
    def modifyLookahead(self, lookahead: LogicalTimeInterval) -> None:
        """Replace the current time-regulation lookahead interval."""

    @abstractmethod
    def queryLookahead(self) -> LogicalTimeInterval:
        """Return the provider-created current lookahead interval."""

    @abstractmethod
    def queryLogicalTime(self) -> LogicalTime:
        """Return the provider-created current logical time."""

    @abstractmethod
    def queryGALT(self) -> TimeQueryResult:
        """Return the Java ``TimeQueryReturn``-shaped federation GALT result."""

    @abstractmethod
    def queryLITS(self) -> TimeQueryResult:
        """Return the Java ``TimeQueryReturn``-shaped federation LITS result."""

    @abstractmethod
    def createFederationExecution(
        self,
        federationName: str,
        fomModule: str | Iterable[str],
        logicalTimeImplementationName: str = "",
    ) -> None:
        """Create a federation from one or more FOM modules."""

    @abstractmethod
    def createFederationExecutionWithMIM(
        self,
        federationName: str,
        fomModules: Iterable[str],
        mimModule: str,
        logicalTimeImplementationName: str = "",
    ) -> None:
        """Create a federation from FOM modules plus an explicit MIM URL."""

    @abstractmethod
    def destroyFederationExecution(self, federationName: str) -> None:
        """Destroy an unjoined federation execution."""


class RtiFactory(ABC):
    """Abstract provider factory.

    Java surface: ``hla.rti1516_2025.RtiFactory``.
    C++ surface: ``RTI::RTIambassadorFactory`` and provider factory helpers.
    """

    @abstractmethod
    def getRtiAmbassador(self) -> RTIambassador:
        """Return a new RTI ambassador."""

    @abstractmethod
    def getEncoderFactory(self) -> EncoderFactory:
        """Return the provider-owned standard basic-data-element factory."""

    @abstractmethod
    def rtiName(self) -> str:
        """Return the provider name."""

    @abstractmethod
    def rtiVersion(self) -> str:
        """Return the provider version."""


__all__ = [
    "HandleFactory",
    "FederateHandleFactory",
    "ObjectClassHandleFactory",
    "ObjectInstanceHandleFactory",
    "AttributeHandleFactory",
    "InteractionClassHandleFactory",
    "ParameterHandleFactory",
    "TransportationTypeHandleFactory",
    "DimensionHandleFactory",
    "RegionHandleFactory",
    "MessageRetractionHandleFactory",
    "AttributeHandleSetFactory",
    "DimensionHandleSetFactory",
    "FederateHandleSetFactory",
    "RegionHandleSetFactory",
    "InteractionClassHandleSetFactory",
    "AttributeHandleValueMapFactory",
    "ParameterHandleValueMapFactory",
    "AttributeSetRegionSetPairListFactory",
    "LogicalTimeFactory",
    "HLAinteger64TimeFactory",
    "HLAfloat64TimeFactory",
    "FederateAmbassador",
    "RTIambassador",
    "RtiFactory",
]
