#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_federation_lifecycle_support.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

namespace {

std::wstring serviceReportCallbackModelName(CallbackModel callbackModel) {
  switch (callbackModel) {
    case HLA_IMMEDIATE:
      return L"HLA_IMMEDIATE";
    case HLA_EVOKED:
      return L"HLA_EVOKED";
  }
  throw UnsupportedCallbackModel(
      L"Umbra supports only HLA_IMMEDIATE and HLA_EVOKED callback models.");
}

std::uint64_t nextServiceReportJoinIdentifier() {
  static std::atomic_uint64_t next{1U};
  auto const sequence = next.fetch_add(1U, std::memory_order_relaxed);
  auto const ticks = static_cast<std::uint64_t>(
      std::chrono::system_clock::now().time_since_epoch().count());
  // The local sequence separates joins in one process even when clocks have
  // the same resolution; mixing wall-clock ticks reduces cross-process
  // collision likelihood without assigning the path format to callers.
  return ticks ^ (sequence + 0x9e3779b97f4a7c15ULL + (ticks << 6U) + (ticks >> 2U));
}

// A report pathname is allocated before the joined-federate MOM object and
// service-report routes are committed. Keep that allocation transactional:
// an exceptional Join path must not leave an orphan file that advertises a
// joined-federate lifetime which never became visible. Once the lifecycle
// transition succeeds, commit() deliberately disarms cleanup; resignation
// keeps the completed lifetime's report file available for the administrator.
class PendingServiceReportFileCleanup final {
 public:
  PendingServiceReportFileCleanup() = default;
  PendingServiceReportFileCleanup(PendingServiceReportFileCleanup const&) = delete;
  PendingServiceReportFileCleanup& operator=(PendingServiceReportFileCleanup const&) = delete;

  ~PendingServiceReportFileCleanup() {
    if (!location_) {
      return;
    }
    std::error_code ignored;
    std::filesystem::remove(*location_, ignored);
  }

  void arm(std::filesystem::path location) {
    if (!location.empty()) {
      location_ = std::move(location);
    }
  }

  void commit() noexcept { location_.reset(); }

 private:
  std::optional<std::filesystem::path> location_;
};




std::vector<std::wstring> firstFomModuleDesignatorsSpecifiedAtJoin(
    std::vector<umbra::detail::PrevalidatedFomModule> const& modules) {
  std::set<std::filesystem::path> seenSources;
  std::vector<std::wstring> result;
  result.reserve(modules.size());
  for (auto const& module : modules) {
    if (module.kind == umbra::detail::FomModuleKind::fom &&
        seenSources.insert(module.sourcePath).second) {
      result.push_back(module.designator);
    }
  }
  return result;
}

std::wstring formatJoinedFederateServiceReportInitialRecord(
    ServiceReportConnectionSnapshot const& connection,
    std::wstring const& federationName,
    umbra::detail::FederationDefinition const& definition,
    umbra::detail::FederateMembership const& membership,
    bool autoProvide,
    std::vector<umbra::detail::PrevalidatedFomModule> const& fomModulesSpecifiedAtJoin) {
  umbra::detail::MomServiceReportInitialRecord record;
  record.callbackModel = serviceReportCallbackModelName(connection.callbackModel);
  record.configurationName = connection.configurationName;
  record.rtiAddress = connection.rtiAddress;
  record.additionalSettings = connection.additionalSettings;
  if (connection.credentials) {
    auto const data = connection.credentials->getData();
    auto bytes = umbra::detail::variable_length_data_2025::copyBytes(data);
    record.credentials = umbra::detail::MomServiceReportCredentials{
        connection.credentials->getType(), std::move(bytes)};
  }

  record.federationName = federationName;
  record.rtiVersion = std::wstring{kUmbraRtiVersion};
  record.timeImplementationName = definition.logicalTimeImplementationName;
  record.autoProvide = autoProvide;
  record.federateHandle = makeFederateHandle(membership.id).toString();
  record.federateName = membership.name;
  record.federateType = membership.type;
  // This in-process profile has no transport-host discovery layer yet.  The
  // stable profile identity is more truthful than manufacturing a network
  // host value; a remote transport will supply the actual host before this
  // behavior is promoted as MOM conformance evidence.
  record.federateHost = std::wstring{kUmbraEmbeddedFederateHost};

  for (auto const& module : definition.fomModules) {
    if (module.kind == umbra::detail::FomModuleKind::mim) {
      record.mimDesignator = module.designator;
    } else {
      record.federationFomModuleDesignators.push_back(module.designator);
    }
  }
  record.federateFomModuleDesignators =
      firstFomModuleDesignatorsSpecifiedAtJoin(fomModulesSpecifiedAtJoin);
  return umbra::detail::formatMomServiceReportInitialRecord(record);
}

}  // namespace

