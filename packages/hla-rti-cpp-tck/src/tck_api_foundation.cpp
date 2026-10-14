#include "tck_api_foundation.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyStandardExceptionHierarchyContract() {
  auto widenExceptionName = [](char const* name) {
    std::wstring result;
    for (auto const character : std::string{name}) {
      result.push_back(static_cast<wchar_t>(character));
    }
    return result;
  };
  auto verifyStandardException = [](auto factory,
                                    std::wstring const& expectedName) {
    auto exception = factory(L"portable-exception");
    auto copied = exception;
    auto assigned = factory(L"temporary");
    assigned = exception;
    rti::Exception const& polymorphic = exception;
    std::wostringstream text;
    text << exception;
    require(
        exception.name() == expectedName &&
            exception.what() == L"portable-exception" &&
            copied.name() == expectedName &&
            copied.what() == L"portable-exception" &&
            assigned.name() == expectedName &&
            assigned.what() == L"portable-exception" &&
            polymorphic.name() == expectedName &&
            polymorphic.what() == L"portable-exception" &&
            !text.str().empty(),
        "standard exception hierarchy name, message, copy, assignment, polymorphism, or stream semantics were not preserved");
  };
#define TCK_VERIFY_STANDARD_EXCEPTION(type) \
  verifyStandardException( \
      [](std::wstring const& message) { return rti::type(message); }, \
      widenExceptionName(#type))
  TCK_VERIFY_STANDARD_EXCEPTION(AlreadyConnected);
  TCK_VERIFY_STANDARD_EXCEPTION(AsynchronousDeliveryAlreadyDisabled);
  TCK_VERIFY_STANDARD_EXCEPTION(AsynchronousDeliveryAlreadyEnabled);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeAcquisitionWasNotRequested);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeAlreadyBeingAcquired);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeAlreadyBeingChanged);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeAlreadyBeingDivested);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeAlreadyOwned);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeDivestitureWasNotRequested);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeNotDefined);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeNotOwned);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeNotPublished);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeRelevanceAdvisorySwitchIsOff);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeRelevanceAdvisorySwitchIsOn);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeScopeAdvisorySwitchIsOff);
  TCK_VERIFY_STANDARD_EXCEPTION(AttributeScopeAdvisorySwitchIsOn);
  TCK_VERIFY_STANDARD_EXCEPTION(CallNotAllowedFromWithinCallback);
  TCK_VERIFY_STANDARD_EXCEPTION(ConnectionFailed);
  TCK_VERIFY_STANDARD_EXCEPTION(CouldNotCreateLogicalTimeFactory);
  TCK_VERIFY_STANDARD_EXCEPTION(CouldNotDecode);
  TCK_VERIFY_STANDARD_EXCEPTION(CouldNotEncode);
  TCK_VERIFY_STANDARD_EXCEPTION(CouldNotOpenFOM);
  TCK_VERIFY_STANDARD_EXCEPTION(CouldNotOpenMIM);
  TCK_VERIFY_STANDARD_EXCEPTION(DeletePrivilegeNotHeld);
  TCK_VERIFY_STANDARD_EXCEPTION(DesignatorIsHLAstandardMIM);
  TCK_VERIFY_STANDARD_EXCEPTION(RequestForTimeConstrainedPending);
  TCK_VERIFY_STANDARD_EXCEPTION(RequestForTimeRegulationPending);
  TCK_VERIFY_STANDARD_EXCEPTION(ErrorReadingFOM);
  TCK_VERIFY_STANDARD_EXCEPTION(ErrorReadingMIM);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateAlreadyExecutionMember);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateHandleNotKnown);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateHasNotBegunSave);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateInternalError);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateIsExecutionMember);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateNameAlreadyInUse);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateNotExecutionMember);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateOwnsAttributes);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateServiceInvocationsAreBeingReportedViaMOM);
  TCK_VERIFY_STANDARD_EXCEPTION(FederateUnableToUseTime);
  TCK_VERIFY_STANDARD_EXCEPTION(FederatesCurrentlyJoined);
  TCK_VERIFY_STANDARD_EXCEPTION(FederationExecutionAlreadyExists);
  TCK_VERIFY_STANDARD_EXCEPTION(FederationExecutionDoesNotExist);
  TCK_VERIFY_STANDARD_EXCEPTION(IllegalName);
  TCK_VERIFY_STANDARD_EXCEPTION(IllegalTimeArithmetic);
  TCK_VERIFY_STANDARD_EXCEPTION(InconsistentFOM);
  TCK_VERIFY_STANDARD_EXCEPTION(InteractionClassAlreadyBeingChanged);
  TCK_VERIFY_STANDARD_EXCEPTION(InteractionClassNotDefined);
  TCK_VERIFY_STANDARD_EXCEPTION(InteractionClassNotPublished);
  TCK_VERIFY_STANDARD_EXCEPTION(InteractionParameterNotDefined);
  TCK_VERIFY_STANDARD_EXCEPTION(InteractionRelevanceAdvisorySwitchIsOff);
  TCK_VERIFY_STANDARD_EXCEPTION(InteractionRelevanceAdvisorySwitchIsOn);
  TCK_VERIFY_STANDARD_EXCEPTION(InTimeAdvancingState);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidAttributeHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidCredentials);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidDimensionHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidFederateHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidFOM);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidInteractionClassHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidLogicalTime);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidLogicalTimeInterval);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidLookahead);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidMIM);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidObjectClassHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidObjectInstanceHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidOrderName);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidOrderType);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidParameterHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidRangeBound);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidRegion);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidResignAction);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidRegionContext);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidMessageRetractionHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidServiceGroup);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidTransportationName);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidTransportationTypeHandle);
  TCK_VERIFY_STANDARD_EXCEPTION(InvalidUpdateRateDesignator);
  TCK_VERIFY_STANDARD_EXCEPTION(LogicalTimeAlreadyPassed);
  TCK_VERIFY_STANDARD_EXCEPTION(MessageCanNoLongerBeRetracted);
  TCK_VERIFY_STANDARD_EXCEPTION(NameNotFound);
  TCK_VERIFY_STANDARD_EXCEPTION(NameSetWasEmpty);
  TCK_VERIFY_STANDARD_EXCEPTION(NoAcquisitionPending);
  TCK_VERIFY_STANDARD_EXCEPTION(NotConnected);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectClassNotDefined);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectClassNotPublished);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectClassRelevanceAdvisorySwitchIsOff);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectClassRelevanceAdvisorySwitchIsOn);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectInstanceNameInUse);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectInstanceNameNotReserved);
  TCK_VERIFY_STANDARD_EXCEPTION(ObjectInstanceNotKnown);
  TCK_VERIFY_STANDARD_EXCEPTION(OwnershipAcquisitionPending);
  TCK_VERIFY_STANDARD_EXCEPTION(RTIinternalError);
  TCK_VERIFY_STANDARD_EXCEPTION(RegionDoesNotContainSpecifiedDimension);
  TCK_VERIFY_STANDARD_EXCEPTION(RegionInUseForUpdateOrSubscription);
  TCK_VERIFY_STANDARD_EXCEPTION(RegionNotCreatedByThisFederate);
  TCK_VERIFY_STANDARD_EXCEPTION(ReportServiceInvocationsAreSubscribed);
  TCK_VERIFY_STANDARD_EXCEPTION(RestoreInProgress);
  TCK_VERIFY_STANDARD_EXCEPTION(RestoreNotInProgress);
  TCK_VERIFY_STANDARD_EXCEPTION(RestoreNotRequested);
  TCK_VERIFY_STANDARD_EXCEPTION(SaveInProgress);
  TCK_VERIFY_STANDARD_EXCEPTION(SaveNotInProgress);
  TCK_VERIFY_STANDARD_EXCEPTION(SaveNotInitiated);
  TCK_VERIFY_STANDARD_EXCEPTION(SynchronizationPointLabelNotAnnounced);
  TCK_VERIFY_STANDARD_EXCEPTION(TimeConstrainedAlreadyEnabled);
  TCK_VERIFY_STANDARD_EXCEPTION(TimeConstrainedIsNotEnabled);
  TCK_VERIFY_STANDARD_EXCEPTION(TimeRegulationAlreadyEnabled);
  TCK_VERIFY_STANDARD_EXCEPTION(TimeRegulationIsNotEnabled);
  TCK_VERIFY_STANDARD_EXCEPTION(Unauthorized);
  TCK_VERIFY_STANDARD_EXCEPTION(UnsupportedCallbackModel);
  TCK_VERIFY_STANDARD_EXCEPTION(InternalError);
  TCK_VERIFY_STANDARD_EXCEPTION(EncoderException);
