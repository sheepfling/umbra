#include "tck_api_foundation.hpp"
#include "tck_configuration_authorization.hpp"
#include "tck_data_encoding.hpp"
#include "tck_factory_identity.hpp"
#include "tck_logical_time.hpp"

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


void verifyStandardNullFederateAmbassadorContract() {
  rti::NullFederateAmbassador nullFederateAmbassador;
  rti::FederationExecutionInformationVector federationExecutions;
  rti::FederationExecutionMemberInformationVector federationMembers;
  rti::FederateHandleSet failedFederates;
  rti::FederateHandleSaveStatusPairVector saveStatuses;
  rti::FederateRestoreStatusVector restoreStatuses;
  rti::VariableLengthData callbackTag;
  rti::ObjectClassHandle objectClass;
  rti::ObjectInstanceHandle objectInstance;
  rti::InteractionClassHandle interactionClass;
  rti::FederateHandle federateHandle;
  rti::AttributeHandle attribute;
  rti::AttributeHandleSet attributes;
  rti::AttributeHandleValueMap attributeValues;
  rti::ParameterHandleValueMap parameterValues;
  rti::RegionHandleSet regions;
  rti::TransportationTypeHandle transportation;
  rti::MessageRetractionHandle retraction;
  std::set<std::wstring> reservedNames;
  rti::HLAinteger64Time callbackTime{1};
  rti::HLAinteger64Time optimisticTime{0};
  nullFederateAmbassador.connectionLost(L"portable");
  nullFederateAmbassador.reportFederationExecutions(federationExecutions);
  nullFederateAmbassador.reportFederationExecutionMembers(
      L"portable-federation", federationMembers);
  nullFederateAmbassador.reportFederationExecutionDoesNotExist(
      L"portable-missing");
  nullFederateAmbassador.federateResigned(L"portable-resign");
  nullFederateAmbassador.synchronizationPointRegistrationSucceeded(L"portable-sync");
  nullFederateAmbassador.synchronizationPointRegistrationFailed(
      L"portable-sync", rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE);
  nullFederateAmbassador.announceSynchronizationPoint(L"portable-sync", callbackTag);
  nullFederateAmbassador.federationSynchronized(L"portable-sync", failedFederates);
  nullFederateAmbassador.initiateFederateSave(L"portable-save");
  nullFederateAmbassador.initiateFederateSave(L"portable-save", callbackTime);
  nullFederateAmbassador.federationSaved();
  nullFederateAmbassador.federationNotSaved(rti::RTI_UNABLE_TO_SAVE);
  nullFederateAmbassador.federationSaveStatusResponse(saveStatuses);
  nullFederateAmbassador.requestFederationRestoreSucceeded(L"portable-restore");
  nullFederateAmbassador.requestFederationRestoreFailed(L"portable-restore");
  nullFederateAmbassador.federationRestoreBegun();
  nullFederateAmbassador.initiateFederateRestore(
      L"portable-restore", L"portable-federate", federateHandle);
  nullFederateAmbassador.federationRestored();
  nullFederateAmbassador.federationNotRestored(rti::RTI_UNABLE_TO_RESTORE);
  nullFederateAmbassador.federationRestoreStatusResponse(restoreStatuses);
  nullFederateAmbassador.startRegistrationForObjectClass(objectClass);
  nullFederateAmbassador.stopRegistrationForObjectClass(objectClass);
  nullFederateAmbassador.turnInteractionsOn(interactionClass);
  nullFederateAmbassador.turnInteractionsOff(interactionClass);
  nullFederateAmbassador.objectInstanceNameReservationSucceeded(L"portable-object");
  nullFederateAmbassador.objectInstanceNameReservationFailed(L"portable-object");
  nullFederateAmbassador.multipleObjectInstanceNameReservationSucceeded(reservedNames);
  nullFederateAmbassador.multipleObjectInstanceNameReservationFailed(reservedNames);
  nullFederateAmbassador.discoverObjectInstance(
      objectInstance, objectClass, L"portable-object", federateHandle);
  nullFederateAmbassador.reflectAttributeValues(
      objectInstance, attributeValues, callbackTag, transportation, federateHandle, nullptr);
  nullFederateAmbassador.reflectAttributeValues(
      objectInstance, attributeValues, callbackTag, transportation, federateHandle, nullptr,
      callbackTime, rti::RECEIVE, rti::RECEIVE, nullptr);
  nullFederateAmbassador.receiveInteraction(
      interactionClass, parameterValues, callbackTag, transportation, federateHandle, nullptr);
  nullFederateAmbassador.receiveInteraction(
      interactionClass, parameterValues, callbackTag, transportation, federateHandle, nullptr,
      callbackTime, rti::RECEIVE, rti::RECEIVE, nullptr);
  nullFederateAmbassador.receiveDirectedInteraction(
      interactionClass, objectInstance, parameterValues, callbackTag, transportation,
      federateHandle);
  nullFederateAmbassador.receiveDirectedInteraction(
      interactionClass, objectInstance, parameterValues, callbackTag, transportation,
      federateHandle, callbackTime, rti::RECEIVE, rti::RECEIVE, nullptr);
  nullFederateAmbassador.removeObjectInstance(
      objectInstance, callbackTag, federateHandle);
  nullFederateAmbassador.removeObjectInstance(
      objectInstance, callbackTag, federateHandle, callbackTime, rti::RECEIVE,
      rti::RECEIVE, nullptr);
  nullFederateAmbassador.attributesInScope(objectInstance, attributes);
  nullFederateAmbassador.attributesOutOfScope(objectInstance, attributes);
  nullFederateAmbassador.provideAttributeValueUpdate(objectInstance, attributes, callbackTag);
  nullFederateAmbassador.turnUpdatesOnForObjectInstance(objectInstance, attributes);
  nullFederateAmbassador.turnUpdatesOnForObjectInstance(
      objectInstance, attributes, L"portable-rate");
  nullFederateAmbassador.turnUpdatesOffForObjectInstance(objectInstance, attributes);
  nullFederateAmbassador.confirmAttributeTransportationTypeChange(
      objectInstance, attributes, transportation);
  nullFederateAmbassador.reportAttributeTransportationType(
      objectInstance, attribute, transportation);
  nullFederateAmbassador.confirmInteractionTransportationTypeChange(
      interactionClass, transportation);
  nullFederateAmbassador.reportInteractionTransportationType(
      federateHandle, interactionClass, transportation);
  nullFederateAmbassador.requestAttributeOwnershipAssumption(
      objectInstance, attributes, callbackTag);
  nullFederateAmbassador.requestDivestitureConfirmation(
      objectInstance, attributes, callbackTag);
  nullFederateAmbassador.attributeOwnershipAcquisitionNotification(
      objectInstance, attributes, callbackTag);
  nullFederateAmbassador.attributeOwnershipUnavailable(
      objectInstance, attributes, callbackTag);
  nullFederateAmbassador.requestAttributeOwnershipRelease(
      objectInstance, attributes, callbackTag);
  nullFederateAmbassador.confirmAttributeOwnershipAcquisitionCancellation(
      objectInstance, attributes);
  nullFederateAmbassador.informAttributeOwnership(
      objectInstance, attributes, federateHandle);
  nullFederateAmbassador.attributeIsNotOwned(objectInstance, attributes);
  nullFederateAmbassador.attributeIsOwnedByRTI(objectInstance, attributes);
  nullFederateAmbassador.timeRegulationEnabled(callbackTime);
  nullFederateAmbassador.timeConstrainedEnabled(callbackTime);
  nullFederateAmbassador.flushQueueGrant(callbackTime, optimisticTime);
  nullFederateAmbassador.timeAdvanceGrant(callbackTime);
  nullFederateAmbassador.requestRetraction(retraction);
  require(true, "standard NullFederateAmbassador callback contract completed");
}
void scenarioNullFederateAmbassadorContract(
    Options const&,
    rti::CallbackModel) {
  verifyStandardNullFederateAmbassadorContract();
}
void scenarioApiSurfaceInventory(Options const&, rti::CallbackModel) {
  static_assert(sizeof(rti::RTIambassadorFactory) > 0,
                "RTIambassadorFactory is not available in the standard API");
  static_assert(sizeof(rti::RTIambassador) > 0,
                "RTIambassador is not available in the standard API");
 static_assert(sizeof(rti::FederateAmbassador) > 0,
               "FederateAmbassador is not available in the standard API");
 static_assert(sizeof(rti::NullFederateAmbassador) > 0,
               "NullFederateAmbassador is not available in the standard API");
  static_assert(sizeof(rti::Authorizer) > 0,
                "Authorizer is not available in the standard API");
  static_assert(sizeof(rti::AuthorizerFactory) > 0,
                "AuthorizerFactory is not available in the standard API");
  static_assert(sizeof(rti::HLAauthorizerFactoryFactory) > 0,
                "HLAauthorizerFactoryFactory is not available in the standard API");
 static_assert(sizeof(rti::LogicalTime) > 0,
                "LogicalTime is not available in the standard API");
  static_assert(sizeof(rti::LogicalTimeFactory) > 0,
                "LogicalTimeFactory is not available in the standard API");
  static_assert(sizeof(rti::LogicalTimeInterval) > 0,
                "LogicalTimeInterval is not available in the standard API");
  static_assert(sizeof(rti::VariableLengthData) > 0,
                "VariableLengthData is not available in the standard API");
  static_assert(sizeof(rti::AttributeHandle) > 0,
                "AttributeHandle is not available in the standard API");
  static_assert(sizeof(rti::ParameterHandle) > 0,
                "ParameterHandle is not available in the standard API");
  static_assert(sizeof(rti::FederateHandle) > 0,
                "FederateHandle is not available in the standard API");
  static_assert(sizeof(rti::ObjectClassHandle) > 0,
                "ObjectClassHandle is not available in the standard API");
  static_assert(sizeof(rti::InteractionClassHandle) > 0,
                "InteractionClassHandle is not available in the standard API");
  static_assert(sizeof(rti::ObjectInstanceHandle) > 0,
                "ObjectInstanceHandle is not available in the standard API");
  static_assert(sizeof(rti::DimensionHandle) > 0,
                "DimensionHandle is not available in the standard API");
  static_assert(sizeof(rti::MessageRetractionHandle) > 0,
                "MessageRetractionHandle is not available in the standard API");
  static_assert(sizeof(rti::RegionHandle) > 0,
                "RegionHandle is not available in the standard API");
  static_assert(sizeof(rti::TransportationTypeHandle) > 0,
                "TransportationTypeHandle is not available in the standard API");
  static_assert(sizeof(rti::AttributeHandleSetRegionHandleSetPairVector) > 0,
                "AttributeHandleSetRegionHandleSetPairVector is not available in the standard API");
  static_assert(sizeof(rti::FederateHandleSaveStatusPairVector) > 0,
                "FederateHandleSaveStatusPairVector is not available in the standard API");
  static_assert(sizeof(rti::FederateRestoreStatusVector) > 0,
                "FederateRestoreStatusVector is not available in the standard API");
  static_assert(sizeof(rti::FederationExecutionInformationVector) > 0,
                "FederationExecutionInformationVector is not available in the standard API");
  static_assert(sizeof(rti::FederationExecutionMemberInformationVector) > 0,
                "FederationExecutionMemberInformationVector is not available in the standard API");
  static_assert(sizeof(rti::RangeBounds) > 0,
                "RangeBounds is not available in the standard API");
  static_assert(sizeof(rti::RtiConfiguration) > 0,
                "RtiConfiguration is not available in the standard API");
  static_assert(sizeof(rti::ConfigurationResult) > 0,
                "ConfigurationResult is not available in the standard API");
  static_assert(sizeof(rti::FederationExecutionInformation) > 0,
                "FederationExecutionInformation is not available in the standard API");
  static_assert(sizeof(rti::FederationExecutionMemberInformation) > 0,
                "FederationExecutionMemberInformation is not available in the standard API");
  static_assert(sizeof(rti::FederateRestoreStatus) > 0,
                "FederateRestoreStatus is not available in the standard API");
  static_assert(sizeof(rti::Credentials) > 0,
                "Credentials is not available in the standard API");
  static_assert(sizeof(rti::HLAnoCredentials) > 0,
                "HLAnoCredentials is not available in the standard API");
  static_assert(sizeof(rti::HLAplainTextPassword) > 0,
                "HLAplainTextPassword is not available in the standard API");
  static_assert(sizeof(rti::AuthorizationResult) > 0,
                "AuthorizationResult is not available in the standard API");
  static_assert(sizeof(rti::Exception) > 0,
                "Exception is not available in the standard API");
  static_assert(sizeof(rti::EncoderException) > 0,
                "EncoderException is not available in the standard API");
  static_assert(sizeof(rti::DataElement) > 0,
                "DataElement is not available in the standard API");

  verifyStandardEnumContract();

  verifyStandardExceptionHierarchyContract();

  verifyStandardHandleAndCollectionContract();

  verifyStandardConfigurationAndAuthorizationContract();

  verifyStandardAuthorizerFactoryFactoryContract();

  verifyStandardRTIambassadorFactoryContract();

  verifyStandardRuntimeIdentityContract();

  verifyStandardLogicalTimeFactoryFactoryContract();

  verifyStandardDataElementContract();

  {
    PortableLogicalTimeFactory portableFactory;
    rti::LogicalTimeFactory& factoryInterface = portableFactory;
    require(
        factoryInterface.getName() == L"PortableLogicalTime",
        "standard LogicalTimeFactory did not preserve its implementation name");

    auto initial = factoryInterface.makeInitial();
    auto final = factoryInterface.makeFinal();
    auto zero = factoryInterface.makeZero();
    auto epsilon = factoryInterface.makeEpsilon();
    require(
        dynamic_cast<PortableLogicalTime*>(initial.get()) != nullptr &&
            dynamic_cast<PortableLogicalTime*>(final.get()) != nullptr &&
            dynamic_cast<PortableLogicalTimeInterval*>(zero.get()) != nullptr &&
            dynamic_cast<PortableLogicalTimeInterval*>(epsilon.get()) != nullptr &&
            initial->isInitial() && !initial->isFinal() &&
            final->isFinal() && !final->isInitial() &&
            zero->isZero() && !zero->isEpsilon() &&
            epsilon->isEpsilon() && !epsilon->isZero(),
        "standard LogicalTimeFactory did not create the required boundary values");

    auto advanced = factoryInterface.makeInitial();
    *advanced += *epsilon;
    require(
        *advanced > *initial && *initial < *advanced &&
            *advanced >= *advanced && *initial <= *advanced &&
            !(*advanced == *initial),
        "standard LogicalTime comparison or interval addition semantics were not preserved");
    *advanced -= *epsilon;
    require(
        *advanced == *initial,
        "standard LogicalTime interval subtraction did not restore the initial value");
    advanced->setFinal();
    require(advanced->isFinal(), "standard LogicalTime setFinal did not set final state");
    advanced->setInitial();
    require(advanced->isInitial(), "standard LogicalTime setInitial did not restore initial state");

    *advanced += *epsilon;
    auto copied = factoryInterface.makeInitial();
    *copied = *advanced;
    require(
        *copied == *advanced &&
            advanced->implementationName() == L"PortableLogicalTime" &&
            advanced->toString() == L"1",
        "standard LogicalTime assignment or diagnostic naming semantics were not preserved");

    auto difference = factoryInterface.makeZero();
    difference->setToDifference(*advanced, *initial);
    require(
        *difference == *epsilon &&
            difference->implementationName() == L"PortableLogicalTime" &&
            difference->toString() == L"1",
        "standard LogicalTimeInterval difference or diagnostic naming semantics were not preserved");
    auto intervalCopy = factoryInterface.makeZero();
    *intervalCopy = *difference;
    require(*intervalCopy == *difference, "standard LogicalTimeInterval assignment failed");
    *intervalCopy += *epsilon;
    require(
        *intervalCopy > *difference && *difference < *intervalCopy &&
            *intervalCopy >= *intervalCopy && *difference <= *intervalCopy &&
            !(*intervalCopy == *difference),
        "standard LogicalTimeInterval comparison or addition semantics were not preserved");
    *intervalCopy -= *epsilon;
    require(
        *intervalCopy == *difference,
        "standard LogicalTimeInterval subtraction did not restore the difference");

    auto encodedTime = advanced->encode();
    auto decodedTime = factoryInterface.decodeLogicalTime(encodedTime);
    require(
        decodedTime != nullptr && *decodedTime == *advanced &&
            encodedTime.size() == advanced->encodedLength(),
        "standard LogicalTime variable-length encode/decode did not round-trip");
    std::vector<rti::Octet> timeBuffer(advanced->encodedLength());
    auto const timeBytes = advanced->encode(timeBuffer.data(), timeBuffer.size());
    auto directDecodedTime = factoryInterface.decodeLogicalTime(
        timeBuffer.data(), timeBytes);
    require(
        timeBytes == advanced->encodedLength() &&
            directDecodedTime != nullptr && *directDecodedTime == *advanced,
        "standard LogicalTime direct-buffer encode/decode did not round-trip");
    std::vector<rti::Octet> shortBuffer(
        portableLogicalTimeEncodingSize - 1U);
    requireException(
        [&] { advanced->encode(shortBuffer.data(), shortBuffer.size()); },
        L"CouldNotEncode",
        "standard LogicalTime short output buffer");
    requireException(
        [&] { decodedTime->decode(shortBuffer.data(), shortBuffer.size()); },
        L"CouldNotDecode",
        "standard LogicalTime short input buffer");

    auto encodedInterval = difference->encode();
    auto decodedInterval = factoryInterface.decodeLogicalTimeInterval(encodedInterval);
    require(
        decodedInterval != nullptr && *decodedInterval == *difference &&
            encodedInterval.size() == difference->encodedLength(),
        "standard LogicalTimeInterval variable-length encode/decode did not round-trip");
    std::vector<rti::Octet> intervalBuffer(difference->encodedLength());
    auto const intervalBytes = difference->encode(
        intervalBuffer.data(), intervalBuffer.size());
    auto directDecodedInterval = factoryInterface.decodeLogicalTimeInterval(
        intervalBuffer.data(), intervalBytes);
    require(
        intervalBytes == difference->encodedLength() &&
            directDecodedInterval != nullptr && *directDecodedInterval == *difference,
        "standard LogicalTimeInterval direct-buffer encode/decode did not round-trip");
    requireException(
        [&] { difference->encode(shortBuffer.data(), shortBuffer.size()); },
        L"CouldNotEncode",
        "standard LogicalTimeInterval short output buffer");
    requireException(
        [&] { decodedInterval->decode(shortBuffer.data(), shortBuffer.size()); },
        L"CouldNotDecode",
        "standard LogicalTimeInterval short input buffer");

    std::wostringstream timeText;
    timeText << *advanced;
    std::wostringstream intervalText;
    intervalText << *difference;
    require(
        !timeText.str().empty() && !intervalText.str().empty(),
        "standard logical-time stream output was empty");
  }

  verifyStandardNullFederateAmbassadorContract();


  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  require(
      static_cast<bool>(ambassador),
      "the standard API factory did not return an RTIambassador");

  rti::InvalidFOM portableException{L"portable-exception"};
  rti::InvalidFOM copiedPortableException(portableException);
  rti::InvalidFOM assignedPortableException{L"temporary"};
  assignedPortableException = portableException;
  rti::Exception const& polymorphicException = portableException;
  std::wostringstream exceptionText;
  exceptionText << portableException;
  require(
      portableException.name() == L"InvalidFOM" &&
          portableException.what() == L"portable-exception" &&
          copiedPortableException.name() == L"InvalidFOM" &&
          copiedPortableException.what() == L"portable-exception" &&
          assignedPortableException.name() == L"InvalidFOM" &&
          assignedPortableException.what() == L"portable-exception" &&
          polymorphicException.name() == L"InvalidFOM" &&
          polymorphicException.what() == L"portable-exception" &&
          !exceptionText.str().empty(),
      "standard HLA exception name, message, copy, assignment, or stream semantics were not preserved");

  std::uint8_t bytes[] = {0x01U, 0x02U, 0x03U};
  rti::VariableLengthData data(bytes, sizeof(bytes));
  require(
      data.size() == sizeof(bytes) && copyBytes(data) == std::vector<std::uint8_t>{1U, 2U, 3U},
      "the standard VariableLengthData surface did not preserve its bytes");

  std::vector<rti::Octet> callerOwned{
      static_cast<rti::Octet>('H'),
      static_cast<rti::Octet>('L'),
      static_cast<rti::Octet>('A')};
  rti::VariableLengthData copiedData(callerOwned.data(), callerOwned.size());
  callerOwned[0] = static_cast<rti::Octet>('X');
  require(
      copyBytes(copiedData) == std::vector<std::uint8_t>{'H', 'L', 'A'},
      "VariableLengthData constructor did not copy caller-owned bytes");

  rti::VariableLengthData copiedAgain(copiedData);
  rti::VariableLengthData assigned;
  assigned = copiedData;
  std::vector<rti::Octet> replacement{
      static_cast<rti::Octet>('O'),
      static_cast<rti::Octet>('K')};
  copiedData.setData(replacement.data(), replacement.size());
  require(
      copyBytes(copiedAgain) == std::vector<std::uint8_t>{'H', 'L', 'A'} &&
          copyBytes(assigned) == std::vector<std::uint8_t>{'H', 'L', 'A'} &&
          copyBytes(copiedData) == std::vector<std::uint8_t>{'O', 'K'},
      "VariableLengthData copy and replacement semantics were not independent");

  rti::VariableLengthData borrowedData;
  borrowedData.setDataPointer(callerOwned.data(), callerOwned.size());
  rti::VariableLengthData borrowedCopy(borrowedData);
  callerOwned[1] = static_cast<rti::Octet>('E');
  callerOwned[2] = static_cast<rti::Octet>('W');
  require(
      copyBytes(borrowedData) == std::vector<std::uint8_t>{'X', 'E', 'W'} &&
          copyBytes(borrowedCopy) == std::vector<std::uint8_t>{'X', 'L', 'A'},
      "VariableLengthData borrowed-storage copy semantics were not preserved");

  adoptedDataDeleteCount = 0U;
  auto* adoptedBytes = new rti::Octet[3U]{
      static_cast<rti::Octet>('R'),
      static_cast<rti::Octet>('T'),
      static_cast<rti::Octet>('I')};
  {
    rti::VariableLengthData adoptedData;
    adoptedData.takeDataPointer(
        adoptedBytes,
        3U,
        &deleteAdoptedData);
    require(
        copyBytes(adoptedData) ==
            std::vector<std::uint8_t>{'R', 'T', 'I'},
        "VariableLengthData adopted storage did not preserve its bytes");
    rti::VariableLengthData adoptedCopy(adoptedData);
    std::vector<rti::Octet> adoptedReplacement{
        static_cast<rti::Octet>('!')};
    adoptedData.setData(
        adoptedReplacement.data(),
        adoptedReplacement.size());
    require(
        adoptedDataDeleteCount == 1U &&
            copyBytes(adoptedCopy) ==
                std::vector<std::uint8_t>{'R', 'T', 'I'} &&
            copyBytes(adoptedData) == std::vector<std::uint8_t>{'!'},
        "VariableLengthData adopted storage replacement semantics were not preserved");
  }
  require(
      adoptedDataDeleteCount == 1U,
      "VariableLengthData adopted storage was not released exactly once");

  rti::VariableLengthData emptyData;
  require(
      emptyData.size() == 0U,
      "VariableLengthData default construction did not produce an empty value");
  {
    auto* defaultAdoptedBytes = new rti::Octet[3U]{
        static_cast<rti::Octet>('D'),
        static_cast<rti::Octet>('E'),
        static_cast<rti::Octet>('F')};
    rti::VariableLengthData defaultAdoptedData;
    defaultAdoptedData.takeDataPointer(defaultAdoptedBytes, 3U);
    require(
        defaultAdoptedData.size() == 3U &&
            copyBytes(defaultAdoptedData) ==
                std::vector<std::uint8_t>{'D', 'E', 'F'},
        "VariableLengthData default adopted storage did not preserve its bytes");
  }

}

}  // namespace hla_rti_cpp_tck::detail
