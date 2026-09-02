"""IEEE 1516.1-2010 RTI exception names.

Generated from the standard Java exception source inventory.  The
classes preserve names and catchability; providers supply messages.
"""

from __future__ import annotations

class RTIexception(Exception):
    """Base class for every exception in the 2010 RTI API."""

    def __init__(self, message: str = "", cause: BaseException | None = None) -> None:
        super().__init__(message)
        self.cause = cause
        if cause is not None:
            self.__cause__ = cause

class AlreadyConnected(RTIexception):
    """IEEE 1516e exception ``AlreadyConnected``."""

class AsynchronousDeliveryAlreadyDisabled(RTIexception):
    """IEEE 1516e exception ``AsynchronousDeliveryAlreadyDisabled``."""

class AsynchronousDeliveryAlreadyEnabled(RTIexception):
    """IEEE 1516e exception ``AsynchronousDeliveryAlreadyEnabled``."""

class AttributeAcquisitionWasNotRequested(RTIexception):
    """IEEE 1516e exception ``AttributeAcquisitionWasNotRequested``."""

class AttributeAlreadyBeingAcquired(RTIexception):
    """IEEE 1516e exception ``AttributeAlreadyBeingAcquired``."""

class AttributeAlreadyBeingChanged(RTIexception):
    """IEEE 1516e exception ``AttributeAlreadyBeingChanged``."""

class AttributeAlreadyBeingDivested(RTIexception):
    """IEEE 1516e exception ``AttributeAlreadyBeingDivested``."""

class AttributeAlreadyOwned(RTIexception):
    """IEEE 1516e exception ``AttributeAlreadyOwned``."""

class AttributeDivestitureWasNotRequested(RTIexception):
    """IEEE 1516e exception ``AttributeDivestitureWasNotRequested``."""

class AttributeNotDefined(RTIexception):
    """IEEE 1516e exception ``AttributeNotDefined``."""

class AttributeNotOwned(RTIexception):
    """IEEE 1516e exception ``AttributeNotOwned``."""

class AttributeNotPublished(RTIexception):
    """IEEE 1516e exception ``AttributeNotPublished``."""

class AttributeNotRecognized(RTIexception):
    """IEEE 1516e exception ``AttributeNotRecognized``."""

class AttributeNotSubscribed(RTIexception):
    """IEEE 1516e exception ``AttributeNotSubscribed``."""

class AttributeRelevanceAdvisorySwitchIsOff(RTIexception):
    """IEEE 1516e exception ``AttributeRelevanceAdvisorySwitchIsOff``."""

class AttributeRelevanceAdvisorySwitchIsOn(RTIexception):
    """IEEE 1516e exception ``AttributeRelevanceAdvisorySwitchIsOn``."""

class AttributeScopeAdvisorySwitchIsOff(RTIexception):
    """IEEE 1516e exception ``AttributeScopeAdvisorySwitchIsOff``."""

class AttributeScopeAdvisorySwitchIsOn(RTIexception):
    """IEEE 1516e exception ``AttributeScopeAdvisorySwitchIsOn``."""

class CallNotAllowedFromWithinCallback(RTIexception):
    """IEEE 1516e exception ``CallNotAllowedFromWithinCallback``."""

class ConnectionFailed(RTIexception):
    """IEEE 1516e exception ``ConnectionFailed``."""

class CouldNotCreateLogicalTimeFactory(RTIexception):
    """IEEE 1516e exception ``CouldNotCreateLogicalTimeFactory``."""

class CouldNotDecode(RTIexception):
    """IEEE 1516e exception ``CouldNotDecode``."""

class CouldNotEncode(RTIexception):
    """IEEE 1516e exception ``CouldNotEncode``."""

class CouldNotOpenFDD(RTIexception):
    """IEEE 1516e exception ``CouldNotOpenFDD``."""

class CouldNotOpenMIM(RTIexception):
    """IEEE 1516e exception ``CouldNotOpenMIM``."""

class DeletePrivilegeNotHeld(RTIexception):
    """IEEE 1516e exception ``DeletePrivilegeNotHeld``."""

