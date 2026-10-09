#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

bool EmbeddedFederationRegistry::startFederationSave(
    Federation& federation,
    std::wstring label,
    FederationSaveControlResult& result,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  // Validate every callback route before changing the shared save state.  A
  // timed request may become ready at a callback boundary; this keeps that
  // transition atomic if a member disconnected in the meantime.
  for (auto const& [federateId, member] : federation.members) {
    static_cast<void>(member);
    auto const route = federation.interactionCallbackRoutes.find(federateId);
    if (route == federation.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return false;
    }
  }

  Federation::SaveOperation operation;
  operation.label = std::move(label);
  operation.scheduledSaveTime = std::move(timestamp);
  for (auto const& [federateId, member] : federation.members) {
    static_cast<void>(member);
    operation.statuses.emplace(federateId, rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);
  }
  federation.saveOperation = std::move(operation);
  // Once Initiate Federate Save is emitted there is no longer an outstanding
  // requested (but not yet initiated) save of either form.
  federation.pendingImmediateSave.reset();
  federation.pendingTimedSave.reset();
  federation.nextSaveName.clear();
  federation.nextSaveTime.reset();

  result.notifications.reserve(federation.saveOperation->statuses.size());
  for (auto const& [federateId, status] : federation.saveOperation->statuses) {
    static_cast<void>(status);
    auto const route = federation.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation.serviceReportRoutes.find(federateId);
    auto const publicReportRoute = federation.publicServiceReportRoutes.find(federateId);
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::initiate;
    notification.receivingFederateId = federateId;
    notification.label = federation.saveOperation->label;
    notification.timestamp = federation.saveOperation->scheduledSaveTime;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    notification.publicServiceReportRoute =
        publicReportRoute == federation.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  result.status = FederationSaveControlStatus::applied;
  return true;
}

FederationSaveControlResult EmbeddedFederationRegistry::requestFederationSave(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring label) {
  auto instrumentationScope = beginInstrumentation("requestFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }
  auto const hasTimeConstrainedMember = std::any_of(
      execution->federates.begin(),
      execution->federates.end(),
      [](FederationTimeFederateSnapshot const& federate) {
        return federate.time.timeConstrained;
      });
  if (!hasTimeConstrainedMember) {
    // An immediate request supersedes any not-yet-initiated request only after
    // all callback routes have been validated by startFederationSave().
    static_cast<void>(startFederationSave(federation->second, std::move(label), result));
    return result;
  }

  // A time-constrained member may receive Initiate Federate Save only at its
  // Time Advancing boundary. Validate every route before replacing a pending
  // timed request, then retain this untimed request until the grant dispatcher
  // can invoke the constrained callbacks directly before their grants.
  for (auto const& [federateId, member] : federation->second.members) {
    static_cast<void>(member);
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return result;
    }
  }
  federation->second.pendingImmediateSave = Federation::PendingImmediateSave{
      std::move(label)};
  federation->second.pendingTimedSave.reset();
  federation->second.nextSaveName =
      federation->second.pendingImmediateSave->label;
  federation->second.nextSaveTime.reset();
  return result;
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitImmediateFederationSaveAtTimeAdvanceBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  FederationSaveAdmission result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  // The ordinary pre-grant helper runs before timestamped TSO delivery in the
  // shared dispatcher. Once a timestamped operation has started for another
  // constrained member, it must not consume this member's direct initiation:
  // its own timed helper will run only after the recipient's TSO payloads at
  // or below the scheduled save time have completed.
  if (federation->second.saveOperation.has_value() &&
      federation->second.saveOperation->scheduledSaveTime) {
    return result;
  }

  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }
  auto const current = std::find_if(
      execution->federates.begin(),
      execution->federates.end(),
      [federateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (current == execution->federates.end()) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }

  if (!federation->second.saveOperation.has_value()) {
    if (!federation->second.pendingImmediateSave.has_value()) {
      return result;
    }

    // The source requires a constrained member to receive the callback in
    // Time Advancing. Do not create the operation until every constrained
    // member is simultaneously at such a callback-capable boundary. This
    // lets the later per-recipient dispatch invoke each constrained callback
    // before it changes that federate to Time Granted.
    bool everyConstrainedMemberIsTimeAdvancing = true;
    for (auto const& candidate : execution->federates) {
      if (candidate.time.timeConstrained && !candidate.time.timeAdvancePending) {
        everyConstrainedMemberIsTimeAdvancing = false;
        break;
      }
    }
    if (!everyConstrainedMemberIsTimeAdvancing) {
      return result;
    }
    if (!current->time.timeConstrained || !current->time.timeAdvancePending) {
      return result;
    }

    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      auto const route = federation->second.interactionCallbackRoutes.find(memberId);
      if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
        result.status = FederationSaveControlStatus::callback_route_missing;
        return result;
      }
    }

    Federation::SaveOperation operation;
    operation.label = federation->second.pendingImmediateSave->label;
    operation.directTimeConstrainedInitiation = true;
    for (auto const& candidate : execution->federates) {
      operation.statuses.emplace(candidate.membership.id, rti1516_2025::NO_SAVE_IN_PROGRESS);
      if (candidate.time.timeConstrained) {
        operation.pendingTimeConstrainedInitiations.insert(candidate.membership.id);
      }
    }
    federation->second.saveOperation = std::move(operation);
    federation->second.pendingImmediateSave.reset();
    federation->second.nextSaveName.clear();
    federation->second.nextSaveTime.reset();
  }

  auto& operation = *federation->second.saveOperation;
  if (!operation.directTimeConstrainedInitiation ||
      !operation.pendingTimeConstrainedInitiations.contains(federateId)) {
    return result;
  }
  if (!current->time.timeConstrained || !current->time.timeAdvancePending) {
    return result;
  }

  auto currentStatus = operation.statuses.find(federateId);
  if (currentStatus == operation.statuses.end() ||
      currentStatus->second != rti1516_2025::NO_SAVE_IN_PROGRESS) {
    return result;
  }
  currentStatus->second = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
  operation.pendingTimeConstrainedInitiations.erase(federateId);
  result.currentFederateLabel = operation.label;

  if (!operation.pendingTimeConstrainedInitiations.empty()) {
    return result;
  }

  // The standard permits the non-time-constrained members to be instructed
  // only after every constrained member has become eligible. At this point
  // every constrained member has reached a direct Time Advancing callback
  // boundary, so queue the remaining ordinary callbacks after releasing the
  // federation lock.
  operation.directTimeConstrainedInitiation = false;
  for (auto& [memberId, status] : operation.statuses) {
    if (status != rti1516_2025::NO_SAVE_IN_PROGRESS) {
      continue;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(memberId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return result;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(memberId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(memberId);
    status = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::initiate;
    notification.receivingFederateId = memberId;
    notification.label = operation.label;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    notification.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::requestFederationSave(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring label,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  auto instrumentationScope = beginInstrumentation("requestFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!timestamp || timestamp->isInitial() || timestamp->isFinal() ||
      timestamp->implementationName() != federation->second.definition.logicalTimeImplementationName) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }

  // The public adapter performs the caller-side time-regulation/GALT and
  // lower-bound checks.  Repeat the representation checks here because this
  // registry is also exercised directly by the embedded profile tests.
  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }
  for (auto const& federate : execution->federates) {
    if (federate.time.implementationName !=
            federation->second.definition.logicalTimeImplementationName ||
        !federate.time.currentTime) {
      if (federate.time.timeConstrained) {
        result.status = FederationSaveControlStatus::invalid_timed_save;
        return result;
      }
      continue;
    }
    try {
      if (federate.time.timeConstrained && *timestamp <= *federate.time.currentTime) {
        result.status = FederationSaveControlStatus::invalid_timed_save;
        return result;
      }
    } catch (rti1516_2025::Exception const&) {
      result.status = FederationSaveControlStatus::invalid_timed_save;
      return result;
    }
  }

  // IEEE 1516.1 permits one outstanding requested save; a later request
  // replaces it, including its label and timestamp.  Re-evaluate immediately
  // so a federation with no constrained members starts without waiting for a
  // future time callback.
  federation->second.pendingImmediateSave.reset();
  federation->second.pendingTimedSave = Federation::PendingTimedSave{
      std::move(label), std::move(timestamp)};
  federation->second.nextSaveName =
      federation->second.pendingTimedSave->label;
  federation->second.nextSaveTime =
      federation->second.pendingTimedSave->timestamp;

  auto pending = federation->second.pendingTimedSave;
  bool ready = true;
  for (auto const& federate : execution->federates) {
    if (!federate.time.timeConstrained) {
      continue;
    }
    if (!federate.time.currentTime) {
      ready = false;
      break;
    }
    try {
      if (*federate.time.currentTime < *pending->timestamp) {
        ready = false;
        break;
      }
      auto const blocks = [&](std::vector<TsoQueuedMessage> const& messages) {
        return std::any_of(
            messages.begin(),
            messages.end(),
            [&pending](TsoQueuedMessage const& message) {
              return message.timestamp && *message.timestamp <= *pending->timestamp;
            });
      };
      if (blocks(federate.queuedTsoMessages) || blocks(federate.inTransitTsoMessages)) {
        ready = false;
        break;
      }
    } catch (rti1516_2025::Exception const&) {
      ready = false;
      break;
    }
  }
  if (ready) {
    auto timestamp = pending->timestamp;
    static_cast<void>(startFederationSave(
        federation->second,
        std::move(pending->label),
        result,
        std::move(timestamp)));
  }
  return result;
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitTimedFederationSaveAtTimeAdvanceBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  return admitTimedFederationSaveAtGrantBoundary(federationName, federateId, nullptr);
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitTimedFederationSaveAtFlushQueueGrantBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime) {
  return admitTimedFederationSaveAtGrantBoundary(
      federationName,
      federateId,
      std::move(flushQueueGrantedTime));
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitTimedFederationSaveAtGrantBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime) {
  FederationSaveAdmission result;

  auto const advanceCanReachTimestamp = [](
                                           FederateTimeSnapshot const& time,
                                           rti1516_2025::LogicalTime const& timestamp,
                                           std::shared_ptr<rti1516_2025::LogicalTime const> const&
                                               flushQueueGrantTime) {
    if (!time.timeAdvancePending || !time.requestedTime ||
        time.implementationName != timestamp.implementationName() ||
        time.requestedTime->implementationName() != timestamp.implementationName()) {
      return false;
    }
    try {
      switch (time.advanceMode) {
        case FederateTimeAdvanceMode::time_advance_request:
        case FederateTimeAdvanceMode::next_message_request:
          return *time.requestedTime >= timestamp;
        case FederateTimeAdvanceMode::time_advance_request_available:
        case FederateTimeAdvanceMode::next_message_request_available:
          return *time.requestedTime > timestamp;
        case FederateTimeAdvanceMode::flush_queue_request:
          return flushQueueGrantTime &&
              flushQueueGrantTime->implementationName() == timestamp.implementationName() &&
              *flushQueueGrantTime > timestamp;
        case FederateTimeAdvanceMode::none:
          return false;
      }
    } catch (rti1516_2025::Exception const&) {
      return false;
    }
    return false;
  };
  auto const hasUndeliveredTimestampThrough = [](
                                                   FederationTimeFederateSnapshot const& federate,
                                                   rti1516_2025::LogicalTime const& timestamp) {
    auto const blocks = [&timestamp](std::vector<TsoQueuedMessage> const& messages) {
      try {
        return std::any_of(
            messages.begin(),
            messages.end(),
            [&timestamp](TsoQueuedMessage const& message) {
              return message.timestamp && *message.timestamp <= timestamp;
            });
      } catch (rti1516_2025::Exception const&) {
        return true;
      }
    };
    return blocks(federate.queuedTsoMessages) || blocks(federate.inTransitTsoMessages);
  };

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }

  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }
  auto const current = std::find_if(
      execution->federates.begin(),
      execution->federates.end(),
      [federateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (current == execution->federates.end()) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (flushQueueGrantedTime &&
      (current->time.advanceMode != FederateTimeAdvanceMode::flush_queue_request ||
       flushQueueGrantedTime->implementationName() != current->time.implementationName)) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }

  auto const flushQueueGrantFor = [&execution,
                                   federateId,
                                   &flushQueueGrantedTime](
                                      FederationTimeFederateSnapshot const& candidate) {
    std::shared_ptr<rti1516_2025::LogicalTime const> grant;
    if (candidate.time.advanceMode != FederateTimeAdvanceMode::flush_queue_request) {
      return grant;
    }
    if (candidate.membership.id == federateId) {
      return flushQueueGrantedTime;
    }
    FederationFlushQueueGrantCalculator flushQueueGrantCalculator;
    auto calculation = flushQueueGrantCalculator.calculate(*execution, candidate.membership.id);
    if (!calculation.calculated()) {
      return grant;
    }
    return std::shared_ptr<rti1516_2025::LogicalTime const>{
        std::move(calculation.grantedTime)};
  };

  // A timestamped save is admitted by the qualifying constrained federate at
  // its own ordinary time-advance boundary. A non-constrained member may
  // request the save and receive its eventual notification, but it must not
  // open the operation while another member is still responsible for the
  // required pre-grant Initiate Federate Save callback.
  if (!current->time.timeConstrained) {
    return result;
  }

  if (!federation->second.saveOperation.has_value()) {
    if (!federation->second.pendingTimedSave.has_value()) {
      return result;
    }
    auto const& pending = *federation->second.pendingTimedSave;
    auto const currentFlushQueueGrant = flushQueueGrantFor(*current);
    if (!pending.timestamp ||
        !advanceCanReachTimestamp(
            current->time,
            *pending.timestamp,
            currentFlushQueueGrant) ||
        hasUndeliveredTimestampThrough(*current, *pending.timestamp)) {
      return result;
    }

    // Beginning a save after one constrained callback would make an idle
    // peer unable to request its own advance. Require every constrained
    // member to have a qualifying, already scheduled grant before this first
    // direct admission. FQR candidates use their shared callback-time grant
    // calculation, so a strict FQR boundary cannot open a save unless every
    // constrained member is already known to cross it. Its outstanding TSO
    // payloads are drained at that member's own callback boundary below.
    for (auto const& candidate : execution->federates) {
      if (!candidate.time.timeConstrained) {
        continue;
      }
      auto const candidateFlushQueueGrant = flushQueueGrantFor(candidate);
      if (!advanceCanReachTimestamp(
              candidate.time,
              *pending.timestamp,
              candidateFlushQueueGrant)) {
        return result;
      }
      if (candidate.membership.id == federateId) {
        continue;
      }
      auto const scheduled = federation->second.pendingTimeAdvanceGrants.find(
          candidate.membership.id);
      if (scheduled == federation->second.pendingTimeAdvanceGrants.end() ||
          !scheduled->second.dispatchQueued) {
        return result;
      }
    }

    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      auto const route = federation->second.interactionCallbackRoutes.find(memberId);
      if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
        result.status = FederationSaveControlStatus::callback_route_missing;
        return result;
      }
    }

    Federation::SaveOperation operation;
    operation.label = pending.label;
    operation.directTimeConstrainedInitiation = true;
    operation.scheduledSaveTime = pending.timestamp;
    for (auto const& candidate : execution->federates) {
      operation.statuses.emplace(candidate.membership.id, rti1516_2025::NO_SAVE_IN_PROGRESS);
      if (candidate.time.timeConstrained) {
        operation.pendingTimeConstrainedInitiations.insert(candidate.membership.id);
      }
    }
    federation->second.saveOperation = std::move(operation);
    federation->second.pendingTimedSave.reset();
    federation->second.nextSaveName.clear();
    federation->second.nextSaveTime.reset();
  }

  auto& operation = *federation->second.saveOperation;
  auto const currentFlushQueueGrant = flushQueueGrantFor(*current);
  if (!operation.directTimeConstrainedInitiation || !operation.scheduledSaveTime ||
      !operation.pendingTimeConstrainedInitiations.contains(federateId) ||
      !advanceCanReachTimestamp(
          current->time,
          *operation.scheduledSaveTime,
          currentFlushQueueGrant) ||
      hasUndeliveredTimestampThrough(*current, *operation.scheduledSaveTime)) {
    return result;
  }

  auto currentStatus = operation.statuses.find(federateId);
  if (currentStatus == operation.statuses.end() ||
      currentStatus->second != rti1516_2025::NO_SAVE_IN_PROGRESS) {
    return result;
  }
  currentStatus->second = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
  operation.pendingTimeConstrainedInitiations.erase(federateId);
  result.currentFederateLabel = operation.label;
  result.currentFederateTimestamp = operation.scheduledSaveTime;

  if (!operation.pendingTimeConstrainedInitiations.empty()) {
    return result;
  }

  operation.directTimeConstrainedInitiation = false;
  for (auto& [memberId, status] : operation.statuses) {
    if (status != rti1516_2025::NO_SAVE_IN_PROGRESS) {
      continue;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(memberId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return result;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(memberId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(memberId);
    status = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::initiate;
    notification.receivingFederateId = memberId;
    notification.label = operation.label;
    notification.timestamp = operation.scheduledSaveTime;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    notification.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::reevaluateTimedFederationSave(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("reevaluateTimedFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.pendingTimedSave.has_value()) {
    return result;
  }

  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }
  auto const hasTimeConstrainedMember = std::any_of(
      execution->federates.begin(),
      execution->federates.end(),
      [](FederationTimeFederateSnapshot const& federate) {
        return federate.time.timeConstrained;
      });
  if (hasTimeConstrainedMember) {
    // A constrained recipient must be admitted while still Time Advancing.
    // Do not revive the former post-grant callback path here: the qualified
    // ordinary-grant dispatcher owns that pre-grant admission instead.
    return result;
  }
  auto const& pending = *federation->second.pendingTimedSave;
  bool ready = true;
  try {
    for (auto const& federate : execution->federates) {
      if (!federate.time.timeConstrained) {
        continue;
      }
      if (!federate.time.currentTime ||
          *federate.time.currentTime < *pending.timestamp) {
        ready = false;
        break;
      }
      auto const blocks = [&](std::vector<TsoQueuedMessage> const& messages) {
        return std::any_of(
            messages.begin(),
            messages.end(),
            [&pending](TsoQueuedMessage const& message) {
              return message.timestamp && *message.timestamp <= *pending.timestamp;
            });
      };
      if (blocks(federate.queuedTsoMessages) || blocks(federate.inTransitTsoMessages)) {
        ready = false;
        break;
      }
    }
  } catch (rti1516_2025::Exception const&) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }
  if (ready) {
    auto label = pending.label;
    auto timestamp = pending.timestamp;
    static_cast<void>(startFederationSave(
        federation->second,
        std::move(label),
        result,
        std::move(timestamp)));
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::federateSaveBegun(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateSaveBegun");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_not_initiated;
    return result;
  }
  auto status = federation->second.saveOperation->statuses.find(federateId);
  if (status == federation->second.saveOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE) {
    result.status = FederationSaveControlStatus::save_not_initiated;
    return result;
  }
  status->second = rti1516_2025::FEDERATE_SAVING;
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::federateSaveComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateSaveComplete");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }
  auto status = federation->second.saveOperation->statuses.find(federateId);
  if (status == federation->second.saveOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_SAVING) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }
  status->second = rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE;

  bool allComplete = true;
  for (auto const& [participantId, participantStatus] :
       federation->second.saveOperation->statuses) {
    static_cast<void>(participantId);
    if (participantStatus != rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE) {
      allComplete = false;
      break;
    }
  }
  if (allComplete) {
    auto const snapshotLabel = federation->second.saveOperation->label;
    FederationSaveCommitDescriptor commitDescriptor;
    commitDescriptor.federationName = federationName;
    commitDescriptor.label = snapshotLabel;
    commitDescriptor.logicalTimeImplementationName =
        federation->second.definition.logicalTimeImplementationName;
    commitDescriptor.timed =
        static_cast<bool>(federation->second.saveOperation->scheduledSaveTime);
    commitDescriptor.memberFederateIds.reserve(federation->second.members.size());
    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      commitDescriptor.memberFederateIds.push_back(memberId);
    }
    bool snapshotStored = false;
    try {
      Federation snapshot = federation->second;
      snapshot.saveOperation.reset();
      snapshot.pendingImmediateSave.reset();
      snapshot.pendingTimedSave.reset();
      snapshot.restoreOperation.reset();
      snapshot.pendingTimeAdvanceGrants.clear();
      snapshot.timeAdvanceGrantDispatchFactories.clear();
      commitDescriptor.stateImage = FederationStateImageCodec::encode(
          stateImageFor(snapshot, federationName));
      // The durable commit envelope and its versioned state image must be
      // accepted before Federation Saved is exposed.  Callback routes remain
      // live-only and are deliberately not part of the persisted payload.
      saveCommitStore_->commit(commitDescriptor);
      saveSnapshots_[federationName][snapshotLabel] = std::move(snapshot);
      snapshotStored = true;
    } catch (...) {
      // The control operation remains well-formed even if this process-local
      // snapshot cannot be materialized. Report the standard save failure and
      // never claim that a restorable image exists.
      auto saved = saveSnapshots_.find(federationName);
      if (saved != saveSnapshots_.end()) {
        saved->second.erase(snapshotLabel);
      }
    }
    appendSaveCompletionNotifications(
        federation->second,
        snapshotStored,
        rti1516_2025::RTI_UNABLE_TO_SAVE,
        result.notifications);
    if (snapshotStored) {
      result.saveCompletedSuccessfully = true;
      federation->second.lastSaveName = snapshotLabel;
      federation->second.lastSaveTime =
          federation->second.saveOperation->scheduledSaveTime;
    }
    federation->second.saveOperation.reset();
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::federateSaveNotComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateSaveNotComplete");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }
  auto status = federation->second.saveOperation->statuses.find(federateId);
  if (status == federation->second.saveOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_SAVING) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }

  appendSaveCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE,
      result.notifications);
  federation->second.saveOperation.reset();
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::abortFederationSave(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("abortFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_not_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }

  appendSaveCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::SAVE_ABORTED,
      result.notifications);
  federation->second.saveOperation.reset();
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::queryFederationSaveStatus(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("queryFederationSaveStatus");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  auto const route = federation->second.interactionCallbackRoutes.find(federateId);
  if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
    result.status = FederationSaveControlStatus::callback_route_missing;
    return result;
  }

  FederationSaveNotification notification;
  notification.kind = FederationSaveNotificationKind::status;
  notification.receivingFederateId = federateId;
  notification.callbackRoute = route->second;
  auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
  auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
  notification.serviceReportRoute =
      reportRoute == federation->second.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : reportRoute->second;
  notification.publicServiceReportRoute =
      publicReportRoute == federation->second.publicServiceReportRoutes.end()
      ? FederatePublicServiceReportRoute{}
      : publicReportRoute->second;
  if (federation->second.saveOperation.has_value()) {
    for (auto const& [participantId, status] : federation->second.saveOperation->statuses) {
      notification.statuses.emplace_back(participantId, status);
    }
  } else {
    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      notification.statuses.emplace_back(memberId, rti1516_2025::NO_SAVE_IN_PROGRESS);
    }
  }
  result.notifications.push_back(std::move(notification));
  return result;
}

FederationServiceOperationStatus EmbeddedFederationRegistry::serviceOperationStatus(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  auto instrumentationScope = beginInstrumentation("serviceOperationStatus");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationServiceOperationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationServiceOperationStatus::federate_not_member;
  }
  if (federation->second.saveOperation.has_value()) {
    return FederationServiceOperationStatus::save_in_progress;
  }
  if (federation->second.restoreOperation.has_value()) {
    return FederationServiceOperationStatus::restore_in_progress;
  }
  return FederationServiceOperationStatus::available;
}

}  // namespace umbra::detail