#undef TCK_VERIFY_STANDARD_EXCEPTION
}


void verifyStandardEnumContract() {
  auto verifyEnumValues = [](std::set<int> values,
                             std::size_t expected,
                             char const* enumName) {
    require(
        values.size() == expected,
        std::string("standard ") + enumName +
            " did not expose distinct values for every enumerator");
  };
  verifyEnumValues(
      std::set<int>{
          rti::SETTINGS_IGNORED,
          rti::SETTINGS_FAILED_TO_PARSE,
          rti::SETTINGS_APPLIED},
      3U,
      "AdditionalSettingsResultCode");
  verifyEnumValues(
      std::set<int>{rti::HLA_IMMEDIATE, rti::HLA_EVOKED},
      2U,
      "CallbackModel");
  verifyEnumValues(
      std::set<int>{rti::RECEIVE, rti::TIMESTAMP},
      2U,
      "OrderType");
  verifyEnumValues(
      std::set<int>{
          rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES,
          rti::DELETE_OBJECTS,
          rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
          rti::DELETE_OBJECTS_THEN_DIVEST,
          rti::CANCEL_THEN_DELETE_THEN_DIVEST,
          rti::NO_ACTION},
      6U,
      "ResignAction");
  verifyEnumValues(
      std::set<int>{
          rti::RTI_UNABLE_TO_RESTORE,
          rti::FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
          rti::FEDERATE_RESIGNED_DURING_RESTORE,
          rti::RTI_DETECTED_FAILURE_DURING_RESTORE,
          rti::RESTORE_ABORTED},
      5U,
      "RestoreFailureReason");
  verifyEnumValues(
      std::set<int>{
          rti::NO_RESTORE_IN_PROGRESS,
          rti::FEDERATE_RESTORE_REQUEST_PENDING,
          rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN,
          rti::FEDERATE_PREPARED_TO_RESTORE,
          rti::FEDERATE_RESTORING,
          rti::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE},
      6U,
      "RestoreStatus");
  verifyEnumValues(
      std::set<int>{
          rti::RTI_UNABLE_TO_SAVE,
          rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE,
          rti::FEDERATE_RESIGNED_DURING_SAVE,
          rti::RTI_DETECTED_FAILURE_DURING_SAVE,
          rti::SAVE_TIME_CANNOT_BE_HONORED,
          rti::SAVE_ABORTED},
      6U,
      "SaveFailureReason");
  verifyEnumValues(
      std::set<int>{
          rti::NO_SAVE_IN_PROGRESS,
          rti::FEDERATE_INSTRUCTED_TO_SAVE,
          rti::FEDERATE_SAVING,
          rti::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE},
      4U,
      "SaveStatus");
  verifyEnumValues(
      std::set<int>{
          rti::FEDERATION_MANAGEMENT,
          rti::DECLARATION_MANAGEMENT,
          rti::OBJECT_MANAGEMENT,
          rti::OWNERSHIP_MANAGEMENT,
          rti::TIME_MANAGEMENT,
          rti::DATA_DISTRIBUTION_MANAGEMENT,
          rti::SUPPORT_SERVICES},
      7U,
      "ServiceGroup");
  verifyEnumValues(
      std::set<int>{
          rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE,
          rti::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED},
      2U,
      "SynchronizationPointFailureReason");
  verifyEnumValues(
      std::set<int>{
          rti::AuthorizationResult::AUTHORIZED,
          rti::AuthorizationResult::UNAUTHORIZED,
          rti::AuthorizationResult::INVALID_CREDENTIALS,
          rti::AuthorizationResult::AUTHORIZATION_ERROR},
      4U,
      "AuthorizationResult::Code");
}