class DesignatorIsHLAstandardMIM(RTIexception):
    """IEEE 1516e exception ``DesignatorIsHLAstandardMIM``."""

class ErrorReadingFDD(RTIexception):
    """IEEE 1516e exception ``ErrorReadingFDD``."""

class ErrorReadingMIM(RTIexception):
    """IEEE 1516e exception ``ErrorReadingMIM``."""

class FederateAlreadyExecutionMember(RTIexception):
    """IEEE 1516e exception ``FederateAlreadyExecutionMember``."""

class FederateHandleNotKnown(RTIexception):
    """IEEE 1516e exception ``FederateHandleNotKnown``."""

class FederateHasNotBegunSave(RTIexception):
    """IEEE 1516e exception ``FederateHasNotBegunSave``."""

class FederateInternalError(RTIexception):
    """IEEE 1516e exception ``FederateInternalError``."""

class FederateIsExecutionMember(RTIexception):
    """IEEE 1516e exception ``FederateIsExecutionMember``."""

class FederateNameAlreadyInUse(RTIexception):
    """IEEE 1516e exception ``FederateNameAlreadyInUse``."""

class FederateNotExecutionMember(RTIexception):
    """IEEE 1516e exception ``FederateNotExecutionMember``."""

class FederateOwnsAttributes(RTIexception):
    """IEEE 1516e exception ``FederateOwnsAttributes``."""

class FederateServiceInvocationsAreBeingReportedViaMOM(RTIexception):
    """IEEE 1516e exception ``FederateServiceInvocationsAreBeingReportedViaMOM``."""

class FederateUnableToUseTime(RTIexception):
    """IEEE 1516e exception ``FederateUnableToUseTime``."""

class FederatesCurrentlyJoined(RTIexception):
    """IEEE 1516e exception ``FederatesCurrentlyJoined``."""

class FederationExecutionAlreadyExists(RTIexception):
    """IEEE 1516e exception ``FederationExecutionAlreadyExists``."""

class FederationExecutionDoesNotExist(RTIexception):
    """IEEE 1516e exception ``FederationExecutionDoesNotExist``."""

class IllegalName(RTIexception):
    """IEEE 1516e exception ``IllegalName``."""

class IllegalTimeArithmetic(RTIexception):
    """IEEE 1516e exception ``IllegalTimeArithmetic``."""

class InTimeAdvancingState(RTIexception):
    """IEEE 1516e exception ``InTimeAdvancingState``."""

class InconsistentFDD(RTIexception):
    """IEEE 1516e exception ``InconsistentFDD``."""

class InteractionClassAlreadyBeingChanged(RTIexception):
    """IEEE 1516e exception ``InteractionClassAlreadyBeingChanged``."""

class InteractionClassNotDefined(RTIexception):
    """IEEE 1516e exception ``InteractionClassNotDefined``."""

class InteractionClassNotPublished(RTIexception):
    """IEEE 1516e exception ``InteractionClassNotPublished``."""

class InteractionParameterNotDefined(RTIexception):
    """IEEE 1516e exception ``InteractionParameterNotDefined``."""

class InteractionRelevanceAdvisorySwitchIsOff(RTIexception):
    """IEEE 1516e exception ``InteractionRelevanceAdvisorySwitchIsOff``."""

class InteractionRelevanceAdvisorySwitchIsOn(RTIexception):
    """IEEE 1516e exception ``InteractionRelevanceAdvisorySwitchIsOn``."""

class InvalidAttributeHandle(RTIexception):
    """IEEE 1516e exception ``InvalidAttributeHandle``."""

class InvalidDimensionHandle(RTIexception):
    """IEEE 1516e exception ``InvalidDimensionHandle``."""

class InvalidFederateHandle(RTIexception):
    """IEEE 1516e exception ``InvalidFederateHandle``."""

class InvalidInteractionClassHandle(RTIexception):
    """IEEE 1516e exception ``InvalidInteractionClassHandle``."""

class InvalidLocalSettingsDesignator(RTIexception):
    """IEEE 1516e exception ``InvalidLocalSettingsDesignator``."""

