#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include "internal/fom/hla_names.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

constexpr std::wstring_view kProcessRtiVersion = L"Umbra 0.1.0";
constexpr std::wstring_view kProcessFederateHostFallback = L"process";
std::atomic_uint64_t nextProcessServiceReportJoinIdentifier{1U};

std::vector<PrevalidatedFomModule> processFomModulesSpecifiedAtJoin(
    FederationDefinition const& definition,
    std::vector<std::wstring> const& requestedDesignators) {
  std::set<std::filesystem::path> seenSources;
  std::vector<PrevalidatedFomModule> result;
  result.reserve(requestedDesignators.size());
  for (auto const& requested : requestedDesignators) {
    auto const found = std::find_if(
        definition.fomModules.begin(),
        definition.fomModules.end(),
        [&](PrevalidatedFomModule const& module) {
          return module.kind == FomModuleKind::fom &&
              module.designator == requested &&
              !module.sourcePath.empty();
        });
    if (found == definition.fomModules.end() ||
        !seenSources.insert(found->sourcePath).second) {
      continue;
    }
    result.push_back(*found);
  }
  return result;
}

std::wstring processFederateHost(ProcessTransportSession const& session) {
  auto const connection = session.connection();
  if (!connection) {
    return std::wstring{kProcessFederateHostFallback};
  }
  auto const converted = wideFromUtf8(connection->peerIdentity().endpointId);
  return converted.value_or(std::wstring{kProcessFederateHostFallback});
}

std::wstring formatProcessServiceReportInitialRecord(
    ProcessFederationJoinConnectionSnapshot const& connection,
    std::wstring const& federationName,
    FederationDefinition const& definition,
    FederateMembership const& membership,
    bool autoProvide,
    std::wstring const& federateHost,
    std::vector<PrevalidatedFomModule> const& fomModulesSpecifiedAtJoin) {
  MomServiceReportInitialRecord record;
  record.callbackModel = connection.callbackModel;
  record.configurationName = connection.configurationName;
  record.rtiAddress = connection.rtiAddress;
  record.additionalSettings = connection.additionalSettings;
  if (connection.credentials) {
    record.credentials = MomServiceReportCredentials{
        connection.credentials->first,
        connection.credentials->second};
  }
  record.federationName = federationName;
  record.rtiVersion = std::wstring{kProcessRtiVersion};
  record.timeImplementationName = definition.logicalTimeImplementationName;
  record.autoProvide = autoProvide;
  record.federateHandle =
      rti1516_2025::umbra_binding_detail::makeFederateHandle(membership.id).toString();
  record.federateName = membership.name;
  record.federateType = membership.type;
  record.federateHost = federateHost;
  for (auto const& module : definition.fomModules) {
    if (module.kind == FomModuleKind::mim) {
      record.mimDesignator = module.designator;
    } else {
      record.federationFomModuleDesignators.push_back(module.designator);
    }
  }
  for (auto const& module : fomModulesSpecifiedAtJoin) {
    record.federateFomModuleDesignators.push_back(module.designator);
  }
  return formatMomServiceReportInitialRecord(record);
}

[[nodiscard]] std::shared_ptr<FederateTimeState> makeProcessFederateTimeState(
    FederationDefinition const& definition) {
  if (definition.logicalTimeImplementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation definition requires a logical-time implementation.");
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      definition.logicalTimeImplementationName);
  if (!factory ||
      factory->getName() != definition.logicalTimeImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "The process federation could not create its logical-time factory.");
  }
  auto initial = factory->makeInitial();
  if (!initial ||
      initial->implementationName() != definition.logicalTimeImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "The process federation logical-time factory did not provide an initial value.");
  }
  return std::make_shared<FederateTimeState>(
      definition.logicalTimeImplementationName, std::move(initial));
}

}  // namespace

