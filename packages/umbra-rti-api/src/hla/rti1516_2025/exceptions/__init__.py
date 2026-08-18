"""IEEE 1516.1-2025 exception types used at Python provider boundaries.

The public names mirror the complete official Java exception package. A name
being present here only guarantees transport-level exception translation; it
does not imply that the service which could raise it is implemented in Python.
"""


class RTIexception(RuntimeError):
    """Base type for standard RTI exceptions."""


class RTIinternalError(RTIexception):
    """The RTI reported an internal error."""


class AlreadyConnected(RTIexception):
    """The ambassador is already connected."""


class NotConnected(RTIexception):
    """The ambassador is not connected."""


class UnsupportedCallbackModel(RTIexception):
    """The selected callback model is unavailable."""


_STANDARD_EXCEPTION_NAMES = (
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
    "CallNotAllowedFromWithinCallback",
    "ConnectionFailed",
    "CouldNotCreateLogicalTimeFactory",
    "CouldNotDecode",
    "CouldNotEncode",
    "CouldNotOpenFOM",
    "CouldNotOpenMIM",
    "DeletePrivilegeNotHeld",
    "DesignatorIsHLAstandardMIM",
    "ErrorReadingFOM",
    "ErrorReadingMIM",
    "FederateAlreadyExecutionMember",
    "FederateHandleNotKnown",
    "FederateHasNotBegunSave",
    "FederateInternalError",
    "FederateIsExecutionMember",
    "FederateNameAlreadyInUse",
    "FederateNotExecutionMember",
    "FederateOwnsAttributes",
    "FederatesCurrentlyJoined",
    "FederateServiceInvocationsAreBeingReportedViaMOM",
    "FederateUnableToUseTime",
    "FederationExecutionAlreadyExists",
    "FederationExecutionDoesNotExist",
    "IllegalName",
    "IllegalTimeArithmetic",
    "InconsistentFOM",
    "InteractionClassAlreadyBeingChanged",
    "InteractionClassNotDefined",
    "InteractionClassNotPublished",
    "InteractionParameterNotDefined",
    "InTimeAdvancingState",
    "InvalidAttributeHandle",
    "InvalidCredentials",
    "InvalidDimensionHandle",
    "InvalidFederateHandle",
    "InvalidFOM",
    "InvalidInteractionClassHandle",
    "InvalidLogicalTime",
    "InvalidLogicalTimeInterval",
    "InvalidLookahead",
    "InvalidMessageRetractionHandle",
    "InvalidMIM",
    "InvalidObjectClassHandle",
    "InvalidObjectInstanceHandle",
    "InvalidOrderName",
    "InvalidOrderType",
    "InvalidParameterHandle",
    "InvalidRangeBound",
    "InvalidRegion",
    "InvalidRegionContext",
    "InvalidResignAction",
    "InvalidServiceGroup",
    "InvalidTransportationName",
    "InvalidTransportationTypeHandle",
    "InvalidUpdateRateDesignator",
    "LogicalTimeAlreadyPassed",
    "MessageCanNoLongerBeRetracted",
    "NameNotFound",
    "NameSetWasEmpty",
    "NoAcquisitionPending",
    "ObjectClassNotDefined",
    "ObjectClassNotPublished",
    "ObjectInstanceNameInUse",
    "ObjectInstanceNameNotReserved",
    "ObjectInstanceNotKnown",
    "OwnershipAcquisitionPending",
    "RegionDoesNotContainSpecifiedDimension",
    "RegionInUseForUpdateOrSubscription",
    "RegionNotCreatedByThisFederate",
    "ReportServiceInvocationsAreSubscribed",
    "RequestForTimeConstrainedPending",
    "RequestForTimeRegulationPending",
    "RestoreInProgress",
    "RestoreNotInProgress",
    "RestoreNotRequested",
    "SaveInProgress",
    "SaveNotInitiated",
    "SaveNotInProgress",
    "SynchronizationPointLabelNotAnnounced",
    "TimeConstrainedAlreadyEnabled",
    "TimeConstrainedIsNotEnabled",
    "TimeRegulationAlreadyEnabled",
    "TimeRegulationIsNotEnabled",
    "Unauthorized",
)

_EXCEPTION_TYPES: dict[str, type[RTIexception]] = {
    "AlreadyConnected": AlreadyConnected,
    "NotConnected": NotConnected,
    "RTIinternalError": RTIinternalError,
    "UnsupportedCallbackModel": UnsupportedCallbackModel,
}
for _exception_name in _STANDARD_EXCEPTION_NAMES:
    _exception_type = type(_exception_name, (RTIexception,), {"__module__": __name__})
    globals()[_exception_name] = _exception_type
    _EXCEPTION_TYPES[_exception_name] = _exception_type


def exceptionForName(name: str, message: str) -> RTIexception:
    """Translate an official C++ or Java simple exception name at the edge."""

    return _EXCEPTION_TYPES.get(name, RTIexception)(message)


__all__ = [
    "RTIexception",
    *sorted(_EXCEPTION_TYPES),
]