class InvalidLogicalTime(RTIexception):
    """IEEE 1516e exception ``InvalidLogicalTime``."""

class InvalidLogicalTimeInterval(RTIexception):
    """IEEE 1516e exception ``InvalidLogicalTimeInterval``."""

class InvalidLookahead(RTIexception):
    """IEEE 1516e exception ``InvalidLookahead``."""

class InvalidMessageRetractionHandle(RTIexception):
    """IEEE 1516e exception ``InvalidMessageRetractionHandle``."""

class InvalidObjectClassHandle(RTIexception):
    """IEEE 1516e exception ``InvalidObjectClassHandle``."""

class InvalidOrderName(RTIexception):
    """IEEE 1516e exception ``InvalidOrderName``."""

class InvalidOrderType(RTIexception):
    """IEEE 1516e exception ``InvalidOrderType``."""

class InvalidParameterHandle(RTIexception):
    """IEEE 1516e exception ``InvalidParameterHandle``."""

class InvalidRangeBound(RTIexception):
    """IEEE 1516e exception ``InvalidRangeBound``."""

class InvalidRegion(RTIexception):
    """IEEE 1516e exception ``InvalidRegion``."""

class InvalidRegionContext(RTIexception):
    """IEEE 1516e exception ``InvalidRegionContext``."""

class InvalidResignAction(RTIexception):
    """IEEE 1516e exception ``InvalidResignAction``."""

class InvalidServiceGroup(RTIexception):
    """IEEE 1516e exception ``InvalidServiceGroup``."""

class InvalidTransportationName(RTIexception):
    """IEEE 1516e exception ``InvalidTransportationName``."""

class InvalidTransportationType(RTIexception):
    """IEEE 1516e exception ``InvalidTransportationType``."""

class InvalidUpdateRateDesignator(RTIexception):
    """IEEE 1516e exception ``InvalidUpdateRateDesignator``."""

class LogicalTimeAlreadyPassed(RTIexception):
    """IEEE 1516e exception ``LogicalTimeAlreadyPassed``."""

class MessageCanNoLongerBeRetracted(RTIexception):
    """IEEE 1516e exception ``MessageCanNoLongerBeRetracted``."""

class NameNotFound(RTIexception):
    """IEEE 1516e exception ``NameNotFound``."""

class NameSetWasEmpty(RTIexception):
    """IEEE 1516e exception ``NameSetWasEmpty``."""

class NoAcquisitionPending(RTIexception):
    """IEEE 1516e exception ``NoAcquisitionPending``."""

class NoRequestToEnableTimeConstrainedWasPending(RTIexception):
    """IEEE 1516e exception ``NoRequestToEnableTimeConstrainedWasPending``."""

class NoRequestToEnableTimeRegulationWasPending(RTIexception):
    """IEEE 1516e exception ``NoRequestToEnableTimeRegulationWasPending``."""

class NotConnected(RTIexception):
    """IEEE 1516e exception ``NotConnected``."""

class ObjectClassNotDefined(RTIexception):
    """IEEE 1516e exception ``ObjectClassNotDefined``."""

class ObjectClassNotPublished(RTIexception):
    """IEEE 1516e exception ``ObjectClassNotPublished``."""

class ObjectClassRelevanceAdvisorySwitchIsOff(RTIexception):
    """IEEE 1516e exception ``ObjectClassRelevanceAdvisorySwitchIsOff``."""

class ObjectClassRelevanceAdvisorySwitchIsOn(RTIexception):
    """IEEE 1516e exception ``ObjectClassRelevanceAdvisorySwitchIsOn``."""

class ObjectInstanceNameInUse(RTIexception):
    """IEEE 1516e exception ``ObjectInstanceNameInUse``."""

class ObjectInstanceNameNotReserved(RTIexception):
    """IEEE 1516e exception ``ObjectInstanceNameNotReserved``."""

class ObjectInstanceNotKnown(RTIexception):
    """IEEE 1516e exception ``ObjectInstanceNotKnown``."""

class OwnershipAcquisitionPending(RTIexception):
    """IEEE 1516e exception ``OwnershipAcquisitionPending``."""