TransportServiceMessage ProcessFederationService::handleCreate(
    TransportServiceMessage const& request) {
  auto const createRequest = decodeProcessFederationCreateRequest(request.payload);
  std::optional<FederationDefinition> preparedDefinition;
  if (createRequest.hasFomInputs) {
    if (!options_.createFomPreparation) {
      // The process transport must not silently discard the official FOM/MIM
      // inputs. A configured endpoint without the standards-derived
      // coordinator rejects the explicit form deterministically.
      return rejected(request);
    }
    preparedDefinition = options_.createFomPreparation(
        createRequest.fomModules,
        createRequest.mimModule,
        createRequest.logicalTimeImplementationName);
    if (!preparedDefinition || preparedDefinition->fomModules.empty() ||
        preparedDefinition->logicalTimeImplementationName.empty()) {
      return rejected(request);
    }
  }
  auto result = registry_.create(
      createRequest.federationName,
      preparedDefinition ? std::move(*preparedDefinition)
                         : *federationDefinition_);
  if (result.status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleDestroy(
    TransportServiceMessage const& request) {
  auto const destroyRequest =
      decodeProcessFederationDestroyRequest(request.payload);
  auto const destroyed = registry_.destroy(destroyRequest.federationName);
  ProcessFederationDestroyStatus status =
      ProcessFederationDestroyStatus::invalid_request;
  switch (destroyed.status) {
    case FederationRegistryStatus::applied:
      status = ProcessFederationDestroyStatus::applied;
      break;
    case FederationRegistryStatus::federation_does_not_exist:
      status = ProcessFederationDestroyStatus::federation_does_not_exist;
      break;
    case FederationRegistryStatus::federates_currently_joined:
      status = ProcessFederationDestroyStatus::federates_currently_joined;
      break;
    case FederationRegistryStatus::invalid_request:
    case FederationRegistryStatus::federation_already_exists:
    case FederationRegistryStatus::federate_name_already_in_use:
    case FederationRegistryStatus::federate_not_member:
    case FederationRegistryStatus::invalid_resign_action:
    case FederationRegistryStatus::ownership_acquisition_pending:
    case FederationRegistryStatus::federate_owns_attributes:
      break;
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDestroyResult(
          ProcessFederationDestroyResult{status}));
}

TransportServiceMessage ProcessFederationService::handleJoin(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const joinRequest = decodeProcessFederationJoinRequest(request.payload);
  // Additional FOM preparation and the registry commit must observe one
  // definition.  Serialize this pair across process sessions so concurrent
  // joins cannot both compose from the same stale catalog and race a
  // replacement definition into the registry.
  std::scoped_lock joinLock(joinMutex_);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() || state->second.federateId != 0U) {
      return rejected(request);
    }
  }

  auto currentDefinition = registry_.definitionFor(joinRequest.federationName);
  if (!currentDefinition) {
    return rejected(request);
  }

  std::optional<FederationDefinition> replacementDefinition;
  if (!joinRequest.additionalFomModules.empty()) {
    if (!options_.additionalFomPreparation) {
      // The process transport does not invent FOM composition semantics. A
      // configured endpoint must supply the same standards-derived
      // coordinator used by the embedded profile; otherwise this request is
      // rejected deterministically instead of silently dropping modules.
      return rejected(request);
    }
    replacementDefinition = options_.additionalFomPreparation(
        *currentDefinition, joinRequest.additionalFomModules);
    if (!replacementDefinition || replacementDefinition->fomModules.empty()) {
      return rejected(request);
    }
  }

  auto const& effectiveDefinition = replacementDefinition
      ? *replacementDefinition
      : *currentDefinition;
  // Capture the selected implementation before an additional-FOM definition
  // is moved into the registry below. The Join response must describe the
  // committed definition, not the moved-from preparation object.
  auto logicalTimeImplementationName =
      effectiveDefinition.logicalTimeImplementationName;

  InteractionCallbackRoute callbackRoute;
  callbackRoute.submit = [](FederateCallbackInvocation) {};
  // The registry retains this route as the callback ownership boundary.  The
  // first process slice projects the callback payload into the receiver's
  // event queue below; a later public adapter will replace this bridge with
  // the receiver process's official FederateAmbassador dispatch.
  callbackRoute.receiveOrderSubmit = [](FederateCallbackInvocation) {};
  auto timeState = makeProcessFederateTimeState(effectiveDefinition);
  auto const processConnection = session.connection();
  auto timeAdvanceGrantDispatchFactory =
      [this,
       processConnection,
       federationName = joinRequest.federationName,
       timeState](std::uint64_t federateId,
                   std::uint64_t generation,
                   std::uint64_t dispatchIdentity)
      -> FederationTimeGrantDispatch {
    if (!processConnection || !timeState || federateId == 0U ||
        generation == 0U || dispatchIdentity == 0U) {
      return {};
    }
    return [this,
            processConnection,
            federationName,
            timeState,
            federateId,
            generation,
            dispatchIdentity] {
      auto const beginStatus = registry_.beginTimeAdvanceGrant(
          federationName,
          federateId,
          generation,
          dispatchIdentity);
      switch (beginStatus) {
        case FederationTimeGrantStatus::time_advance_not_pending:
        case FederationTimeGrantStatus::stale_generation:
        case FederationTimeGrantStatus::grant_not_ready:
        case FederationTimeGrantStatus::federate_not_member:
        case FederationTimeGrantStatus::federation_does_not_exist:
          // A resignation, restore, or competing re-evaluation may have
          // invalidated a queued dispatch. The registry deliberately fences
          // that work; do not manufacture a callback for stale state.
          return;
        case FederationTimeGrantStatus::inconsistent_temporal_state:
          throw std::runtime_error(
              "The process federation time-grant scheduler lost temporal state.");
        case FederationTimeGrantStatus::applied:
          break;
      }

      auto const pendingSnapshot = timeState->snapshot();
      if (!pendingSnapshot.requestedTime ||
          pendingSnapshot.requestedTime->implementationName() !=
              timeState->implementationName()) {
        throw std::runtime_error(
            "The process federation time-grant state has no usable delivery boundary.");
      }
      if (pendingSnapshot.advanceMode == FederateTimeAdvanceMode::flush_queue_request) {
        if (!pendingSnapshot.currentTime ||
            pendingSnapshot.currentTime->implementationName() !=
                timeState->implementationName()) {
          throw std::runtime_error(
              "The process federation Flush Queue Grant has no usable current-time boundary.");
        }

        auto execution = registry_.timeSnapshotFor(federationName);
        if (!execution) {
          throw std::runtime_error(
              "The process federation no longer records the Flush Queue requester.");
        }
        FederationFlushQueueGrantCalculator flushQueueGrantCalculator;
        auto flushQueueCalculation = flushQueueGrantCalculator.calculate(
            *execution, federateId);
        if (!flushQueueCalculation.calculated()) {
          throw std::runtime_error(
              "The process federation could not calculate the Flush Queue Grant times.");
        }

        auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
            pendingSnapshot.implementationName);
        if (!factory || factory->getName() != pendingSnapshot.implementationName) {
          throw std::runtime_error(
              "The process federation could not create the Flush Queue time factory.");
        }
        auto finalTime = factory->makeFinal();
        if (!finalTime || finalTime->implementationName() !=
                              pendingSnapshot.implementationName) {
          throw std::runtime_error(
              "The process federation could not create the Flush Queue boundary.");
        }

        auto encodedOptimistic = encodeProcessLogicalTime(
            flushQueueCalculation.optimisticTime);
        if (!encodedOptimistic) {
          throw std::runtime_error(
              "The process federation could not encode the Flush Queue optimistic time.");
        }

        // Keep the state in Time Advancing while all queued TSO payloads cross
        // the process callback boundary, then commit the two-value FQR state
        // immediately before publishing the matching event. Flush Queue
        // Request uses the caller-supplied frontier for delivery admission;
        // the calculated actual and optimistic values describe the resulting
        // grant and must not be used to suppress payloads admitted by FQR.
        dispatchTsoInteractionPayloads(
            federationName, federateId, *pendingSnapshot.requestedTime);
        auto flushResult = timeState->grantFlushQueue(
            generation,
            std::move(flushQueueCalculation.grantedTime),
            std::move(flushQueueCalculation.optimisticTime));
        if (flushResult.status != FederateTimeAdvanceStatus::applied) {
          throw std::runtime_error(
              "The process federation time state rejected the Flush Queue Grant.");
        }
        auto encodedGrant = encodeProcessLogicalTime(timeState->currentTime());
        if (!encodedGrant) {
          throw std::runtime_error(
              "The process federation could not encode the Flush Queue Grant time.");
        }

        ProcessFederationTimeAdvanceResult eventResult;
        eventResult.status = ProcessFederationTimeAdvanceStatus::applied;
        eventResult.grantedTime = std::move(encodedGrant);
        eventResult.optimisticTime = std::move(encodedOptimistic);
        ProcessTransportSession eventSession(processConnection);
        if (!eventSession.send(
                TransportServiceMessage{
                    TransportServiceMessageKind::event,
                    TransportServiceOperation::time_advance_grant,
                    TransportServiceStatus::ok,
                    0U,
                    encodeProcessFederationTimeAdvanceResult(eventResult)})) {
          throw std::runtime_error(
              "The process federation could not deliver the Flush Queue Grant event.");
        }
        return;
      }
      // Timestamped interaction callbacks precede the matching grant.  Keep
      // the FederateTimeState in Time Advancing until the registry has moved
      // every eligible process payload across its callback boundary.
      dispatchTsoInteractionPayloads(
          federationName,
          federateId,
          *pendingSnapshot.requestedTime);

      // A timestamped federation save has the same pre-grant boundary as the
      // embedded adapter: the constrained recipient must observe Initiate
      // Federate Save while it is still Time Advancing, before this grant is
      // published.  The registry owns eligibility and cross-federate ordering;
      // this process seam only projects the resulting callback event.
      auto const saveAdmission =
          registry_.admitTimedFederationSaveAtTimeAdvanceBoundary(
              federationName,
              federateId);
      if (saveAdmission.status != FederationSaveControlStatus::applied) {
        throw std::runtime_error(
            "The process federation could not admit a timestamped save at the time-advance boundary.");
      }
      if (saveAdmission.currentFederateLabel) {
        if (!saveAdmission.currentFederateTimestamp) {
          throw std::runtime_error(
              "The process federation admitted a timestamped save without its requested time.");
        }
        ProcessFederationSaveEvent saveEvent;
        saveEvent.kind = FederationSaveNotificationKind::initiate;
        saveEvent.receivingFederateId = federateId;
        saveEvent.label = *saveAdmission.currentFederateLabel;
        saveEvent.successful = true;
        saveEvent.timestamp = encodeProcessLogicalTime(
            saveAdmission.currentFederateTimestamp);
        if (!saveEvent.timestamp) {
          throw std::runtime_error(
              "The process federation could not encode the timestamped save boundary.");
        }
        ProcessFederationReceiveInteractionResult saveResult;
        saveResult.saveEvent = std::move(saveEvent);
        ProcessTransportSession eventSession(processConnection);
        if (!eventSession.send(TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_interaction,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveInteractionResult(saveResult)})) {
          throw std::runtime_error(
              "The process federation could not deliver the timestamped save initiation event.");
        }
      }
      if (!saveAdmission.notifications.empty() &&
          !enqueueFederationSaveNotifications(
              federationName,
              std::move(saveAdmission.notifications))) {
        throw std::runtime_error(
            "The process federation could not enqueue the remaining timestamped save notifications.");
      }

      auto const grantedTime = timeState->grant(generation);
      auto encodedGrant = encodeProcessLogicalTime(grantedTime);
      if (!encodedGrant) {
        throw std::runtime_error(
            "The process federation time-grant state rejected its scheduled generation.");
      }
      ProcessFederationTimeAdvanceResult eventResult;
      eventResult.status = ProcessFederationTimeAdvanceStatus::applied;
      eventResult.grantedTime = std::move(encodedGrant);
      ProcessTransportSession eventSession(processConnection);
      if (!eventSession.send(
              TransportServiceMessage{
                  TransportServiceMessageKind::event,
                  TransportServiceOperation::time_advance_grant,
                  TransportServiceStatus::ok,
                  0U,
                  encodeProcessFederationTimeAdvanceResult(eventResult)})) {
        throw std::runtime_error(
            "The process federation could not deliver a time-advance grant event.");
      }
    };
  };
  FederationJoinResult result;
  if (replacementDefinition) {
    result = registry_.joinWithDefinitionAndTimeState(
        joinRequest.federationName,
        std::move(*replacementDefinition),
        timeState,
        joinRequest.federateType,
        joinRequest.requestedFederateName,
        std::move(callbackRoute),
        std::move(timeAdvanceGrantDispatchFactory));
  } else {
    result = registry_.joinWithTimeState(
        joinRequest.federationName,
        timeState,
        joinRequest.federateType,
        joinRequest.requestedFederateName,
        std::move(callbackRoute),
        std::move(timeAdvanceGrantDispatchFactory));
  }
  if (result.status != FederationRegistryStatus::applied || !result.membership) {
    return rejected(request);
  }

  // A configured process service owns the same joined-federate report-file
  // transaction as the embedded profile.  Allocate the immutable filesystem
  // identity and establish the RTI-owned MOM ledger before exposing the
  // successful Join response; a failure rolls the membership back and never
  // falls back to an in-memory or synthetic location.
  std::unique_ptr<ServiceReportWriter> serviceReportWriter;
  std::filesystem::path serviceReportLocation;
  std::wstring reportServiceFile;
  if (serviceReportStore_) {
    auto rollbackJoinedMembership = [&]() {
      auto const rollback = registry_.resign(
          joinRequest.federationName,
          result.membership->id,
          rti1516_2025::NO_ACTION);
      if (timeState) {
        timeState->deactivate();
      }
      return rollback.status == FederationRegistryStatus::applied;
    };

    try {
      auto const joinedDefinition =
          registry_.definitionFor(joinRequest.federationName);
      auto const autoProvide = registry_.autoProvideSwitchFor(
          joinRequest.federationName,
          result.membership->id);
      if (!joinedDefinition || !autoProvide) {
        throw std::runtime_error(
            "The process federation could not resolve the committed Join definition.");
      }

      auto const fomModulesSpecifiedAtJoin =
          processFomModulesSpecifiedAtJoin(
              *joinedDefinition,
              joinRequest.additionalFomModules);
      if (fomModulesSpecifiedAtJoin.size() !=
          std::set<std::wstring>{
              joinRequest.additionalFomModules.begin(),
              joinRequest.additionalFomModules.end()}.size()) {
        throw std::runtime_error(
            "The process federation could not retain the validated Join FOM modules.");
      }

      bool const legacyFomCompatibility =
          joinedDefinition->standardEdition == FomStandardEdition::ieee1516_2010;
      std::wstring mimDesignator;
      for (auto const& module : joinedDefinition->fomModules) {
        if (module.kind == FomModuleKind::mim) {
          mimDesignator = module.designator;
        }
      }
      if (!legacyFomCompatibility) {
        auto const federationMomStatus = registry_.establishFederationMomObject(
            joinRequest.federationName,
            std::wstring{kProcessRtiVersion},
            mimDesignator);
        if (federationMomStatus != JoinedFederateMomObjectStatus::applied &&
            federationMomStatus != JoinedFederateMomObjectStatus::already_established) {
          throw std::runtime_error(
              "The process federation could not establish its RTI-owned federation MOM object.");
        }
      }

      auto const joinIdentifier = nextProcessServiceReportJoinIdentifier.fetch_add(
          1U,
          std::memory_order_relaxed);
      auto writer = serviceReportStore_->createForJoinedFederate({
          joinRequest.federationName,
          result.membership->name,
          result.membership->id,
          joinIdentifier,
          formatProcessServiceReportInitialRecord(
              joinRequest.connection,
              joinRequest.federationName,
              *joinedDefinition,
              *result.membership,
              *autoProvide,
              processFederateHost(session),
              fomModulesSpecifiedAtJoin),
      });
      if (!writer || writer->location().empty()) {
        throw std::runtime_error(
            "The process federation service-report store did not return a filesystem location.");
      }
      serviceReportLocation = writer->location();
      reportServiceFile = serviceReportLocation.wstring();

      if (!legacyFomCompatibility) {
        auto const momObjectStatus = registry_.establishJoinedFederateMomObject(
            joinRequest.federationName,
            result.membership->id,
            {
                processFederateHost(session),
                std::wstring{kProcessRtiVersion},
                fomModulesSpecifiedAtJoin,
                reportServiceFile,
            });
        if (momObjectStatus != JoinedFederateMomObjectStatus::applied) {
          throw std::runtime_error(
              "The process federation could not establish the joined federate MOM object.");
        }
      }
      serviceReportWriter = std::move(writer);
    } catch (...) {
      if (!rollbackJoinedMembership()) {
        return internalError(request);
      }
      return internalError(request);
    }
  }

  {
    std::scoped_lock lock(mutex_);
    auto& state = sessions_.at(&session);
    state.federationName = joinRequest.federationName;
    state.federateId = result.membership->id;
    state.timeState = std::move(timeState);
    state.serviceReportWriter = std::move(serviceReportWriter);
    state.serviceReportLocation = serviceReportLocation;
    sessionsByFederateId_[state.federateId] = &session;
  }
  // Join creates the RTI-owned HLAfederate MOM object for this new joined
  // lifetime. Existing class subscribers must discover that object through
  // the same process event path used by the embedded Join implementation;
  // they cannot rely on a new subscription to backfill the new lifetime.
  if (auto const momObject = registry_.joinedFederateMomObjectFor(
          joinRequest.federationName,
          result.membership->id)) {
    auto discoveries = registry_.planJoinedFederateMomObjectDiscoveriesForInstance(
        joinRequest.federationName,
        momObject->objectInstanceHandle);
    if (!enqueueObjectInstanceDiscoveries(
            joinRequest.federationName,
            std::move(discoveries))) {
      return internalError(request);
    }
  }
  auto pendingAnnouncements = registry_.announcePendingSynchronizationPoints(
      joinRequest.federationName,
      result.membership->id);
  if (pendingAnnouncements.status !=
          SynchronizationPointAnnouncementStatus::applied ||
      (!pendingAnnouncements.announcements.empty() &&
       !enqueueSynchronizationPointAnnouncements(
           joinRequest.federationName,
           std::move(pendingAnnouncements.announcements)))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationJoinResult(
          ProcessFederationJoinResult{
              result.membership->id,
              result.membership->name,
              std::move(logicalTimeImplementationName),
              std::move(reportServiceFile)}));
}

}  // namespace umbra::detail