void UmbraRtiAmbassador::createFederationExecution(
    std::wstring const& federationName,
    std::wstring const& fomModule,
    std::wstring const& logicalTimeImplementationName) {
  auto instrumentationScope = beginRtiCall("createFederationExecution");
  createFederationExecution(
      federationName,
      std::vector<std::wstring>{fomModule},
      logicalTimeImplementationName);
}
void UmbraRtiAmbassador::createFederationExecution(
    std::wstring const& federationName,
    std::vector<std::wstring> const& fomModules,
    std::wstring const& logicalTimeImplementationName) {
  auto instrumentationScope = beginRtiCall("createFederationExecution");
  try {
  authorizeFederationOperationForConnectedFederate(
      federationName, L"Create Federation Execution");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (processEndpointActive_) {
      if (!processFederationClient_) {
        throw RTIinternalError(
            L"Umbra's process endpoint has no active federation client.");
      }
      try {
        // Keep the official FOM and logical-time arguments at the process
        // boundary.  The service delegates validation/composition to its
        // injected standards-derived coordinator before registry commit.
        if (logicalTimeImplementationName.empty()) {
          // Preserve the original server-owned process fixture payload for
          // the binding's default-time overload.  Explicit logical-time
          // selections use the standards-derived FOM transport below; a
          // future capability handshake can remove this legacy branch.
          processFederationClient_->createFederationExecution(federationName);
        } else {
          processFederationClient_->createFederationExecution(
              federationName,
              fomModules,
              std::nullopt,
              logicalTimeImplementationName);
        }
      } catch (std::exception const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
      return;
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);

  auto preparation = embeddedFederationCoordinator().prepareCreate(
      fomModules,
      std::nullopt,
      logicalTimeImplementationName,
      fomStandardEdition_);
  if (!preparation.accepted()) {
    throwPreparationFailure(preparation);
  }

  auto created = embeddedFederationRegistry().create(federationName, std::move(*preparation.definition));
  switch (created.status) {
    case umbra::detail::FederationRegistryStatus::applied:
      return;
    case umbra::detail::FederationRegistryStatus::federation_already_exists:
      throw FederationExecutionAlreadyExists(
          L"A federation execution with the supplied name already exists.");
    case umbra::detail::FederationRegistryStatus::invalid_request:
    case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
    case umbra::detail::FederationRegistryStatus::federates_currently_joined:
    case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
    case umbra::detail::FederationRegistryStatus::federate_not_member:
      throw RTIinternalError(L"Umbra could not commit the prepared federation definition.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation-registry creation outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Create Federation Execution", exception);
    throw;
  }
}
void UmbraRtiAmbassador::createFederationExecutionWithMIM(
    std::wstring const& federationName,
    std::vector<std::wstring> const& fomModules,
    std::wstring const& mimModule,
    std::wstring const& logicalTimeImplementationName) {
  auto instrumentationScope = beginRtiCall("createFederationExecutionWithMIM");
  try {
  authorizeFederationOperationForConnectedFederate(
      federationName, L"Create Federation Execution With MIM");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (processEndpointActive_) {
      if (!processFederationClient_) {
        throw RTIinternalError(
            L"Umbra's process endpoint has no active federation client.");
      }
      try {
        processFederationClient_->createFederationExecution(
            federationName,
            fomModules,
            std::optional<std::wstring>{mimModule},
            logicalTimeImplementationName);
      } catch (std::exception const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
      return;
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);

  auto preparation = embeddedFederationCoordinator().prepareCreate(
      fomModules,
      std::optional<std::wstring>{mimModule},
      logicalTimeImplementationName,
      fomStandardEdition_);
  if (!preparation.accepted()) {
    throwPreparationFailure(preparation);
  }

  auto created = embeddedFederationRegistry().create(federationName, std::move(*preparation.definition));
  switch (created.status) {
    case umbra::detail::FederationRegistryStatus::applied:
      return;
    case umbra::detail::FederationRegistryStatus::federation_already_exists:
      throw FederationExecutionAlreadyExists(
          L"A federation execution with the supplied name already exists.");
    case umbra::detail::FederationRegistryStatus::invalid_request:
    case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
    case umbra::detail::FederationRegistryStatus::federates_currently_joined:
    case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
    case umbra::detail::FederationRegistryStatus::federate_not_member:
      throw RTIinternalError(L"Umbra could not commit the prepared federation definition.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation-registry creation outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Create Federation Execution With MIM", exception);
    throw;
  }
}

void UmbraRtiAmbassador::destroyFederationExecution(std::wstring const& federationName) {
  auto instrumentationScope = beginRtiCall("destroyFederationExecution");
  try {
  authorizeFederationOperationForConnectedFederate(
      federationName, L"Destroy Federation Execution");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (processEndpointActive_) {
      if (!processFederationClient_) {
        throw RTIinternalError(
            L"Umbra's process endpoint has no active federation client.");
      }
      umbra::detail::ProcessFederationDestroyResult destroyed;
      try {
        destroyed = processFederationClient_->destroyFederationExecution(
            federationName);
      } catch (std::exception const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
      switch (destroyed.status) {
        case umbra::detail::ProcessFederationDestroyStatus::applied:
          eraseAmbassadorUpdateRateHistoryForFederation(federationName);
          return;
        case umbra::detail::ProcessFederationDestroyStatus::federation_does_not_exist:
          throw FederationExecutionDoesNotExist(
              L"The supplied federation execution does not exist.");
        case umbra::detail::ProcessFederationDestroyStatus::federates_currently_joined:
          throw FederatesCurrentlyJoined(
              L"A federation execution cannot be destroyed while federates remain joined.");
        case umbra::detail::ProcessFederationDestroyStatus::invalid_request:
          throw RTIinternalError(
              L"Umbra could not destroy the federation execution.");
      }
      throw RTIinternalError(
          L"Umbra encountered an unknown process federation destruction outcome.");
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);

  auto destroyed = embeddedFederationRegistry().destroy(federationName);
  switch (destroyed.status) {
    case umbra::detail::FederationRegistryStatus::applied:
      // A later execution may reuse federate/object handle values; do not
      // carry wall-clock admission history across this federation lifetime.
      // Keep independent live executions isolated: update-rate reduction is
      // keyed by federation as well as recipient/object/subscription state.
      eraseAmbassadorUpdateRateHistoryForFederation(federationName);
      return;
    case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
      throw FederationExecutionDoesNotExist(L"The supplied federation execution does not exist.");
    case umbra::detail::FederationRegistryStatus::federates_currently_joined:
      throw FederatesCurrentlyJoined(
          L"A federation execution cannot be destroyed while federates remain joined.");
    case umbra::detail::FederationRegistryStatus::invalid_request:
    case umbra::detail::FederationRegistryStatus::federation_already_exists:
    case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
    case umbra::detail::FederationRegistryStatus::federate_not_member:
      throw RTIinternalError(L"Umbra could not destroy the federation execution.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown federation-registry destruction outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Destroy Federation Execution", exception);
    throw;
  }
}

FederateHandle UmbraRtiAmbassador::joinFederationExecutionImpl(
    std::optional<std::wstring> requestedFederateName,
    std::wstring const& federateType,
    std::wstring const& federationName,
    std::vector<std::wstring> const& additionalFomModules) {
  auto instrumentationScope = beginRtiCall("joinFederationExecutionImpl");
  try {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Join Federation Execution cannot be called from within a federate callback.");
  }

  // The optional-name Join overload has no caller-supplied name to authorize.
  // The official Authorizer API represents that absence with an empty string;
  // the RTI-assigned name is selected by the membership transaction later.
  authorizeFederateOperationForConnectedFederate(
      federationName,
      requestedFederateName.value_or(std::wstring{}),
      federateType);

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (processEndpointActive_) {
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined) {
        throw FederateAlreadyExecutionMember(
            L"This RTI ambassador is already joined to a federation execution.");
      }
      if (!processFederationClient_) {
        throw RTIinternalError(
            L"Umbra's process endpoint has no active federation client.");
      }
      umbra::detail::ProcessFederationJoinConnectionSnapshot connection;
      connection.callbackModel = serviceReportCallbackModelName(
          serviceReportConnection_.callbackModel);
      connection.configurationName = serviceReportConnection_.configurationName;
      connection.rtiAddress = serviceReportConnection_.rtiAddress;
      connection.additionalSettings = serviceReportConnection_.additionalSettings;
      if (serviceReportConnection_.credentials) {
        auto const data = serviceReportConnection_.credentials->getData();
        auto bytes = umbra::detail::variable_length_data_2025::copyBytes(data);
        connection.credentials = std::make_pair(
            serviceReportConnection_.credentials->getType(), std::move(bytes));
      }
      umbra::detail::ProcessFederationJoinResult joined;
      try {
        joined = processFederationClient_->joinFederationExecution(
            federationName,
            federateType,
            std::move(requestedFederateName),
            additionalFomModules,
            std::move(connection));
      } catch (std::exception const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
      if (joined.federateId == 0U || joined.federateName.empty()) {
        throw RTIinternalError(
            L"The private process endpoint returned an invalid federate identity.");
      }
      if (joined.logicalTimeImplementationName.empty()) {
        throw RTIinternalError(
            L"The process service did not return the selected logical-time implementation.");
      }
      if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::join) !=
          umbra::detail::FederateLifecycleResult::applied) {
        throw RTIinternalError(
            L"Umbra could not complete the process Join Federation Execution transition.");
      }
      joinedFederationName_ = federationName;
      joinedFederateId_ = joined.federateId;
      processLogicalTimeImplementationName_ =
          std::move(joined.logicalTimeImplementationName);
      processServiceReportFile_ = joined.reportServiceFile.empty()
          ? std::nullopt
          : std::optional<std::filesystem::path>{
                std::filesystem::path(joined.reportServiceFile)};
      return makeFederateHandle(joined.federateId);
    }
  }
#endif

  FederateHandle result;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::vector<umbra::detail::SynchronizationPointAnnouncement>
      newlyJoinedSynchronizationAnnouncements;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      newlyEligibleOwnershipAssumptions;
  std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient>
      newlyJoinedMomDiscoveries;
  std::uint64_t joinedFederateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined) {
      throw FederateAlreadyExecutionMember(
          L"This RTI ambassador is already joined to a federation execution.");
    }

    auto currentDefinition = embeddedFederationRegistry().definitionFor(federationName);
    if (!currentDefinition) {
      throw FederationExecutionDoesNotExist(L"The supplied federation execution does not exist.");
    }

    auto callbackSession = callbackSession_;
    auto callbackDispatcher = callbacks_;
    if (!callbackSession || !callbackDispatcher) {
      throw RTIinternalError(
          L"The embedded connection has no federate ambassador callback recipient.");
    }
    // Register this endpoint as part of the registry's membership transaction.
    // In particular, an additional-FOM join must not commit its definition and
    // membership before an allocation failure can prevent federate callback delivery.
    auto callbackRoute = ambassadorMakeFederateCallbackRoute(
        callbackDispatcher,
        callbackSession);

    umbra::detail::FederationJoinResult joined;
    std::shared_ptr<umbra::detail::FederateTimeState> timeState;
    std::vector<umbra::detail::PrevalidatedFomModule> fomModulesSpecifiedAtJoin;
    if (additionalFomModules.empty()) {
      // Allocate and validate the initial logical-time state before the registry
      // commits membership, so a factory/allocation failure cannot leave a
      // joined federate without the required initial time position.
      timeState = ambassadorMakeFederateTimeState( *currentDefinition);
      auto timeAdvanceGrantDispatchFactory = ambassadorMakeTimeAdvanceGrantDispatchFactory(
          callbackDispatcher,
          callbackSession,
          timeState,
          federationName);
      auto timeRoleEnableDispatchFactory = ambassadorMakeTimeRoleEnableDispatchFactory(
          callbackDispatcher,
          callbackSession,
          timeState,
          federationName);
      joined = embeddedFederationRegistry().joinWithTimeState(
          federationName,
          timeState,
          federateType,
          std::move(requestedFederateName),
          std::move(callbackRoute),
          std::move(timeAdvanceGrantDispatchFactory),
          std::move(timeRoleEnableDispatchFactory));
    } else {
      auto preparation = embeddedFederationCoordinator().prepareAdditionalModules(
          *currentDefinition,
          additionalFomModules);
      if (!preparation.accepted()) {
        throwPreparationFailure(preparation);
      }
      if (!preparation.definition ||
          preparation.definition->fomModules.size() < additionalFomModules.size()) {
        throw RTIinternalError(
            L"Umbra could not retain the validated FOM modules supplied at Join.");
      }
      auto const firstJoinModule = preparation.definition->fomModules.end() -
          static_cast<std::ptrdiff_t>(additionalFomModules.size());
      fomModulesSpecifiedAtJoin.assign(
          firstJoinModule,
          preparation.definition->fomModules.end());
      for (auto const& module : fomModulesSpecifiedAtJoin) {
        if (module.kind != umbra::detail::FomModuleKind::fom) {
          throw RTIinternalError(
              L"Umbra could not retain a valid FOM-module designator for Join.");
        }
      }
      timeState = ambassadorMakeFederateTimeState( *preparation.definition);
      auto timeAdvanceGrantDispatchFactory = ambassadorMakeTimeAdvanceGrantDispatchFactory(
          callbackDispatcher,
          callbackSession,
          timeState,
          federationName);
      auto timeRoleEnableDispatchFactory = ambassadorMakeTimeRoleEnableDispatchFactory(
          callbackDispatcher,
          callbackSession,
          timeState,
          federationName);
      joined = embeddedFederationRegistry().joinWithDefinitionAndTimeState(
          federationName,
          std::move(*preparation.definition),
          timeState,
          federateType,
          std::move(requestedFederateName),
          std::move(callbackRoute),
          std::move(timeAdvanceGrantDispatchFactory),
          std::move(timeRoleEnableDispatchFactory));
    }

    switch (joined.status) {
      case umbra::detail::FederationRegistryStatus::applied:
        break;
      case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
        throw FederationExecutionDoesNotExist(L"The supplied federation execution does not exist.");
      case umbra::detail::FederationRegistryStatus::federate_name_already_in_use:
        throw FederateNameAlreadyInUse(
            L"The supplied federate name is already in use in this federation execution.");
      case umbra::detail::FederationRegistryStatus::invalid_request:
      case umbra::detail::FederationRegistryStatus::federation_already_exists:
      case umbra::detail::FederationRegistryStatus::federates_currently_joined:
      case umbra::detail::FederationRegistryStatus::federate_not_member:
        throw RTIinternalError(L"Umbra could not commit the federate membership.");
    }
    if (!joined.membership) {
      throw RTIinternalError(L"Umbra could not complete the federate membership transaction.");
    }

    // Report-file allocation is part of the joined-federate lifetime, not a
    // lazy side effect of a later switch update.  The registry has supplied
    // the final membership identity at this point; if filesystem creation
    // fails, remove that new membership before exposing a successful Join.
    auto rollbackJoinedMembership = [&]() {
      auto const rolledBack = embeddedFederationRegistry().resign(
          federationName,
          joined.membership->id,
          NO_ACTION);
      if (timeState) {
        timeState->deactivate();
      }
      if (rolledBack.status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
          L"Umbra could not roll back a joined federate after service-report setup failed.");
      }
    };
    PendingServiceReportFileCleanup pendingReportFileCleanup;

    auto joinedDefinition = embeddedFederationRegistry().definitionFor(federationName);
    auto const autoProvide = embeddedFederationRegistry().autoProvideSwitchFor(
        federationName,
        joined.membership->id);
    if (!joinedDefinition || !autoProvide || !serviceReportStore_) {
      rollbackJoinedMembership();
      throw RTIinternalError(
          L"Umbra could not initialize the joined federate's service-report state.");
    }

    // The public runtime is IEEE 1516.1-2025, while this opt-in model lane
    // accepts an IEEE 1516.2-2010 catalog for registration-only use.  The
    // 2010 MIM does not contain the 2025 MOM federation attributes used by
    // the runtime's RTI-owned MOM object implementation.  Keep that
    // compatibility boundary explicit: ordinary FOM lookup, publication,
    // registration, and service-report file lifetime still work, but the
    // 2025 MOM object projection is not synthesized from a 2010 MIM.
    bool const legacyFomCompatibility =
        joinedDefinition->standardEdition ==
        umbra::detail::FomStandardEdition::ieee1516_2010;

    // The federation-execution MOM object is independent of any one joined
    // federate's report-file lifetime.  Establish it once at the first
    // successful Join so later standard subscriptions can discover the
    // static HLAmanager.HLAfederation attributes through the same RTI-owned
    // object callback path as HLAmanager.HLAfederate.
    if (!legacyFomCompatibility) {
      auto const federationMomStatus = embeddedFederationRegistry().establishFederationMomObject(
          federationName,
          std::wstring{kUmbraRtiVersion},
          umbra::detail::hla::wide::mom::standard_mim);
      if (federationMomStatus != umbra::detail::JoinedFederateMomObjectStatus::applied &&
          federationMomStatus != umbra::detail::JoinedFederateMomObjectStatus::already_established) {
        rollbackJoinedMembership();
        throw RTIinternalError(
            L"Umbra could not establish the federation execution's RTI-owned MOM object state.");
      }
    }

    std::optional<JoinedServiceReportState> pendingServiceReport;
    try {
      auto const joinIdentifier = nextServiceReportJoinIdentifier();
      auto const initialRecord = formatJoinedFederateServiceReportInitialRecord(
          serviceReportConnection_,
          federationName,
          *joinedDefinition,
          *joined.membership,
          *autoProvide,
          fomModulesSpecifiedAtJoin);
      auto writer = serviceReportStore_->createForJoinedFederate({
          federationName,
          joined.membership->name,
          joined.membership->id,
          joinIdentifier,
          initialRecord,
      });
      if (!writer ||
          (!activeServiceReportStoreIsTestOnly_ && writer->location().empty())) {
        throw RTIinternalError(
            L"Umbra's runtime service-report store did not return a filesystem location.");
      }
      auto location = writer->location();
      pendingReportFileCleanup.arm(location);
      auto endpoint = std::make_shared<JoinedServiceReportEndpoint>();
      endpoint->writer = std::move(writer);
      if (!activeServiceReportStoreIsTestOnly_ && !legacyFomCompatibility) {
        // The production writer has now allocated the one immutable location
        // required for this joined-federate lifetime. Establish the private
        // MIM object from that exact value before the Join becomes visible;
        // test-only memory stores intentionally do not invent a public-facing
        // report-file designator.
        auto const momObjectStatus = embeddedFederationRegistry().establishJoinedFederateMomObject(
            federationName,
            joined.membership->id,
            {
                std::wstring{kUmbraEmbeddedFederateHost},
                std::wstring{kUmbraRtiVersion},
                fomModulesSpecifiedAtJoin,
                location.wstring(),
            });
        if (momObjectStatus != umbra::detail::JoinedFederateMomObjectStatus::applied) {
          throw RTIinternalError(
              L"Umbra could not establish the joined federate's RTI-owned MOM object state.");
        }
      }
      pendingServiceReport.emplace(JoinedServiceReportState{
          joined.membership->id,
          joinIdentifier,
          std::move(location),
          endpoint,
      });
      // Attach the writer only after it has selected the immutable joined
      // file. The weak route lets RTI-initiated services report at recipient
      // federates without retaining their endpoint beyond resignation.
      auto const routeStatus = embeddedFederationRegistry().setServiceReportRoute(
          federationName,
          joined.membership->id,
          ambassadorMakeFederateServiceReportRoute(
              endpoint,
              federationName,
              joined.membership->id));
      if (routeStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"Umbra could not attach the joined federate's service-report route.");
      }
      auto const publicRouteStatus = embeddedFederationRegistry().setPublicServiceReportRoute(
          federationName,
          joined.membership->id,
          ambassadorMakeFederatePublicServiceReportRoute(
              federationName,
              joined.membership->id));
      if (publicRouteStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"Umbra could not attach the joined federate's public service-report route.");
      }
      // Move the fully constructed lifetime state into the ambassador before
      // changing the lifecycle. If this allocation/move were ever to fail,
      // the catch below can still roll the registry membership and route back.
      joinedServiceReport_ = std::move(pendingServiceReport);
    } catch (...) {
      rollbackJoinedMembership();
      throw RTIinternalError(
          L"Umbra could not create the configured service-report file for the joined federate.");
    }

    if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::join) !=
        umbra::detail::FederateLifecycleResult::applied) {
      joinedServiceReport_.reset();
      rollbackJoinedMembership();
      throw RTIinternalError(L"Umbra could not complete the Join Federation Execution transition.");
    }
    pendingReportFileCleanup.commit();

    joinedFederationName_ = federationName;
    joinedFederateId_ = joined.membership->id;
    joinedFederateId = joined.membership->id;
    federateTimeState_ = std::move(timeState);
    result = makeFederateHandle(joined.membership->id);

    // The RTI-owned joined-federate MOM object was established before the
    // lifecycle transition. Existing subscribers may therefore need a
    // discovery now that this joined-federate lifetime is visible.
    if (auto const momObject = embeddedFederationRegistry().joinedFederateMomObjectFor(
            federationName,
            joined.membership->id)) {
      newlyJoinedMomDiscoveries = embeddedFederationRegistry()
          .planJoinedFederateMomObjectDiscoveriesForInstance(
              federationName,
              momObject->objectInstanceHandle);
    }

    auto synchronizationAnnouncements =
        embeddedFederationRegistry().announcePendingSynchronizationPoints(
            federationName,
            joined.membership->id);
    if (synchronizationAnnouncements.status ==
        umbra::detail::SynchronizationPointAnnouncementStatus::applied) {
      newlyJoinedSynchronizationAnnouncements =
          std::move(synchronizationAnnouncements.announcements);
    }

    // A newly joined federate is a possible future assumption recipient. It
    // normally becomes eligible only after discovery/publication, but run the
    // same registry planner at the membership boundary so a compatible
    // restored/extended definition cannot bypass the search state.
    newlyEligibleOwnershipAssumptions =
        embeddedFederationRegistry().planAttributeOwnershipAssumptionsForFederate(
            federationName,
            joined.membership->id);

    // An additional FOM may alter federation-wide time metadata such as the
    // NRG switch. Re-evaluate existing private TARs only after the new
    // definition and membership have committed, then submit the actions after
    // releasing the runtime locks below.
    auto scheduled = embeddedFederationRegistry().reevaluateTimeAdvanceGrants(federationName);
    if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
      newlyEligible = std::move(scheduled.dispatches);
    }
  }
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
  submitAmbassadorSynchronizationPointAnnouncements(
      std::move(newlyJoinedSynchronizationAnnouncements));
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(newlyEligibleOwnershipAssumptions),
      federationName,
      VariableLengthData());
  queueAmbassadorObjectInstanceDiscoveries(
      std::move(newlyJoinedMomDiscoveries), federationName);
  // Join Federation Execution is an explicit HLAfederateState conditional
  // update boundary.  Queue it after discovery so an immediate subscriber
  // observes the object before its state reflection, while the conditional
  // helper still snapshots ActiveFederate from the committed ledger.
  queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      joinedFederateId,
      {umbra::detail::hla::utf8::mom::federate_state});
  queueAmbassadorFederationMomConditionalAttributeUpdate(
      federationName,
      {umbra::detail::hla::utf8::mom::federates_in_federation});
  if (!additionalFomModules.empty()) {
    queueAmbassadorFederationMomConditionalAttributeUpdate(
        federationName,
        {umbra::detail::hla::utf8::mom::fom_module_designator_list, umbra::detail::hla::utf8::mom::current_fdd});
  }
  return result;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Join Federation Execution", exception);
    throw;
  }
}

