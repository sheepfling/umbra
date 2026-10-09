#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/embedded_transport.hpp"
#include "internal/observability/service_report_store.hpp"
#include "internal/runtime/ambassador_connection_support.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/rti_initialization_data.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAnoCredentials.h>

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

using namespace service_failure_translation;

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel) {
  auto instrumentationScope = beginRtiCall("connect");
  return connectImpl(federateAmbassador, callbackModel, nullptr, nullptr);
}
ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const& configuration) {
  auto instrumentationScope = beginRtiCall("connect");
  return connectImpl(federateAmbassador, callbackModel, &configuration, nullptr);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    Credentials const& credentials) {
  auto instrumentationScope = beginRtiCall("connect");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Connect cannot be called from within a federate callback.");
  }
  return connectImpl(federateAmbassador, callbackModel, nullptr, &credentials);
}

ConfigurationResult UmbraRtiAmbassador::connect(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const& configuration,
    Credentials const& credentials) {
  auto instrumentationScope = beginRtiCall("connect");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Connect cannot be called from within a federate callback.");
  }
  return connectImpl(federateAmbassador, callbackModel, &configuration, &credentials);
}

ConfigurationResult UmbraRtiAmbassador::connectImpl(
    FederateAmbassador& federateAmbassador,
    CallbackModel callbackModel,
    RtiConfiguration const* configuration,
    Credentials const* credentials) {
  auto instrumentationScope = beginRtiCall("connectImpl");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Connect cannot be called from within a federate callback.");
  }
  validateAmbassadorConnectionCallbackModel(callbackModel);

  // Resolve the connection lifecycle precondition before touching any
  // caller-supplied configuration. In the embedded profile, preparing the
  // real filesystem-backed service-report store may create directories; a
  // second Connect must report AlreadyConnected without validating or
  // mutating that configuration.
  {
    std::scoped_lock lock(mutex_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::not_connected) {
      throw AlreadyConnected(L"The RTI ambassador already has an active connection.");
    }
  }

  std::unique_ptr<rti1516_2025::Authorizer> candidateAuthorizer;
  {
    std::scoped_lock authorizerLock(authorizerMutex_);
    rti1516_2025::Authorizer* authorizer = authorizer_.get();
    if (authorizer == nullptr) {
      try {
        auto authorizerFactory =
            umbra::detail::makeAuthorizerFactoryFromRtiInitializationData(
                configuration);
        if (authorizerFactory) {
          candidateAuthorizer = authorizerFactory->getAuthorizer();
          if (!candidateAuthorizer) {
            throw std::runtime_error(
                "Umbra's configured AuthorizerFactory returned no authorizer.");
          }
          authorizer = candidateAuthorizer.get();
        }
      } catch (std::exception const&) {
        throw RTIinternalError(
            L"Umbra could not load the configured authorization service from RTI Initialization Data.");
      }
    }
    if (authorizer != nullptr) {
      authorizeAmbassadorConnectIfConfigured(*authorizer, credentials);
    } else if (credentials != nullptr) {
      requireNoCredentialsWhenAmbassadorAuthorizationIsDisabled(*credentials);
    }
  }

  bool settingsApplied = false;
  bool fomEditionSettingsApplied = false;
  bool fomEditionSettingsFailedToParse = false;
  std::unique_ptr<umbra::detail::ServiceReportStore> serviceReportStore;
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto processEndpoint = configuredAmbassadorProcessEndpoint(configuration);
  auto const configuredFomEdition = configuredAmbassadorFomStandardEdition(
      configuration,
      fomEditionSettingsApplied,
      fomEditionSettingsFailedToParse);
  // The factory-created runtime always takes the filesystem path. An
  // explicitly constructed internal federation-management test ambassador
  // may instead supply its deterministic store; that construction path is not
  // reachable through the official RTI configuration surface.
  bool const useInjectedServiceReportStore =
      static_cast<bool>(injectedServiceReportStoreForTesting_);
  if (!useInjectedServiceReportStore) {
#endif
    auto const serviceReportDirectory = validateAmbassadorServiceReportDirectory(
        configuredAmbassadorServiceReportDirectory(configuration, settingsApplied));
    serviceReportStore = std::make_unique<umbra::detail::FilesystemServiceReportStore>(
        serviceReportDirectory,
        instrumentation_);
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  }
#endif
  ServiceReportConnectionSnapshot serviceReportConnection;
  serviceReportConnection.callbackModel = callbackModel;
  if (configuration != nullptr) {
    serviceReportConnection.configurationName = configuration->configurationName();
    serviceReportConnection.rtiAddress = configuration->rtiAddress();
    serviceReportConnection.additionalSettings = configuration->additionalSettings();
  }
  if (credentials != nullptr) {
    serviceReportConnection.credentials = *credentials;
  }

  // Construct the truthful result before mutating lifecycle state so a failed
  // allocation cannot leave a partially established connection behind.
  ConfigurationResult result = ignoredAmbassadorConnectionResult();
  auto const additionalSettingsResult =
      fomEditionSettingsFailedToParse
          ? SETTINGS_FAILED_TO_PARSE
          : (settingsApplied || fomEditionSettingsApplied)
                ? SETTINGS_APPLIED
                : SETTINGS_IGNORED;
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpoint) {
    result = ConfigurationResult(
        true,
        true,
        additionalSettingsResult,
        L"Umbra selected the configured private process endpoint; its public service mapping is still being completed.");
  } else
