#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#endif

#include <mutex>

namespace rti1516_2025::umbra_binding_detail {

bool UmbraRtiAmbassador::getAttributeScopeAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getAttributeScopeAdvisorySwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Attribute Scope Advisory Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getAttributeScopeAdvisorySwitch(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Attribute Scope Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Attribute Scope Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .attributeScopeAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Scope Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setAttributeScopeAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setAttributeScopeAdvisorySwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Set Attribute Scope Advisory Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->setAttributeScopeAdvisorySwitch(
          std::move(federationName), federateId, switchValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  bool accepted = false;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Attribute Scope Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Attribute Scope Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry().setAttributeScopeAdvisorySwitch(
        *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::attribute_scope_advisory});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Attribute Scope Advisory Switch encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetAttributeScopeAdvisorySwitch",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::boolean,
          L"SwitchValue",
          umbra::detail::formatMomBoolean(switchValue)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Attribute Scope Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getObjectClassRelevanceAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getObjectClassRelevanceAdvisorySwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Object Class Relevance Advisory Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getObjectClassRelevanceAdvisorySwitch(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Object Class Relevance Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Object Class Relevance Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .objectClassRelevanceAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Object Class Relevance Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setObjectClassRelevanceAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setObjectClassRelevanceAdvisorySwitch");
  try {
  bool accepted = false;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Object Class Relevance Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Object Class Relevance Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry()
        .setObjectClassRelevanceAdvisorySwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::object_class_relevance_advisory});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Object Class Relevance Advisory Switch encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetObjectClassRelevanceAdvisorySwitch",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::boolean,
          L"SwitchValue",
          umbra::detail::formatMomBoolean(switchValue)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Object Class Relevance Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAttributeRelevanceAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getAttributeRelevanceAdvisorySwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Attribute Relevance Advisory Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getAttributeRelevanceAdvisorySwitch(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Attribute Relevance Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Attribute Relevance Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .attributeRelevanceAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Attribute Relevance Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setAttributeRelevanceAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setAttributeRelevanceAdvisorySwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Set Attribute Relevance Advisory Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->setAttributeRelevanceAdvisorySwitch(
          std::move(federationName), federateId, switchValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  bool accepted = false;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Attribute Relevance Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Attribute Relevance Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry()
        .setAttributeRelevanceAdvisorySwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::attribute_relevance_advisory});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Attribute Relevance Advisory Switch encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetAttributeRelevanceAdvisorySwitch",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::boolean,
          L"SwitchValue",
          umbra::detail::formatMomBoolean(switchValue)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Attribute Relevance Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getInteractionRelevanceAdvisorySwitch() const {
  auto instrumentationScope = beginRtiCall("getInteractionRelevanceAdvisorySwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Interaction Relevance Advisory Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getInteractionRelevanceAdvisorySwitch(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Interaction Relevance Advisory Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Interaction Relevance Advisory Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .interactionRelevanceAdvisorySwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Interaction Relevance Advisory Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setInteractionRelevanceAdvisorySwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setInteractionRelevanceAdvisorySwitch");
  try {
  bool accepted = false;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Interaction Relevance Advisory Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Interaction Relevance Advisory Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry()
        .setInteractionRelevanceAdvisorySwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::interaction_relevance_advisory});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Interaction Relevance Advisory Switch encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetInteractionRelevanceAdvisorySwitch",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::boolean,
          L"SwitchValue",
          umbra::detail::formatMomBoolean(switchValue)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Interaction Relevance Advisory Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getConveyRegionDesignatorSetsSwitch() const {
  auto instrumentationScope = beginRtiCall("getConveyRegionDesignatorSetsSwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Convey Region Designator Sets Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getConveyRegionDesignatorSetsSwitch(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Convey Region Designator Sets Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Convey Region Designator Sets Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .conveyRegionDesignatorSetsSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Convey Region Designator Sets Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setConveyRegionDesignatorSetsSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setConveyRegionDesignatorSetsSwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Set Convey Region Designator Sets Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->setConveyRegionDesignatorSetsSwitch(
          std::move(federationName), federateId, switchValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  bool accepted = false;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Convey Region Designator Sets Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Convey Region Designator Sets Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry()
        .setConveyRegionDesignatorSetsSwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::convey_region_designator_sets});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Convey Region Designator Sets Switch encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetConveyRegionDesignatorSetsSwitch",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::boolean,
          L"SwitchValue",
          umbra::detail::formatMomBoolean(switchValue)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Convey Region Designator Sets Switch", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