void UmbraRtiAmbassador::resignFederationExecution(ResignAction resignAction) {
  auto instrumentationScope = beginRtiCall("resignFederationExecution");
  try {
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Resign Federation Execution cannot be called from within a federate callback.");
  }

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (processEndpointActive_) {
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"This RTI ambassador is not a member of a federation execution.");
      }
      requireAmbassadorValidResignAction(resignAction);
      if (!processFederationClient_) {
        throw RTIinternalError(
            L"Umbra's process endpoint has no active federation client.");
      }
      try {
        processFederationClient_->resignFederationExecution(
            *joinedFederationName_, *joinedFederateId_, resignAction);
      } catch (std::exception const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
      if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::resign) !=
          umbra::detail::FederateLifecycleResult::applied) {
        throw RTIinternalError(
            L"Umbra could not complete the process Resign Federation Execution transition.");
      }
      joinedFederationName_.reset();
      joinedFederateId_.reset();
      processLogicalTimeImplementationName_.reset();
      processServiceReportFile_.reset();
      return;
    }
  }
#endif

  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::vector<umbra::detail::FederationSynchronizedNotification>
      synchronizationNotifications;
  std::vector<umbra::detail::FederationSaveNotification> saveNotifications;
  std::vector<umbra::detail::FederationRestoreNotification> restoreNotifications;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> resignObjectRemovals;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      resignOwnershipAssumptions;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem>
      resignOwnershipAcquisitionWorkItems;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::wstring federationName;
  std::uint64_t reportedFederateId = 0;
  umbra::detail::FederationRegistryResult resigned;
  bool finalServiceReportAppendFailed = false;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"This RTI ambassador is not a member of a federation execution.");
    }
    requireAmbassadorValidResignAction(resignAction);

    federationName = *joinedFederationName_;
    reportedFederateId = *joinedFederateId_;
    resigned = embeddedFederationRegistry()
        .resignWithFinalServiceReportReservation(
        federationName,
        *joinedFederateId_,
        resignAction,
        static_cast<std::uint16_t>(
            umbra::detail::MomServiceType::federation_management),
        true);
    if (resigned.status == umbra::detail::FederationRegistryStatus::federate_not_member ||
        resigned.status == umbra::detail::FederationRegistryStatus::federation_does_not_exist) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    if (resigned.status == umbra::detail::FederationRegistryStatus::invalid_resign_action) {
      throw InvalidResignAction(
          L"The supplied resign action is not an IEEE 1516.1 ResignAction value.");
    }
    if (resigned.status ==
        umbra::detail::FederationRegistryStatus::ownership_acquisition_pending) {
      throw OwnershipAcquisitionPending(
          L"Resign Federation Execution cannot leave an ownership acquisition pending.");
    }
    if (resigned.status == umbra::detail::FederationRegistryStatus::federate_owns_attributes) {
      throw FederateOwnsAttributes(
          L"Resign Federation Execution cannot leave instance attributes owned by the federate.");
    }
    if (resigned.status != umbra::detail::FederationRegistryStatus::applied ||
        lifecycle_.apply(umbra::detail::FederateLifecycleEvent::resign) !=
            umbra::detail::FederateLifecycleResult::applied) {
      throw RTIinternalError(L"Umbra could not complete the Resign Federation Execution transition.");
    }
    if (resigned.finalServiceReportFileSerialNumber.has_value()) {
      // Section 4.12 supplies one action argument.  Section 11.5.1 leaves
      // descriptive argument text implementation-dependent; Umbra uses the
      // corresponding Table 20 MIM parameter spelling while preserving the
      // Table 5 type-44 ResignAction encoding and official enum value.
      try {
        appendReservedSuccessfulVoidServiceReportToFile(
            *resigned.finalServiceReportFileSerialNumber,
            L"ResignFederationExecution",
            {{umbra::detail::MomArgumentType::resign_action,
              L"HLAresignAction",
              umbra::detail::formatMomResignAction(resignAction)}});
      } catch (RTIinternalError const&) {
        // Membership is already irrevocably removed.  Finish tearing down the
        // local joined-federate lifetime and submit its surviving work before
        // surfacing the deterministic file error; do not retain a stale
        // writer or silently replace it with an in-memory sink.
        finalServiceReportAppendFailed = true;
      }
    }
    synchronizationNotifications = std::move(resigned.synchronizationNotifications);
    saveNotifications = std::move(resigned.saveNotifications);
    restoreNotifications = std::move(resigned.restoreNotifications);
    resignObjectRemovals.reserve(resigned.resignObjectRemovals.size());
    for (auto& removal : resigned.resignObjectRemovals) {
      resignObjectRemovals.push_back({
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          std::move(removal.callbackRoute),
          std::move(removal.serviceReportRoute),
          removal.rtiOwnedMomObject,
      });
    }
    resignOwnershipAssumptions.reserve(resigned.resignOwnershipAssumptions.size());
    for (auto& assumption : resigned.resignOwnershipAssumptions) {
      resignOwnershipAssumptions.push_back({
          assumption.receivingFederateId,
          assumption.objectInstanceHandle,
          std::move(assumption.attributeHandles),
          std::move(assumption.callbackRoute),
      });
    }
    resignOwnershipAcquisitionWorkItems =
        std::move(resigned.resignOwnershipAcquisitionWorkItems);
    // Membership removal can itself eliminate the last active subscriber for
    // another publisher. Compute the ordinary declaration advisories while
    // the registry still has the post-resign state, but queue them only after
    // all federation and ambassador locks have been released.
    declarationAdvisories = embeddedFederationRegistry()
        .planDeclarationAdvisories(federationName);

    if (federateTimeState_) {
      federateTimeState_->deactivate();
    }
    federateTimeState_.reset();
    // Do not retain a report writer beyond the resigned membership.  The
    // filesystem file is intentionally neither truncated nor removed.
    joinedServiceReport_.reset();
    joinedFederationName_.reset();
    joinedFederateId_.reset();

    auto scheduled = embeddedFederationRegistry().reevaluateTimeAdvanceGrants(
        federationName);
    if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
      newlyEligible = std::move(scheduled.dispatches);
    }
    auto timedSave = embeddedFederationRegistry()
        .reevaluateTimedFederationSave(federationName);
    if (timedSave.status == umbra::detail::FederationSaveControlStatus::applied) {
      for (auto& notification : timedSave.notifications) {
        saveNotifications.push_back(std::move(notification));
      }
    }
  }
  // Membership removal ends the receiving federate's update-rate lifetime.
  // Keep this scoped to the departed member so unrelated subscribers retain
  // their independent delivery windows.
  eraseAmbassadorUpdateRateHistoryForFederate(federationName, reportedFederateId);
  // Resignation ends this ambassador's joined-federate lifetime.  Any
  // restore/save or application callbacks queued before the transition are
  // stale for the departing member and must not be delivered after it has
  // resigned.  Notifications prepared for surviving members use their own
  // callback routes and are submitted below after this local queue is reset.
  callbacks_->reset();
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(saveNotifications));
  submitAmbassadorFederationRestoreNotifications(
      federationName,
      std::move(restoreNotifications));
  submitAmbassadorFederationSynchronizedNotifications(std::move(synchronizationNotifications));
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  if (resigned.finalServiceReportInteractionReservation) {
    auto reservation = std::move(*resigned.finalServiceReportInteractionReservation);
    if (reservation.serialNumber >
            static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        reservation.routing.reportParameterHandles.size() != 8U) {
      throw RTIinternalError(
          L"Umbra could not reserve the final ResignFederationExecution service-report interaction.");
    }
    std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
        {umbra::detail::MomArgumentType::resign_action,
         L"HLAresignAction",
         umbra::detail::formatMomResignAction(resignAction)}};
    umbra::detail::MomServiceArgument returnedArgument{
        umbra::detail::MomArgumentType::null_value,
        L"",
        umbra::detail::formatMomNull()};
    auto const encoded = umbra::detail::encodeMomServiceInvocation(
        L"ResignFederationExecution",
        umbra::detail::MomServiceType::federation_management,
        true,
        suppliedArguments,
        returnedArgument,
        L"",
        static_cast<std::int32_t>(reservation.serialNumber));
    std::vector<AmbassadorInteractionParameterValue> reportParameters;
    reportParameters.reserve(reservation.routing.reportParameterHandles.size());
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[0], encoded.service);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[1], encoded.serviceType);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[2], encoded.successIndicator);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[3], encoded.suppliedArguments);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[4], encoded.returnedArgument);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[5], encoded.exception);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[6], encoded.serialNumber);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[7],
        makeFederateHandle(reportedFederateId).encode());
    queueAmbassadorMomServiceReportInteraction(
        federationName,
        reportedFederateId,
        static_cast<std::uint16_t>(umbra::detail::MomServiceType::federation_management),
        std::move(reservation),
        std::move(reportParameters),
        ambassadorTransportationHandleFromEmbeddedName(
            umbra::detail::hla::utf8::mom::reliable,
            L"The embedded federation could not reconstruct HLAreportServiceInvocation transportation."),
        true);
  }
  VariableLengthData emptyResignTag;
  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
      std::move(resignOwnershipAcquisitionWorkItems),
      federationName);
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(resignOwnershipAssumptions),
      federationName,
      emptyResignTag);
  queueAmbassadorObjectInstanceRemovals(
      std::move(resignObjectRemovals),
      federationName,
      emptyResignTag);
  queueAmbassadorFederationMomConditionalAttributeUpdate(
      federationName,
      {umbra::detail::hla::utf8::mom::federates_in_federation});
  if (finalServiceReportAppendFailed) {
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Resign Federation Execution", exception);
    throw;
  }
}