class RTIinternalError(RTIexception):
    """IEEE 1516e exception ``RTIinternalError``."""

class RegionDoesNotContainSpecifiedDimension(RTIexception):
    """IEEE 1516e exception ``RegionDoesNotContainSpecifiedDimension``."""

class RegionInUseForUpdateOrSubscription(RTIexception):
    """IEEE 1516e exception ``RegionInUseForUpdateOrSubscription``."""

class RegionNotCreatedByThisFederate(RTIexception):
    """IEEE 1516e exception ``RegionNotCreatedByThisFederate``."""

class RequestForTimeConstrainedPending(RTIexception):
    """IEEE 1516e exception ``RequestForTimeConstrainedPending``."""

class RequestForTimeRegulationPending(RTIexception):
    """IEEE 1516e exception ``RequestForTimeRegulationPending``."""

class RestoreInProgress(RTIexception):
    """IEEE 1516e exception ``RestoreInProgress``."""

class RestoreNotInProgress(RTIexception):
    """IEEE 1516e exception ``RestoreNotInProgress``."""

class RestoreNotRequested(RTIexception):
    """IEEE 1516e exception ``RestoreNotRequested``."""

class SaveInProgress(RTIexception):
    """IEEE 1516e exception ``SaveInProgress``."""

class SaveNotInProgress(RTIexception):
    """IEEE 1516e exception ``SaveNotInProgress``."""

class SaveNotInitiated(RTIexception):
    """IEEE 1516e exception ``SaveNotInitiated``."""

class SynchronizationPointLabelNotAnnounced(RTIexception):
    """IEEE 1516e exception ``SynchronizationPointLabelNotAnnounced``."""

class TimeConstrainedAlreadyEnabled(RTIexception):
    """IEEE 1516e exception ``TimeConstrainedAlreadyEnabled``."""

class TimeConstrainedIsNotEnabled(RTIexception):
    """IEEE 1516e exception ``TimeConstrainedIsNotEnabled``."""

class TimeRegulationAlreadyEnabled(RTIexception):
    """IEEE 1516e exception ``TimeRegulationAlreadyEnabled``."""

class TimeRegulationIsNotEnabled(RTIexception):
    """IEEE 1516e exception ``TimeRegulationIsNotEnabled``."""

class UnableToPerformSave(RTIexception):
    """IEEE 1516e exception ``UnableToPerformSave``."""

class UnknownName(RTIexception):
    """IEEE 1516e exception ``UnknownName``."""

class UnsupportedCallbackModel(RTIexception):
    """IEEE 1516e exception ``UnsupportedCallbackModel``."""

