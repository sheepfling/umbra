#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <memory>
#include <ostream>
#include <string>

namespace rti1516_2025 {

Exception::Exception() = default;
Exception::Exception(Exception const&) = default;
Exception& Exception::operator=(Exception const&) = default;
Exception::~Exception() noexcept = default;

#define UMBRA_DEFINE_EXCEPTION(TYPE, LITERAL)                              \
  TYPE::TYPE(std::wstring const& message) noexcept : _msg(message) {}     \
  std::wstring TYPE::what() const noexcept { return _msg; }               \
  std::wstring TYPE::name() const noexcept { return LITERAL; }

UMBRA_DEFINE_EXCEPTION(AlreadyConnected, L"AlreadyConnected")
UMBRA_DEFINE_EXCEPTION(AttributeNotDefined, L"AttributeNotDefined")
UMBRA_DEFINE_EXCEPTION(AttributeNotOwned, L"AttributeNotOwned")
UMBRA_DEFINE_EXCEPTION(CallNotAllowedFromWithinCallback, L"CallNotAllowedFromWithinCallback")
UMBRA_DEFINE_EXCEPTION(CouldNotCreateLogicalTimeFactory, L"CouldNotCreateLogicalTimeFactory")
UMBRA_DEFINE_EXCEPTION(CouldNotDecode, L"CouldNotDecode")
UMBRA_DEFINE_EXCEPTION(CouldNotEncode, L"CouldNotEncode")
UMBRA_DEFINE_EXCEPTION(CouldNotOpenFOM, L"CouldNotOpenFOM")
UMBRA_DEFINE_EXCEPTION(CouldNotOpenMIM, L"CouldNotOpenMIM")
UMBRA_DEFINE_EXCEPTION(DeletePrivilegeNotHeld, L"DeletePrivilegeNotHeld")
UMBRA_DEFINE_EXCEPTION(DesignatorIsHLAstandardMIM, L"DesignatorIsHLAstandardMIM")
UMBRA_DEFINE_EXCEPTION(ErrorReadingFOM, L"ErrorReadingFOM")
UMBRA_DEFINE_EXCEPTION(ErrorReadingMIM, L"ErrorReadingMIM")
UMBRA_DEFINE_EXCEPTION(FederateAlreadyExecutionMember, L"FederateAlreadyExecutionMember")
UMBRA_DEFINE_EXCEPTION(FederateHandleNotKnown, L"FederateHandleNotKnown")
UMBRA_DEFINE_EXCEPTION(FederateIsExecutionMember, L"FederateIsExecutionMember")
UMBRA_DEFINE_EXCEPTION(FederateNameAlreadyInUse, L"FederateNameAlreadyInUse")
UMBRA_DEFINE_EXCEPTION(FederateNotExecutionMember, L"FederateNotExecutionMember")
UMBRA_DEFINE_EXCEPTION(FederatesCurrentlyJoined, L"FederatesCurrentlyJoined")
UMBRA_DEFINE_EXCEPTION(FederationExecutionAlreadyExists, L"FederationExecutionAlreadyExists")
UMBRA_DEFINE_EXCEPTION(FederationExecutionDoesNotExist, L"FederationExecutionDoesNotExist")
UMBRA_DEFINE_EXCEPTION(IllegalTimeArithmetic, L"IllegalTimeArithmetic")
UMBRA_DEFINE_EXCEPTION(InconsistentFOM, L"InconsistentFOM")
UMBRA_DEFINE_EXCEPTION(InTimeAdvancingState, L"InTimeAdvancingState")
UMBRA_DEFINE_EXCEPTION(InternalError, L"InternalError")
UMBRA_DEFINE_EXCEPTION(InteractionClassNotDefined, L"InteractionClassNotDefined")
UMBRA_DEFINE_EXCEPTION(InteractionClassNotPublished, L"InteractionClassNotPublished")
UMBRA_DEFINE_EXCEPTION(InteractionParameterNotDefined, L"InteractionParameterNotDefined")
UMBRA_DEFINE_EXCEPTION(InvalidFOM, L"InvalidFOM")
UMBRA_DEFINE_EXCEPTION(InvalidAttributeHandle, L"InvalidAttributeHandle")
UMBRA_DEFINE_EXCEPTION(InvalidFederateHandle, L"InvalidFederateHandle")
UMBRA_DEFINE_EXCEPTION(InvalidInteractionClassHandle, L"InvalidInteractionClassHandle")
UMBRA_DEFINE_EXCEPTION(InvalidLogicalTime, L"InvalidLogicalTime")
UMBRA_DEFINE_EXCEPTION(InvalidLogicalTimeInterval, L"InvalidLogicalTimeInterval")
UMBRA_DEFINE_EXCEPTION(InvalidLookahead, L"InvalidLookahead")
UMBRA_DEFINE_EXCEPTION(InvalidMIM, L"InvalidMIM")
UMBRA_DEFINE_EXCEPTION(InvalidObjectClassHandle, L"InvalidObjectClassHandle")
UMBRA_DEFINE_EXCEPTION(InvalidObjectInstanceHandle, L"InvalidObjectInstanceHandle")
UMBRA_DEFINE_EXCEPTION(ObjectClassNotDefined, L"ObjectClassNotDefined")
UMBRA_DEFINE_EXCEPTION(ObjectClassNotPublished, L"ObjectClassNotPublished")
UMBRA_DEFINE_EXCEPTION(ObjectInstanceNameInUse, L"ObjectInstanceNameInUse")
UMBRA_DEFINE_EXCEPTION(ObjectInstanceNameNotReserved, L"ObjectInstanceNameNotReserved")
UMBRA_DEFINE_EXCEPTION(ObjectInstanceNotKnown, L"ObjectInstanceNotKnown")
UMBRA_DEFINE_EXCEPTION(InvalidParameterHandle, L"InvalidParameterHandle")
UMBRA_DEFINE_EXCEPTION(InvalidResignAction, L"InvalidResignAction")
UMBRA_DEFINE_EXCEPTION(InvalidTransportationName, L"InvalidTransportationName")
UMBRA_DEFINE_EXCEPTION(InvalidTransportationTypeHandle, L"InvalidTransportationTypeHandle")
UMBRA_DEFINE_EXCEPTION(LogicalTimeAlreadyPassed, L"LogicalTimeAlreadyPassed")
UMBRA_DEFINE_EXCEPTION(NameNotFound, L"NameNotFound")
UMBRA_DEFINE_EXCEPTION(NotConnected, L"NotConnected")
UMBRA_DEFINE_EXCEPTION(RequestForTimeConstrainedPending, L"RequestForTimeConstrainedPending")
UMBRA_DEFINE_EXCEPTION(RequestForTimeRegulationPending, L"RequestForTimeRegulationPending")
UMBRA_DEFINE_EXCEPTION(RTIinternalError, L"RTIinternalError")
UMBRA_DEFINE_EXCEPTION(TimeConstrainedAlreadyEnabled, L"TimeConstrainedAlreadyEnabled")
UMBRA_DEFINE_EXCEPTION(TimeConstrainedIsNotEnabled, L"TimeConstrainedIsNotEnabled")
UMBRA_DEFINE_EXCEPTION(TimeRegulationAlreadyEnabled, L"TimeRegulationAlreadyEnabled")
UMBRA_DEFINE_EXCEPTION(TimeRegulationIsNotEnabled, L"TimeRegulationIsNotEnabled")
UMBRA_DEFINE_EXCEPTION(UnsupportedCallbackModel, L"UnsupportedCallbackModel")

// Keep every exception class declared by the vendored IEEE 1516.1-2025
// Exception.h linkable, including declarations that no implemented runtime
// service currently throws. The official headers own these public types; the
// binding must not leave a future standards service with an unresolved symbol.
UMBRA_DEFINE_EXCEPTION(AsynchronousDeliveryAlreadyDisabled, L"AsynchronousDeliveryAlreadyDisabled")
UMBRA_DEFINE_EXCEPTION(AsynchronousDeliveryAlreadyEnabled, L"AsynchronousDeliveryAlreadyEnabled")
UMBRA_DEFINE_EXCEPTION(AttributeAcquisitionWasNotRequested, L"AttributeAcquisitionWasNotRequested")
UMBRA_DEFINE_EXCEPTION(AttributeAlreadyBeingAcquired, L"AttributeAlreadyBeingAcquired")
UMBRA_DEFINE_EXCEPTION(AttributeAlreadyBeingChanged, L"AttributeAlreadyBeingChanged")
UMBRA_DEFINE_EXCEPTION(AttributeAlreadyBeingDivested, L"AttributeAlreadyBeingDivested")
UMBRA_DEFINE_EXCEPTION(AttributeAlreadyOwned, L"AttributeAlreadyOwned")
UMBRA_DEFINE_EXCEPTION(AttributeDivestitureWasNotRequested, L"AttributeDivestitureWasNotRequested")
UMBRA_DEFINE_EXCEPTION(AttributeNotPublished, L"AttributeNotPublished")
UMBRA_DEFINE_EXCEPTION(AttributeRelevanceAdvisorySwitchIsOff, L"AttributeRelevanceAdvisorySwitchIsOff")
UMBRA_DEFINE_EXCEPTION(AttributeRelevanceAdvisorySwitchIsOn, L"AttributeRelevanceAdvisorySwitchIsOn")
UMBRA_DEFINE_EXCEPTION(AttributeScopeAdvisorySwitchIsOff, L"AttributeScopeAdvisorySwitchIsOff")
UMBRA_DEFINE_EXCEPTION(AttributeScopeAdvisorySwitchIsOn, L"AttributeScopeAdvisorySwitchIsOn")
UMBRA_DEFINE_EXCEPTION(ConnectionFailed, L"ConnectionFailed")
UMBRA_DEFINE_EXCEPTION(FederateHasNotBegunSave, L"FederateHasNotBegunSave")
UMBRA_DEFINE_EXCEPTION(FederateInternalError, L"FederateInternalError")
UMBRA_DEFINE_EXCEPTION(FederateOwnsAttributes, L"FederateOwnsAttributes")
UMBRA_DEFINE_EXCEPTION(FederateServiceInvocationsAreBeingReportedViaMOM, L"FederateServiceInvocationsAreBeingReportedViaMOM")
UMBRA_DEFINE_EXCEPTION(FederateUnableToUseTime, L"FederateUnableToUseTime")
UMBRA_DEFINE_EXCEPTION(IllegalName, L"IllegalName")
UMBRA_DEFINE_EXCEPTION(InteractionClassAlreadyBeingChanged, L"InteractionClassAlreadyBeingChanged")
UMBRA_DEFINE_EXCEPTION(InteractionRelevanceAdvisorySwitchIsOff, L"InteractionRelevanceAdvisorySwitchIsOff")
UMBRA_DEFINE_EXCEPTION(InteractionRelevanceAdvisorySwitchIsOn, L"InteractionRelevanceAdvisorySwitchIsOn")
UMBRA_DEFINE_EXCEPTION(InvalidCredentials, L"InvalidCredentials")
UMBRA_DEFINE_EXCEPTION(InvalidDimensionHandle, L"InvalidDimensionHandle")
UMBRA_DEFINE_EXCEPTION(InvalidOrderName, L"InvalidOrderName")
UMBRA_DEFINE_EXCEPTION(InvalidOrderType, L"InvalidOrderType")
UMBRA_DEFINE_EXCEPTION(InvalidRangeBound, L"InvalidRangeBound")
UMBRA_DEFINE_EXCEPTION(InvalidRegion, L"InvalidRegion")
UMBRA_DEFINE_EXCEPTION(InvalidRegionContext, L"InvalidRegionContext")
UMBRA_DEFINE_EXCEPTION(InvalidMessageRetractionHandle, L"InvalidMessageRetractionHandle")
UMBRA_DEFINE_EXCEPTION(InvalidServiceGroup, L"InvalidServiceGroup")
UMBRA_DEFINE_EXCEPTION(InvalidUpdateRateDesignator, L"InvalidUpdateRateDesignator")
UMBRA_DEFINE_EXCEPTION(MessageCanNoLongerBeRetracted, L"MessageCanNoLongerBeRetracted")
UMBRA_DEFINE_EXCEPTION(NameSetWasEmpty, L"NameSetWasEmpty")
UMBRA_DEFINE_EXCEPTION(NoAcquisitionPending, L"NoAcquisitionPending")
UMBRA_DEFINE_EXCEPTION(ObjectClassRelevanceAdvisorySwitchIsOff, L"ObjectClassRelevanceAdvisorySwitchIsOff")
UMBRA_DEFINE_EXCEPTION(ObjectClassRelevanceAdvisorySwitchIsOn, L"ObjectClassRelevanceAdvisorySwitchIsOn")
UMBRA_DEFINE_EXCEPTION(OwnershipAcquisitionPending, L"OwnershipAcquisitionPending")
UMBRA_DEFINE_EXCEPTION(RegionDoesNotContainSpecifiedDimension, L"RegionDoesNotContainSpecifiedDimension")
UMBRA_DEFINE_EXCEPTION(RegionInUseForUpdateOrSubscription, L"RegionInUseForUpdateOrSubscription")
UMBRA_DEFINE_EXCEPTION(RegionNotCreatedByThisFederate, L"RegionNotCreatedByThisFederate")
UMBRA_DEFINE_EXCEPTION(ReportServiceInvocationsAreSubscribed, L"ReportServiceInvocationsAreSubscribed")
UMBRA_DEFINE_EXCEPTION(RestoreInProgress, L"RestoreInProgress")
UMBRA_DEFINE_EXCEPTION(RestoreNotInProgress, L"RestoreNotInProgress")
UMBRA_DEFINE_EXCEPTION(RestoreNotRequested, L"RestoreNotRequested")
UMBRA_DEFINE_EXCEPTION(SaveInProgress, L"SaveInProgress")
UMBRA_DEFINE_EXCEPTION(SaveNotInProgress, L"SaveNotInProgress")
UMBRA_DEFINE_EXCEPTION(SaveNotInitiated, L"SaveNotInitiated")
UMBRA_DEFINE_EXCEPTION(SynchronizationPointLabelNotAnnounced, L"SynchronizationPointLabelNotAnnounced")
UMBRA_DEFINE_EXCEPTION(Unauthorized, L"Unauthorized")

#undef UMBRA_DEFINE_EXCEPTION

RTIambassador::RTIambassador() noexcept = default;
RTIambassador::~RTIambassador() = default;

RTIambassadorFactory::RTIambassadorFactory() = default;
RTIambassadorFactory::~RTIambassadorFactory() noexcept = default;

std::unique_ptr<RTIambassador> RTIambassadorFactory::createRTIambassador() {
  return std::make_unique<umbra_binding_detail::UmbraRtiAmbassador>();
}

std::wstring rtiName() {
  return L"Umbra";
}

std::wstring rtiVersion() {
  return L"0.1.0 (IEEE 1516.1-2025 embedded runtime foundation)";
}

}  // namespace rti1516_2025