void UmbraRtiAmbassador::registerFederationSynchronizationPoint(
    std::wstring const& synchronizationPointLabel,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("registerFederationSynchronizationPoint");
  registerFederationSynchronizationPointImpl(
      synchronizationPointLabel,
      userSuppliedTag,
      FederateHandleSet{},
      false);
}

void UmbraRtiAmbassador::registerFederationSynchronizationPoint(
    std::wstring const& synchronizationPointLabel,
    VariableLengthData const& userSuppliedTag,
    FederateHandleSet const& synchronizationSet) {
  auto instrumentationScope = beginRtiCall("registerFederationSynchronizationPoint");
  registerFederationSynchronizationPointImpl(
      synchronizationPointLabel,
      userSuppliedTag,
      synchronizationSet,
      true);
}

void UmbraRtiAmbassador::registerFederationSynchronizationPointImpl(
    std::wstring const& synchronizationPointLabel,
    VariableLengthData const& userSuppliedTag,
    FederateHandleSet const& synchronizationSet,
    bool synchronizationSetWasSupplied) {
  try {
  std::vector<unsigned char> copiedTag;
  if (userSuppliedTag.size() != 0) {
    auto const* data = static_cast<unsigned char const*>(userSuppliedTag.data());
    if (data == nullptr) {
      throw RTIinternalError(
          L"The synchronization-point tag reported a nonzero size with no data pointer.");
    }
    copiedTag.assign(data, data + userSuppliedTag.size());
  }
  auto const reportUserSuppliedTag =
      umbra::detail::formatMomUserSuppliedTag(userSuppliedTag);

  std::set<std::uint64_t> requestedFederateIds;
  for (auto const& federate : synchronizationSet) {
    auto const federateId = federateHandleValue(federate);
    if (!federateId) {
      throw InvalidFederateHandle(
          L"Register Federation Synchronization Point requires valid FederateHandle values.");
    }
    requestedFederateIds.insert(*federateId);
  }

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
            L"Register Federation Synchronization Point requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->registerFederationSynchronizationPoint(
          federationName,
          federateId,
          synchronizationPointLabel,
          std::move(copiedTag),
          std::vector<std::uint64_t>(
              requestedFederateIds.begin(), requestedFederateIds.end()),
          synchronizationSetWasSupplied);
      switch (result.status) {
        case umbra::detail::ProcessFederationSynchronizationPointRegistrationStatus::
            applied:
          break;
        case umbra::detail::ProcessFederationSynchronizationPointRegistrationStatus::
            federation_does_not_exist:
        case umbra::detail::ProcessFederationSynchronizationPointRegistrationStatus::
            federate_not_member:
          throw FederateNotExecutionMember(
              L"The process federation no longer records this RTI ambassador as a member.");
        case umbra::detail::ProcessFederationSynchronizationPointRegistrationStatus::
            callback_route_missing:
          throw RTIinternalError(
              L"The process federation has no callback route for synchronization-point registration.");
      }
      if (result.succeeded) {
        processClient->dispatchSynchronizationPointRegistrationSucceeded(
            synchronizationPointLabel);
      } else {
        processClient->dispatchSynchronizationPointRegistrationFailed(
            synchronizationPointLabel,
            result.failureReason);
      }
      if (joinedServiceReport_) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"RegisterFederationSynchronizationPoint",
          umbra::detail::MomServiceType::federation_management,
          {{umbra::detail::MomArgumentType::string,
            L"Synchronization point label",
            umbra::detail::formatMomString(synchronizationPointLabel)},
           {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
            L"User-supplied tag",
            reportUserSuppliedTag},
           {synchronizationSetWasSupplied
                ? umbra::detail::MomArgumentType::federate_handle_set
                : umbra::detail::MomArgumentType::null_value,
            L"Optional set of joined federate designators",
            synchronizationSetWasSupplied
                ? umbra::detail::formatMomFederateHandleSet(synchronizationSet)
                : umbra::detail::formatMomNull()}},
          true);
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"ConfirmSynchronizationPointRegistration",
          umbra::detail::MomServiceType::federation_management,
          {{umbra::detail::MomArgumentType::string,
            L"Synchronization point label",
            umbra::detail::formatMomString(synchronizationPointLabel)},
           {umbra::detail::MomArgumentType::boolean,
            L"Registration-success indicator",
            umbra::detail::formatMomBoolean(result.succeeded)},
           {result.succeeded
                ? umbra::detail::MomArgumentType::null_value
                : umbra::detail::MomArgumentType::synchronization_point_failure_reason,
            L"Optional failure reason",
            result.succeeded
                ? umbra::detail::formatMomNull()
                : umbra::detail::formatMomSynchronizationPointFailureReason(
                      result.failureReason)}},
          true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif

  umbra::detail::SynchronizationPointRegistrationPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Register Federation Synchronization Point");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Register Federation Synchronization Point requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.registerSynchronizationPoint(
        *joinedFederationName_,
        *joinedFederateId_,
        synchronizationPointLabel,
        std::move(copiedTag),
        requestedFederateIds,
        synchronizationSetWasSupplied);
    switch (plan.status) {
      case umbra::detail::SynchronizationPointRegistrationStatus::applied:
        break;
      case umbra::detail::SynchronizationPointRegistrationStatus::federation_does_not_exist:
      case umbra::detail::SynchronizationPointRegistrationStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::SynchronizationPointRegistrationStatus::callback_route_missing:
        throw RTIinternalError(
            L"The embedded federation has no callback route for synchronization-point registration.");
    }
  }
  // Emit the accepted service records only after releasing both native locks;
  // the public MOM route may synchronously invoke a federate callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RegisterFederationSynchronizationPoint",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::string,
        L"Synchronization point label",
        umbra::detail::formatMomString(synchronizationPointLabel)},
       {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
        L"User-supplied tag",
        reportUserSuppliedTag},
       {synchronizationSetWasSupplied
            ? umbra::detail::MomArgumentType::federate_handle_set
            : umbra::detail::MomArgumentType::null_value,
        L"Optional set of joined federate designators",
        synchronizationSetWasSupplied
            ? umbra::detail::formatMomFederateHandleSet(synchronizationSet)
            : umbra::detail::formatMomNull()}},
      true);
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ConfirmSynchronizationPointRegistration",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::string,
        L"Synchronization point label",
        umbra::detail::formatMomString(synchronizationPointLabel)},
       {umbra::detail::MomArgumentType::boolean,
        L"Registration-success indicator",
        umbra::detail::formatMomBoolean(plan.succeeded)},
       {plan.succeeded
            ? umbra::detail::MomArgumentType::null_value
            : umbra::detail::MomArgumentType::synchronization_point_failure_reason,
        L"Optional failure reason",
        plan.succeeded
            ? umbra::detail::formatMomNull()
            : umbra::detail::formatMomSynchronizationPointFailureReason(
                  plan.failureReason)}},
      true);
  submitAmbassadorSynchronizationPointRegistration(std::move(plan));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Register Federation Synchronization Point", exception);
    throw;
  }
}