_EXCEPTION_TYPES = {
    "RTIexception": RTIexception,
    "AlreadyConnected": AlreadyConnected,
    "AsynchronousDeliveryAlreadyDisabled": AsynchronousDeliveryAlreadyDisabled,
    "AsynchronousDeliveryAlreadyEnabled": AsynchronousDeliveryAlreadyEnabled,
    "AttributeAcquisitionWasNotRequested": AttributeAcquisitionWasNotRequested,
    "AttributeAlreadyBeingAcquired": AttributeAlreadyBeingAcquired,
    "AttributeAlreadyBeingChanged": AttributeAlreadyBeingChanged,
    "AttributeAlreadyBeingDivested": AttributeAlreadyBeingDivested,
    "AttributeAlreadyOwned": AttributeAlreadyOwned,
    "AttributeDivestitureWasNotRequested": AttributeDivestitureWasNotRequested,
    "AttributeNotDefined": AttributeNotDefined,
    "AttributeNotOwned": AttributeNotOwned,
    "AttributeNotPublished": AttributeNotPublished,
    "AttributeNotRecognized": AttributeNotRecognized,
    "AttributeNotSubscribed": AttributeNotSubscribed,
    "AttributeRelevanceAdvisorySwitchIsOff": AttributeRelevanceAdvisorySwitchIsOff,
    "AttributeRelevanceAdvisorySwitchIsOn": AttributeRelevanceAdvisorySwitchIsOn,
    "AttributeScopeAdvisorySwitchIsOff": AttributeScopeAdvisorySwitchIsOff,
    "AttributeScopeAdvisorySwitchIsOn": AttributeScopeAdvisorySwitchIsOn,
    "CallNotAllowedFromWithinCallback": CallNotAllowedFromWithinCallback,
    "ConnectionFailed": ConnectionFailed,
    "CouldNotCreateLogicalTimeFactory": CouldNotCreateLogicalTimeFactory,
    "CouldNotDecode": CouldNotDecode,
    "CouldNotEncode": CouldNotEncode,
    "CouldNotOpenFDD": CouldNotOpenFDD,
    "CouldNotOpenMIM": CouldNotOpenMIM,
    "DeletePrivilegeNotHeld": DeletePrivilegeNotHeld,
    "DesignatorIsHLAstandardMIM": DesignatorIsHLAstandardMIM,
    "ErrorReadingFDD": ErrorReadingFDD,
    "ErrorReadingMIM": ErrorReadingMIM,
    "FederateAlreadyExecutionMember": FederateAlreadyExecutionMember,
    "FederateHandleNotKnown": FederateHandleNotKnown,
    "FederateHasNotBegunSave": FederateHasNotBegunSave,
    "FederateInternalError": FederateInternalError,
    "FederateIsExecutionMember": FederateIsExecutionMember,
    "FederateNameAlreadyInUse": FederateNameAlreadyInUse,
    "FederateNotExecutionMember": FederateNotExecutionMember,
    "FederateOwnsAttributes": FederateOwnsAttributes,
    "FederateServiceInvocationsAreBeingReportedViaMOM": FederateServiceInvocationsAreBeingReportedViaMOM,
    "FederateUnableToUseTime": FederateUnableToUseTime,
    "FederatesCurrentlyJoined": FederatesCurrentlyJoined,
    "FederationExecutionAlreadyExists": FederationExecutionAlreadyExists,
    "FederationExecutionDoesNotExist": FederationExecutionDoesNotExist,
    "IllegalName": IllegalName,
    "IllegalTimeArithmetic": IllegalTimeArithmetic,
    "InTimeAdvancingState": InTimeAdvancingState,
    "InconsistentFDD": InconsistentFDD,
    "InteractionClassAlreadyBeingChanged": InteractionClassAlreadyBeingChanged,
    "InteractionClassNotDefined": InteractionClassNotDefined,
    "InteractionClassNotPublished": InteractionClassNotPublished,
    "InteractionParameterNotDefined": InteractionParameterNotDefined,
    "InteractionRelevanceAdvisorySwitchIsOff": InteractionRelevanceAdvisorySwitchIsOff,
    "InteractionRelevanceAdvisorySwitchIsOn": InteractionRelevanceAdvisorySwitchIsOn,
    "InvalidAttributeHandle": InvalidAttributeHandle,
    "InvalidDimensionHandle": InvalidDimensionHandle,
    "InvalidFederateHandle": InvalidFederateHandle,
    "InvalidInteractionClassHandle": InvalidInteractionClassHandle,
    "InvalidLocalSettingsDesignator": InvalidLocalSettingsDesignator,
    "InvalidLogicalTime": InvalidLogicalTime,
    "InvalidLogicalTimeInterval": InvalidLogicalTimeInterval,
    "InvalidLookahead": InvalidLookahead,
    "InvalidMessageRetractionHandle": InvalidMessageRetractionHandle,
    "InvalidObjectClassHandle": InvalidObjectClassHandle,
    "InvalidOrderName": InvalidOrderName,
    "InvalidOrderType": InvalidOrderType,
    "InvalidParameterHandle": InvalidParameterHandle,
    "InvalidRangeBound": InvalidRangeBound,
    "InvalidRegion": InvalidRegion,
    "InvalidRegionContext": InvalidRegionContext,
    "InvalidResignAction": InvalidResignAction,
    "InvalidServiceGroup": InvalidServiceGroup,
    "InvalidTransportationName": InvalidTransportationName,
    "InvalidTransportationType": InvalidTransportationType,
    "InvalidUpdateRateDesignator": InvalidUpdateRateDesignator,
    "LogicalTimeAlreadyPassed": LogicalTimeAlreadyPassed,
    "MessageCanNoLongerBeRetracted": MessageCanNoLongerBeRetracted,
    "NameNotFound": NameNotFound,
    "NameSetWasEmpty": NameSetWasEmpty,
    "NoAcquisitionPending": NoAcquisitionPending,
    "NoRequestToEnableTimeConstrainedWasPending": NoRequestToEnableTimeConstrainedWasPending,
    "NoRequestToEnableTimeRegulationWasPending": NoRequestToEnableTimeRegulationWasPending,
    "NotConnected": NotConnected,
    "ObjectClassNotDefined": ObjectClassNotDefined,
    "ObjectClassNotPublished": ObjectClassNotPublished,
    "ObjectClassRelevanceAdvisorySwitchIsOff": ObjectClassRelevanceAdvisorySwitchIsOff,
    "ObjectClassRelevanceAdvisorySwitchIsOn": ObjectClassRelevanceAdvisorySwitchIsOn,
    "ObjectInstanceNameInUse": ObjectInstanceNameInUse,
    "ObjectInstanceNameNotReserved": ObjectInstanceNameNotReserved,
    "ObjectInstanceNotKnown": ObjectInstanceNotKnown,
    "OwnershipAcquisitionPending": OwnershipAcquisitionPending,
    "RTIinternalError": RTIinternalError,
    "RegionDoesNotContainSpecifiedDimension": RegionDoesNotContainSpecifiedDimension,
    "RegionInUseForUpdateOrSubscription": RegionInUseForUpdateOrSubscription,
    "RegionNotCreatedByThisFederate": RegionNotCreatedByThisFederate,
    "RequestForTimeConstrainedPending": RequestForTimeConstrainedPending,
    "RequestForTimeRegulationPending": RequestForTimeRegulationPending,
    "RestoreInProgress": RestoreInProgress,
    "RestoreNotInProgress": RestoreNotInProgress,
    "RestoreNotRequested": RestoreNotRequested,
    "SaveInProgress": SaveInProgress,
    "SaveNotInProgress": SaveNotInProgress,
    "SaveNotInitiated": SaveNotInitiated,
    "SynchronizationPointLabelNotAnnounced": SynchronizationPointLabelNotAnnounced,
    "TimeConstrainedAlreadyEnabled": TimeConstrainedAlreadyEnabled,
    "TimeConstrainedIsNotEnabled": TimeConstrainedIsNotEnabled,
    "TimeRegulationAlreadyEnabled": TimeRegulationAlreadyEnabled,
    "TimeRegulationIsNotEnabled": TimeRegulationIsNotEnabled,
    "UnableToPerformSave": UnableToPerformSave,
    "UnknownName": UnknownName,
    "UnsupportedCallbackModel": UnsupportedCallbackModel,
}
_STANDARD_EXCEPTION_NAMES = frozenset(_EXCEPTION_TYPES)