void verifyStandardHandleAndCollectionContract() {
  rti::AttributeHandle invalidAttribute;
  rti::AttributeHandle copiedInvalidAttribute(invalidAttribute);
  rti::AttributeHandle assignedInvalidAttribute;
  assignedInvalidAttribute = invalidAttribute;
  require(
      !invalidAttribute.isValid() && copiedInvalidAttribute == invalidAttribute &&
          assignedInvalidAttribute == invalidAttribute &&
          copiedInvalidAttribute.hash() == invalidAttribute.hash(),
      "standard handle invalid, copy, assignment, or hash semantics were not preserved");

  rti::AttributeHandleSet attributes;
  attributes.insert(invalidAttribute);
  attributes.insert(copiedInvalidAttribute);
  require(
      attributes.size() == 1U && attributes.count(invalidAttribute) == 1U &&
          attributes.find(invalidAttribute) != attributes.end(),
      "standard AttributeHandleSet did not preserve set semantics");

  rti::ParameterHandle invalidParameter;
  std::vector<std::uint8_t> parameterBytes{0x10U, 0x20U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      invalidParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  auto copiedParameters = parameters;
  rti::ParameterHandleValueMap assignedParameters;
  assignedParameters = parameters;
  std::vector<std::uint8_t> copiedParameterBytes{0x21U, 0x22U};
  std::vector<std::uint8_t> assignedParameterBytes{0x31U, 0x32U};
  copiedParameters.at(invalidParameter).setData(
      copiedParameterBytes.data(), copiedParameterBytes.size());
  assignedParameters.at(invalidParameter).setData(
      assignedParameterBytes.data(), assignedParameterBytes.size());

  rti::AttributeHandleValueMap attributeValues;
  std::vector<std::uint8_t> attributeBytes{0x40U, 0x50U};
  attributeValues.emplace(
      invalidAttribute,
      rti::VariableLengthData(attributeBytes.data(), attributeBytes.size()));
  auto copiedAttributeValues = attributeValues;
  rti::AttributeHandleValueMap assignedAttributeValues;
  assignedAttributeValues = attributeValues;
  std::vector<std::uint8_t> copiedAttributeBytes{0x41U, 0x51U};
  std::vector<std::uint8_t> assignedAttributeBytes{0x61U, 0x71U};
  copiedAttributeValues.at(invalidAttribute).setData(
      copiedAttributeBytes.data(), copiedAttributeBytes.size());
  assignedAttributeValues.at(invalidAttribute).setData(
      assignedAttributeBytes.data(), assignedAttributeBytes.size());
  require(
      parameters.size() == 1U && parameters.find(invalidParameter) != parameters.end() &&
          copyBytes(parameters.at(invalidParameter)) ==
              std::vector<std::uint8_t>{0x10U, 0x20U} &&
          copiedParameters.size() == 1U &&
          copyBytes(copiedParameters.at(invalidParameter)) ==
              std::vector<std::uint8_t>{0x21U, 0x22U} &&
          assignedParameters.size() == 1U &&
          copyBytes(assignedParameters.at(invalidParameter)) ==
              std::vector<std::uint8_t>{0x31U, 0x32U} &&
          attributeValues.size() == 1U &&
          attributeValues.find(invalidAttribute) != attributeValues.end() &&
          copyBytes(attributeValues.at(invalidAttribute)) ==
              std::vector<std::uint8_t>{0x40U, 0x50U} &&
          copiedAttributeValues.size() == 1U &&
          copyBytes(copiedAttributeValues.at(invalidAttribute)) ==
              std::vector<std::uint8_t>{0x41U, 0x51U} &&
          assignedAttributeValues.size() == 1U &&
          copyBytes(assignedAttributeValues.at(invalidAttribute)) ==
              std::vector<std::uint8_t>{0x61U, 0x71U},
      "standard attribute/parameter value-map copy, assignment, or deep-independence semantics were not preserved");

  rti::RangeBounds bounds{2UL, 7UL};
  rti::RangeBounds copiedBounds(bounds);
  rti::RangeBounds assignedBounds;
  assignedBounds = bounds;
  bounds.setLowerBound(3UL);
  bounds.setUpperBound(9UL);
  require(
      bounds.getLowerBound() == 3UL && bounds.getUpperBound() == 9UL &&
          copiedBounds.getLowerBound() == 2UL && copiedBounds.getUpperBound() == 7UL &&
          assignedBounds.getLowerBound() == 2UL && assignedBounds.getUpperBound() == 7UL,
      "standard RangeBounds copy, assignment, or replacement semantics were not preserved");

  auto verifyInvalidHandle = [&](auto const& invalid,
                                 auto const& copied,
                                 auto const& assigned,
                                 char const* handleName) {
    require(
        !invalid.isValid() && copied == invalid && assigned == invalid &&
            !(copied != invalid) && !(invalid < copied) &&
            copied.hash() == invalid.hash(),
        std::string("standard ") + handleName +
            " invalid, copy, assignment, ordering, or hash semantics were not preserved");
  };

  rti::FederateHandle invalidFederate;
  rti::FederateHandle copiedFederate(invalidFederate);
  rti::FederateHandle assignedFederate;
  assignedFederate = invalidFederate;
  verifyInvalidHandle(
      invalidFederate, copiedFederate, assignedFederate, "FederateHandle");

  rti::ObjectClassHandle invalidObjectClass;
  rti::ObjectClassHandle copiedObjectClass(invalidObjectClass);
  rti::ObjectClassHandle assignedObjectClass;
  assignedObjectClass = invalidObjectClass;
  verifyInvalidHandle(
      invalidObjectClass, copiedObjectClass, assignedObjectClass, "ObjectClassHandle");

  rti::InteractionClassHandle invalidInteractionClass;
  rti::InteractionClassHandle copiedInteractionClass(invalidInteractionClass);
  rti::InteractionClassHandle assignedInteractionClass;
  assignedInteractionClass = invalidInteractionClass;
  verifyInvalidHandle(
      invalidInteractionClass,
      copiedInteractionClass,
      assignedInteractionClass,
      "InteractionClassHandle");

  rti::ObjectInstanceHandle invalidObjectInstance;
  rti::ObjectInstanceHandle copiedObjectInstance(invalidObjectInstance);
  rti::ObjectInstanceHandle assignedObjectInstance;
  assignedObjectInstance = invalidObjectInstance;
  verifyInvalidHandle(
      invalidObjectInstance,
      copiedObjectInstance,
      assignedObjectInstance,
      "ObjectInstanceHandle");

  rti::DimensionHandle invalidDimension;
  rti::DimensionHandle copiedDimension(invalidDimension);
  rti::DimensionHandle assignedDimension;
  assignedDimension = invalidDimension;
  verifyInvalidHandle(
      invalidDimension, copiedDimension, assignedDimension, "DimensionHandle");

  rti::MessageRetractionHandle invalidRetraction;
  rti::MessageRetractionHandle copiedRetraction(invalidRetraction);
  rti::MessageRetractionHandle assignedRetraction;
  assignedRetraction = invalidRetraction;
  verifyInvalidHandle(
      invalidRetraction,
      copiedRetraction,
      assignedRetraction,
      "MessageRetractionHandle");

  rti::RegionHandle invalidRegion;
  rti::RegionHandle copiedRegion(invalidRegion);
  rti::RegionHandle assignedRegion;
  assignedRegion = invalidRegion;
  verifyInvalidHandle(
      invalidRegion, copiedRegion, assignedRegion, "RegionHandle");

  rti::TransportationTypeHandle invalidTransportation;
  rti::TransportationTypeHandle copiedTransportation(invalidTransportation);
  rti::TransportationTypeHandle assignedTransportation;
  assignedTransportation = invalidTransportation;
  verifyInvalidHandle(
      invalidTransportation,
      copiedTransportation,
      assignedTransportation,
      "TransportationTypeHandle");

  auto verifyHandleSet = [&](auto const& invalid, auto values, char const* setName) {
    values.insert(invalid);
    values.insert(invalid);
    auto const copiedValues = values;
    decltype(values) assignedValues;
    assignedValues = values;
    auto const found = copiedValues.find(invalid);
    values.erase(invalid);
    require(
        copiedValues.size() == 1U && copiedValues.count(invalid) == 1U &&
            found != copiedValues.end() && assignedValues.size() == 1U &&
            assignedValues.count(invalid) == 1U && values.empty(),
        std::string("standard ") + setName +
            " did not preserve copy, assignment, lookup, or erase semantics");
  };
  verifyHandleSet(
      invalidFederate, rti::FederateHandleSet{}, "FederateHandleSet");
  verifyHandleSet(
      invalidInteractionClass,
      rti::InteractionClassHandleSet{},
      "InteractionClassHandleSet");
  verifyHandleSet(
      invalidDimension, rti::DimensionHandleSet{}, "DimensionHandleSet");
  verifyHandleSet(invalidRegion, rti::RegionHandleSet{}, "RegionHandleSet");
  verifyHandleSet(
      invalidAttribute, rti::AttributeHandleSet{}, "AttributeHandleSet");
  verifyHandleSet(
      invalidParameter, rti::ParameterHandleSet{}, "ParameterHandleSet");

  rti::AttributeHandleSetRegionHandleSetPairVector regionPairs;
  regionPairs.emplace_back(attributes, rti::RegionHandleSet{invalidRegion});
  auto copiedRegionPairs = regionPairs;
  rti::AttributeHandleSetRegionHandleSetPairVector assignedRegionPairs;
  assignedRegionPairs = regionPairs;
  copiedRegionPairs.front().first.clear();
  copiedRegionPairs.front().second.clear();
  assignedRegionPairs.front().second.clear();
  require(
      regionPairs.size() == 1U && regionPairs.front().first == attributes &&
          regionPairs.front().second.size() == 1U &&
          regionPairs.front().second.count(invalidRegion) == 1U &&
          copiedRegionPairs.size() == 1U && copiedRegionPairs.front().first.empty() &&
          copiedRegionPairs.front().second.empty() && assignedRegionPairs.size() == 1U &&
          assignedRegionPairs.front().first == attributes &&
          assignedRegionPairs.front().second.empty(),
      "standard attribute/region pair-vector copy, assignment, or deep-independence semantics were not preserved");

  rti::FederateHandleSaveStatusPairVector saveStatusPairs;
  saveStatusPairs.emplace_back(invalidFederate, rti::NO_SAVE_IN_PROGRESS);
  auto copiedSaveStatusPairs = saveStatusPairs;
  rti::FederateHandleSaveStatusPairVector assignedSaveStatusPairs;
  assignedSaveStatusPairs = saveStatusPairs;
  copiedSaveStatusPairs.front().second = rti::FEDERATE_SAVING;
  assignedSaveStatusPairs.front().second = rti::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE;
  require(
      saveStatusPairs.size() == 1U &&
          saveStatusPairs.front().first == invalidFederate &&
          saveStatusPairs.front().second == rti::NO_SAVE_IN_PROGRESS &&
          copiedSaveStatusPairs.size() == 1U &&
          copiedSaveStatusPairs.front().first == invalidFederate &&
          copiedSaveStatusPairs.front().second == rti::FEDERATE_SAVING &&
          assignedSaveStatusPairs.size() == 1U &&
          assignedSaveStatusPairs.front().first == invalidFederate &&
          assignedSaveStatusPairs.front().second ==
              rti::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE,
      "standard federate/save-status pair-vector copy, assignment, or independence semantics were not preserved");

  rti::FederationExecutionInformation executionInformation{
      L"portable-federation",
      L"HLAinteger64Time"};
  rti::FederationExecutionMemberInformation memberInformation{
      L"portable-federate",
      L"portable-type"};
  rti::FederateRestoreStatus restoreStatus{
      rti::FederateHandle{},
      rti::FederateHandle{},
      rti::FEDERATE_RESTORING};
  rti::FederateRestoreStatusVector restoreStatuses{restoreStatus};
  auto copiedRestoreStatuses = restoreStatuses;
  rti::FederateRestoreStatusVector assignedRestoreStatuses;
  assignedRestoreStatuses = restoreStatuses;
  copiedRestoreStatuses.front().status = rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN;
  assignedRestoreStatuses.front().status = rti::NO_RESTORE_IN_PROGRESS;
  rti::FederationExecutionInformationVector executionInformations{
      executionInformation};
  auto copiedExecutionInformations = executionInformations;
  rti::FederationExecutionInformationVector assignedExecutionInformations;
  assignedExecutionInformations = executionInformations;
  copiedExecutionInformations.front().federationExecutionName = L"copied-vector-federation";
  assignedExecutionInformations.front().logicalTimeImplementationName = L"assigned-vector-time";
  rti::FederationExecutionMemberInformationVector memberInformations{
      memberInformation};
  auto copiedMemberInformations = memberInformations;
  rti::FederationExecutionMemberInformationVector assignedMemberInformations;
  assignedMemberInformations = memberInformations;
  copiedMemberInformations.front().federateName = L"copied-vector-federate";
  assignedMemberInformations.front().federateType = L"assigned-vector-type";
  require(
      restoreStatuses.size() == 1U &&
          restoreStatuses.front().status == rti::FEDERATE_RESTORING &&
          copiedRestoreStatuses.size() == 1U &&
          copiedRestoreStatuses.front().status == rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN &&
          assignedRestoreStatuses.size() == 1U &&
          assignedRestoreStatuses.front().status == rti::NO_RESTORE_IN_PROGRESS &&
          executionInformations.size() == 1U &&
          executionInformations.front().federationExecutionName == L"portable-federation" &&
          executionInformations.front().logicalTimeImplementationName == L"HLAinteger64Time" &&
          copiedExecutionInformations.size() == 1U &&
          copiedExecutionInformations.front().federationExecutionName ==
              L"copied-vector-federation" &&
          copiedExecutionInformations.front().logicalTimeImplementationName ==
              L"HLAinteger64Time" &&
          assignedExecutionInformations.size() == 1U &&
          assignedExecutionInformations.front().federationExecutionName ==
              L"portable-federation" &&
          assignedExecutionInformations.front().logicalTimeImplementationName ==
              L"assigned-vector-time" &&
          memberInformations.size() == 1U &&
          memberInformations.front().federateName == L"portable-federate" &&
          memberInformations.front().federateType == L"portable-type" &&
          copiedMemberInformations.size() == 1U &&
          copiedMemberInformations.front().federateName == L"copied-vector-federate" &&
          copiedMemberInformations.front().federateType == L"portable-type" &&
          assignedMemberInformations.size() == 1U &&
          assignedMemberInformations.front().federateName == L"portable-federate" &&
          assignedMemberInformations.front().federateType == L"assigned-vector-type",
      "standard federation and restore vector copy, assignment, or independence semantics were not preserved");
}


}  // namespace hla_rti_cpp_tck::detail