void UmbraRtiAmbassador::synchronizationPointAchieved(
    std::wstring const& synchronizationPointLabel,
    bool successfully) {
  auto instrumentationScope = beginRtiCall("synchronizationPointAchieved");
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
            L"Synchronization Point Achieved requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->synchronizationPointAchieved(
          federationName,
          federateId,
          synchronizationPointLabel,
          successfully);
      switch (result.status) {
        case umbra::detail::ProcessFederationSynchronizationPointAchievedStatus::
            applied:
          break;
        case umbra::detail::ProcessFederationSynchronizationPointAchievedStatus::
            federation_does_not_exist:
        case umbra::detail::ProcessFederationSynchronizationPointAchievedStatus::
            federate_not_member:
          throw FederateNotExecutionMember(
              L"The process federation no longer records this RTI ambassador as a member.");
        case umbra::detail::ProcessFederationSynchronizationPointAchievedStatus::
            synchronization_point_label_not_announced:
          throw SynchronizationPointLabelNotAnnounced(
              L"The supplied synchronization-point label has not been announced to this federate.");
      }
      if (joinedServiceReport_) {
        appendSuccessfulVoidServiceReportToFileIfSelected(
            L"SynchronizationPointAchieved",
            umbra::detail::MomServiceType::federation_management,
            {{umbra::detail::MomArgumentType::string,
              L"Synchronization point label",
              umbra::detail::formatMomString(synchronizationPointLabel)},
             {umbra::detail::MomArgumentType::boolean,
              L"Optional synchronization-success indicator",
              umbra::detail::formatMomBoolean(successfully)}},
            true);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  umbra::detail::SynchronizationPointAchievedPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Synchronization Point Achieved");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Synchronization Point Achieved requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    plan = registry.achieveSynchronizationPoint(
        *joinedFederationName_,
        *joinedFederateId_,
        synchronizationPointLabel,
        successfully);
    switch (plan.status) {
      case umbra::detail::SynchronizationPointAchievedStatus::applied:
        break;
      case umbra::detail::SynchronizationPointAchievedStatus::federation_does_not_exist:
      case umbra::detail::SynchronizationPointAchievedStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::SynchronizationPointAchievedStatus::
          synchronization_point_label_not_announced:
        throw SynchronizationPointLabelNotAnnounced(
            L"The supplied synchronization-point label has not been announced to this federate.");
    }
  }
  // Section 4.17 defines the accepted achievement as the service boundary;
  // emit its public MOM interaction after releasing native locks because an
  // immediate observer may synchronously enter the Java/Python callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SynchronizationPointAchieved",
      umbra::detail::MomServiceType::federation_management,
      {{umbra::detail::MomArgumentType::string,
        L"Synchronization point label",
        umbra::detail::formatMomString(synchronizationPointLabel)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional synchronization-success indicator",
        umbra::detail::formatMomBoolean(successfully)}},
      true);
  submitAmbassadorFederationSynchronizedNotifications(
      std::move(plan.synchronizationNotifications));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Synchronization Point Achieved", exception);
    throw;
  }
}

