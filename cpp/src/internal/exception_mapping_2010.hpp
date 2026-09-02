#ifndef UMBRA_INTERNAL_EXCEPTION_MAPPING_2010_HPP
#define UMBRA_INTERNAL_EXCEPTION_MAPPING_2010_HPP

#include <RTI/Exception.h>
#include <RTI/encoding/EncodingExceptions.h>

#include <string>

// These are the exception classes shared by the IEEE 1516e-2010 Java/Python
// surface.  The vendored C++ header also contains a few historical C++-only
// names; those remain bounded as RTIinternalError at a Java/Python boundary
// because the corresponding 2010 Java API has no type to receive them.
#define UMBRA_RTI2010_JAVA_EXCEPTION_TYPES(X) \
  X(AlreadyConnected) \
  X(AsynchronousDeliveryAlreadyDisabled) \
  X(AsynchronousDeliveryAlreadyEnabled) \
  X(AttributeAcquisitionWasNotRequested) \
  X(AttributeAlreadyBeingAcquired) \
  X(AttributeAlreadyBeingChanged) \
  X(AttributeAlreadyBeingDivested) \
  X(AttributeAlreadyOwned) \
  X(AttributeDivestitureWasNotRequested) \
  X(AttributeNotDefined) \
  X(AttributeNotOwned) \
  X(AttributeNotPublished) \
  X(AttributeNotRecognized) \
  X(AttributeNotSubscribed) \
  X(AttributeRelevanceAdvisorySwitchIsOff) \
  X(AttributeRelevanceAdvisorySwitchIsOn) \
  X(AttributeScopeAdvisorySwitchIsOff) \
  X(AttributeScopeAdvisorySwitchIsOn) \
  X(CallNotAllowedFromWithinCallback) \
  X(ConnectionFailed) \
  X(CouldNotCreateLogicalTimeFactory) \
  X(CouldNotDecode) \
  X(CouldNotEncode) \
  X(CouldNotOpenFDD) \
  X(CouldNotOpenMIM) \
  X(DeletePrivilegeNotHeld) \
  X(DesignatorIsHLAstandardMIM) \
  X(ErrorReadingFDD) \
  X(ErrorReadingMIM) \
  X(FederateAlreadyExecutionMember) \
  X(FederateHandleNotKnown) \
  X(FederateHasNotBegunSave) \
  X(FederateInternalError) \
  X(FederateIsExecutionMember) \
  X(FederateNameAlreadyInUse) \
  X(FederateNotExecutionMember) \
  X(FederateOwnsAttributes) \
  X(FederateServiceInvocationsAreBeingReportedViaMOM) \
  X(FederateUnableToUseTime) \
  X(FederatesCurrentlyJoined) \
  X(FederationExecutionAlreadyExists) \
  X(FederationExecutionDoesNotExist) \
  X(IllegalName) \
  X(IllegalTimeArithmetic) \
  X(InTimeAdvancingState) \
  X(InconsistentFDD) \
  X(InteractionClassAlreadyBeingChanged) \
  X(InteractionClassNotDefined) \
  X(InteractionClassNotPublished) \
  X(InteractionParameterNotDefined) \
  X(InteractionRelevanceAdvisorySwitchIsOff) \
  X(InteractionRelevanceAdvisorySwitchIsOn) \
  X(InvalidAttributeHandle) \
  X(InvalidDimensionHandle) \
  X(InvalidFederateHandle) \
  X(InvalidInteractionClassHandle) \
  X(InvalidLocalSettingsDesignator) \
  X(InvalidLogicalTime) \
  X(InvalidLogicalTimeInterval) \
  X(InvalidLookahead) \
  X(InvalidMessageRetractionHandle) \
  X(InvalidObjectClassHandle) \
  X(InvalidOrderName) \
  X(InvalidOrderType) \
  X(InvalidParameterHandle) \
  X(InvalidRangeBound) \
  X(InvalidRegion) \
  X(InvalidRegionContext) \
  X(InvalidResignAction) \
  X(InvalidServiceGroup) \
  X(InvalidTransportationName) \
  X(InvalidTransportationType) \
  X(InvalidUpdateRateDesignator) \
  X(LogicalTimeAlreadyPassed) \
  X(MessageCanNoLongerBeRetracted) \
  X(NameNotFound) \
  X(NameSetWasEmpty) \
  X(NoAcquisitionPending) \
  X(NoRequestToEnableTimeConstrainedWasPending) \
  X(NoRequestToEnableTimeRegulationWasPending) \
  X(NotConnected) \
  X(ObjectClassNotDefined) \
  X(ObjectClassNotPublished) \
  X(ObjectClassRelevanceAdvisorySwitchIsOff) \
  X(ObjectClassRelevanceAdvisorySwitchIsOn) \
  X(ObjectInstanceNameInUse) \
  X(ObjectInstanceNameNotReserved) \
  X(ObjectInstanceNotKnown) \
  X(OwnershipAcquisitionPending) \
  X(RTIinternalError) \
  X(RegionDoesNotContainSpecifiedDimension) \
  X(RegionInUseForUpdateOrSubscription) \
  X(RegionNotCreatedByThisFederate) \
  X(RequestForTimeConstrainedPending) \
  X(RequestForTimeRegulationPending) \
  X(RestoreInProgress) \
  X(RestoreNotInProgress) \
  X(RestoreNotRequested) \
  X(SaveInProgress) \
  X(SaveNotInProgress) \
  X(SaveNotInitiated) \
  X(SynchronizationPointLabelNotAnnounced) \
  X(TimeConstrainedAlreadyEnabled) \
  X(TimeConstrainedIsNotEnabled) \
  X(TimeRegulationAlreadyEnabled) \
  X(TimeRegulationIsNotEnabled) \
  X(UnableToPerformSave) \
  X(UnknownName) \
  X(UnsupportedCallbackModel)

namespace umbra {
namespace rti1516e_2010 {

inline char const* exceptionName(rti1516e::Exception const& error) {
#define UMBRA_RTI2010_MATCH(NAME) \
  if (dynamic_cast<rti1516e::NAME const*>(&error) != nullptr) return #NAME;
  UMBRA_RTI2010_JAVA_EXCEPTION_TYPES(UMBRA_RTI2010_MATCH)
#undef UMBRA_RTI2010_MATCH
  if (dynamic_cast<rti1516e::EncoderException const*>(&error) != nullptr) {
    return "EncoderException";
  }
  return "RTIinternalError";
}

inline bool isJavaExceptionName(std::string const& name) {
#define UMBRA_RTI2010_IS_JAVA(NAME) \
  if (name == #NAME) return true;
  UMBRA_RTI2010_JAVA_EXCEPTION_TYPES(UMBRA_RTI2010_IS_JAVA)
#undef UMBRA_RTI2010_IS_JAVA
  return name == "EncoderException" || name == "DecoderException";
}

[[noreturn]] inline void throwNamedException(
    std::string const& name, std::wstring const& message) {
#define UMBRA_RTI2010_THROW(NAME) \
  if (name == #NAME) throw rti1516e::NAME(message);
  UMBRA_RTI2010_JAVA_EXCEPTION_TYPES(UMBRA_RTI2010_THROW)
#undef UMBRA_RTI2010_THROW
  if (name == "EncoderException") throw rti1516e::EncoderException(message);
  throw rti1516e::RTIinternalError(message);
}

}  // namespace rti1516e_2010
}  // namespace umbra

#endif  // UMBRA_INTERNAL_EXCEPTION_MAPPING_2010_HPP