def exceptionForName(name: str, message: str = "", cause: BaseException | None = None) -> RTIexception:
    """Construct a standard exception by its Java simple class name."""
    exception_type = _EXCEPTION_TYPES.get(name.rsplit(".", 1)[-1].rsplit("$", 1)[-1])
    if exception_type is None:
        return RTIinternalError(message or f"Unknown IEEE 1516e exception: {name}", cause)
    return exception_type(message, cause)

__all__ = [
    "RTIexception",
    "AlreadyConnected",
    "AsynchronousDeliveryAlreadyDisabled",
    "AsynchronousDeliveryAlreadyEnabled",
    "AttributeAcquisitionWasNotRequested",
    "AttributeAlreadyBeingAcquired",
    "AttributeAlreadyBeingChanged",
    "AttributeAlreadyBeingDivested",
    "AttributeAlreadyOwned",
    "AttributeDivestitureWasNotRequested",
    "AttributeNotDefined",
    "AttributeNotOwned",
    "AttributeNotPublished",
    "AttributeNotRecognized",
    "AttributeNotSubscribed",
    "AttributeRelevanceAdvisorySwitchIsOff",
    "AttributeRelevanceAdvisorySwitchIsOn",
    "AttributeScopeAdvisorySwitchIsOff",
    "AttributeScopeAdvisorySwitchIsOn",
    "CallNotAllowedFromWithinCallback",
    "ConnectionFailed",
    "CouldNotCreateLogicalTimeFactory",
    "CouldNotDecode",
    "CouldNotEncode",
    "CouldNotOpenFDD",
    "CouldNotOpenMIM",
    "DeletePrivilegeNotHeld",
    "DesignatorIsHLAstandardMIM",
    "ErrorReadingFDD",
    "ErrorReadingMIM",
    "FederateAlreadyExecutionMember",
    "FederateHandleNotKnown",
    "FederateHasNotBegunSave",
    "FederateInternalError",
    "FederateIsExecutionMember",
    "FederateNameAlreadyInUse",
    "FederateNotExecutionMember",
    "FederateOwnsAttributes",
    "FederateServiceInvocationsAreBeingReportedViaMOM",
    "FederateUnableToUseTime",
    "FederatesCurrentlyJoined",
    "FederationExecutionAlreadyExists",
    "FederationExecutionDoesNotExist",
    "IllegalName",
    "IllegalTimeArithmetic",
    "InTimeAdvancingState",
    "InconsistentFDD",
    "InteractionClassAlreadyBeingChanged",
    "InteractionClassNotDefined",
    "InteractionClassNotPublished",
    "InteractionParameterNotDefined",
    "InteractionRelevanceAdvisorySwitchIsOff",
    "InteractionRelevanceAdvisorySwitchIsOn",
    "InvalidAttributeHandle",
    "InvalidDimensionHandle",
    "InvalidFederateHandle",
    "InvalidInteractionClassHandle",
    "InvalidLocalSettingsDesignator",
    "InvalidLogicalTime",
    "InvalidLogicalTimeInterval",
    "InvalidLookahead",
    "InvalidMessageRetractionHandle",
    "InvalidObjectClassHandle",
    "InvalidOrderName",
    "InvalidOrderType",
    "InvalidParameterHandle",
    "InvalidRangeBound",
    "InvalidRegion",
    "InvalidRegionContext",
    "InvalidResignAction",
    "InvalidServiceGroup",
    "InvalidTransportationName",
    "InvalidTransportationType",
    "InvalidUpdateRateDesignator",
    "LogicalTimeAlreadyPassed",
    "MessageCanNoLongerBeRetracted",
    "NameNotFound",
    "NameSetWasEmpty",
    "NoAcquisitionPending",
    "NoRequestToEnableTimeConstrainedWasPending",
    "NoRequestToEnableTimeRegulationWasPending",
    "NotConnected",
    "ObjectClassNotDefined",
    "ObjectClassNotPublished",
    "ObjectClassRelevanceAdvisorySwitchIsOff",
    "ObjectClassRelevanceAdvisorySwitchIsOn",
    "ObjectInstanceNameInUse",
    "ObjectInstanceNameNotReserved",
    "ObjectInstanceNotKnown",
    "OwnershipAcquisitionPending",
    "RTIinternalError",
    "RegionDoesNotContainSpecifiedDimension",
    "RegionInUseForUpdateOrSubscription",
    "RegionNotCreatedByThisFederate",
    "RequestForTimeConstrainedPending",
    "RequestForTimeRegulationPending",
    "RestoreInProgress",
    "RestoreNotInProgress",
    "RestoreNotRequested",
    "SaveInProgress",
    "SaveNotInProgress",
    "SaveNotInitiated",
    "SynchronizationPointLabelNotAnnounced",
    "TimeConstrainedAlreadyEnabled",
    "TimeConstrainedIsNotEnabled",
    "TimeRegulationAlreadyEnabled",
    "TimeRegulationIsNotEnabled",
    "UnableToPerformSave",
    "UnknownName",
    "UnsupportedCallbackModel",
    "exceptionForName",
]