std::unique_ptr<LogicalTimeFactory> UmbraRtiAmbassador::getTimeFactory() const {
  auto instrumentationScope = beginRtiCall("getTimeFactory");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  {
    std::optional<std::wstring> processImplementationName;
    bool processEndpoint = false;
    {
      std::scoped_lock lock(mutex_);
      processEndpoint = processEndpointActive_;
      if (processEndpoint) {
        requireConnectedForFederationManagement(lifecycle_);
        if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
            !joinedFederationName_ || !joinedFederateId_) {
          throw FederateNotExecutionMember(
              L"A logical-time factory is available only to a joined federate.");
        }
        processImplementationName = processLogicalTimeImplementationName_;
      }
    }
    if (processEndpoint) {
      if (!processImplementationName || processImplementationName->empty()) {
        throw RTIinternalError(
            L"The process service did not return the selected logical-time implementation.");
      }
      auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
          *processImplementationName);
      if (!factory || factory->getName() != *processImplementationName) {
        throw CouldNotCreateLogicalTimeFactory(
            L"Umbra could not recreate the process federation's selected logical-time factory.");
      }
      return factory;
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"A logical-time factory is available only to a joined federate.");
  }

  auto definition = embeddedFederationRegistry().definitionFor(*joinedFederationName_);
  if (!definition) {
    // The registry cannot normally lose a federation with an active member;
    // preserve the public membership error if an external backend later makes
    // that state observable.
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador's federation execution.");
  }
  return ambassadorMakeDefinitionTimeFactory(*definition);
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