#endif
  if (fomEditionSettingsFailedToParse) {
    result = ConfigurationResult(
        false,
        false,
        SETTINGS_FAILED_TO_PARSE,
        L"Umbra could not parse an optional additional-settings value; the default embedded configuration was used.");
  } else if (settingsApplied || fomEditionSettingsApplied) {
    result = ConfigurationResult(
        true, false, SETTINGS_APPLIED,
        fomEditionSettingsApplied
            ? L"Umbra applied the configured FOM model edition for the embedded compatibility profile."
            : L"Umbra applied serviceReportDirectory for embedded service-report files.");
  }
  auto callbackSession =
      std::make_shared<CallbackSession>(federateAmbassador, instrumentation_);

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  std::unique_ptr<umbra::detail::ProcessFederationClient>
      processFederationClient;
  if (processEndpoint) {
    try {
      processFederationClient =
          std::make_unique<umbra::detail::ProcessFederationClient>(
              *processEndpoint,
              umbra::detail::TransportEndpointIdentity{
                  ambassadorProcessEndpointIdentity(configuration),
                  nextAmbassadorProcessEndpointSessionId()},
              instrumentation_,
              [this](std::wstring faultDescription) {
                handleProcessTransportFailure(std::move(faultDescription));
              });
      // Borrow the official ambassador's callback controls so the process
      // endpoint observes the same enable/disable and dispatch model as the
      // embedded endpoint.  The client never owns the recipient.
      processFederationClient->attachCallbackBridge(callbacks_, callbackSession);
      processFederationClient->setTimeRoleEnableCompletionHandlers(
          [this] {
            std::scoped_lock lock(mutex_);
            processTimeRegulationCallbackPending_ = false;
          },
          [this] {
            std::scoped_lock lock(mutex_);
            processTimeConstrainedCallbackPending_ = false;
          });
    } catch (std::exception const& error) {
      throw ConnectionFailed(wideAscii(error.what()));
    }
  }
#endif

  std::scoped_lock lock(mutex_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::not_connected) {
    throw AlreadyConnected(L"The RTI ambassador already has an active connection.");
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  std::shared_ptr<umbra::detail::TransportConnection> transportConnection;
  if (!processFederationClient) {
    transportConnection = umbra::detail::embeddedTransportHub().connect(
        static_cast<RTIambassador*>(this),
        [this](std::wstring faultDescription) {
          handleEmbeddedTransportFailure(std::move(faultDescription));
        },
        [this](std::wstring reasonForResign) {
          return handleEmbeddedFederateResignation(std::move(reasonForResign));
        },
        instrumentation_);
  }
  if (useInjectedServiceReportStore) {
    serviceReportStore = std::move(injectedServiceReportStoreForTesting_);
    if (!serviceReportStore) {
      throw RTIinternalError(
          L"Umbra's internal service-report test store was already consumed.");
    }
  }
#endif
  if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::connect) !=
      umbra::detail::FederateLifecycleResult::applied) {
    throw AlreadyConnected(L"The RTI ambassador already has an active connection.");
  }
  if (candidateAuthorizer) {
    std::scoped_lock authorizerLock(authorizerMutex_);
    authorizer_ = std::move(candidateAuthorizer);
  }

  callbackSession_ = std::move(callbackSession);
  callbackModel_ = callbackModel;
  callbacks_->configure(ambassadorCallbackDispatchModel(callbackModel));
  serviceReportStore_ = std::move(serviceReportStore);
  serviceReportConnection_ = std::move(serviceReportConnection);
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  fomStandardEdition_ = configuredFomEdition;
  transportConnection_ = std::move(transportConnection);
  processFederationClient_ = std::move(processFederationClient);
  processEndpointActive_ = static_cast<bool>(processFederationClient_);
  processTimeRegulationCallbackPending_ = false;
  processTimeConstrainedCallbackPending_ = false;
  activeServiceReportStoreIsTestOnly_ = useInjectedServiceReportStore;
  startPeriodicMomScheduler();
#endif
  return result;
}

}  // namespace rti1516_2025::umbra_binding_detail
