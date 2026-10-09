#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"

#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

namespace umbra::detail {
using process_federation_payload::parameterVector;
ProcessFederationService::ProcessFederationService(
    EmbeddedFederationRegistry& registry,
    FederationDefinition federationDefinition,
    ProcessFederationServiceOptions options)
    : registry_(registry),
      federationDefinition_(
          std::make_shared<FederationDefinition const>(std::move(federationDefinition))),
      options_(std::move(options)),
      serviceReportStore_(
          options_.serviceReportDirectory.empty()
              ? nullptr
              : std::make_unique<FilesystemServiceReportStore>(
                    options_.serviceReportDirectory)) {}

ProcessTransportServiceDispatcher::Handler ProcessFederationService::handlerFor(
    ProcessTransportSession& session) {
  {
    std::scoped_lock lock(mutex_);
    sessions_.try_emplace(&session);
  }
  return [this, &session](TransportServiceMessage const& request) {
    return handle(session, request);
  };
}

void ProcessFederationService::detach(ProcessTransportSession& session) noexcept {
  std::scoped_lock lock(mutex_);
  for (auto position = pendingPushedRetractionRecipients_.begin();
       position != pendingPushedRetractionRecipients_.end();) {
    auto& recipients = position->second;
    recipients.erase(
        std::remove(recipients.begin(), recipients.end(), &session),
        recipients.end());
    if (recipients.empty()) {
      position = pendingPushedRetractionRecipients_.erase(position);
    } else {
      ++position;
    }
  }
  auto const found = sessions_.find(&session);
  if (found == sessions_.end()) {
    return;
  }
  if (found->second.federateId != 0U) {
    sessionsByFederateId_.erase(found->second.federateId);
  }
  sessions_.erase(found);
}



TransportServiceMessage ProcessFederationService::handleAcknowledgeTsoDelivery(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const acknowledgement =
      decodeProcessFederationAcknowledgeTsoDeliveryRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acknowledgement.federationName ||
        state->second.federateId != acknowledgement.receivingFederateId ||
        !registry_.memberById(
            acknowledgement.federationName,
            acknowledgement.receivingFederateId)) {
      return rejected(request);
    }
  }

  auto const delivery = registry_.completeTsoDeliveryFor(
      acknowledgement.federationName,
      acknowledgement.receivingFederateId,
      acknowledgement.messageId);
  if (delivery.status != FederationTsoRegistryStatus::applied) {
    return rejected(request);
  }

  ProcessFederationTsoDeliveryAcknowledgementResult result;
  switch (delivery.delivery.status) {
    case FederationTsoDeliveryStatus::applied:
      result.status =
          ProcessFederationTsoDeliveryAcknowledgementStatus::applied;
      break;
    case FederationTsoDeliveryStatus::message_already_completed:
      result.status = ProcessFederationTsoDeliveryAcknowledgementStatus::
          already_completed;
      break;
    case FederationTsoDeliveryStatus::message_not_in_transit:
    case FederationTsoDeliveryStatus::no_messages:
      result.status =
          ProcessFederationTsoDeliveryAcknowledgementStatus::not_in_transit;
      break;
    case FederationTsoDeliveryStatus::invalid_recipient:
    case FederationTsoDeliveryStatus::invalid_boundary:
      return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationTsoDeliveryAcknowledgementResult(result));
}

TransportServiceMessage ProcessFederationService::handleReceiveAttributeUpdate(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const receiveRequest = decodeProcessFederationReceiveInteractionRequest(
      request.payload);
  ProcessFederationReceiveAttributeUpdateResult result;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != receiveRequest.federationName ||
        state->second.federateId != receiveRequest.receivingFederateId ||
        !registry_.memberById(
            receiveRequest.federationName, receiveRequest.receivingFederateId)) {
      return rejected(request);
    }
    while (!state->second.attributeUpdateEvents.empty()) {
      auto event = std::move(state->second.attributeUpdateEvents.front());
      state->second.attributeUpdateEvents.pop_front();
      if (event.retractionMessageId &&
          !registry_.beginTsoAttributeUpdateCallback(
              receiveRequest.federationName,
              receiveRequest.receivingFederateId,
              *event.retractionMessageId)) {
        // A legal Retract may have won before this pull-mode event crossed
        // the callback boundary. Consume the stale frame without exposing a
        // reflection callback.
        continue;
      }
      result.event = std::move(event);
      break;
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveAttributeUpdateResult(result));
}

TransportServiceMessage
ProcessFederationService::handleReceiveObjectInstanceDiscovery(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const receiveRequest = decodeProcessFederationReceiveInteractionRequest(
      request.payload);
  ProcessFederationReceiveObjectInstanceDiscoveryResult result;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != receiveRequest.federationName ||
        state->second.federateId != receiveRequest.receivingFederateId ||
        !registry_.memberById(
            receiveRequest.federationName,
            receiveRequest.receivingFederateId)) {
      return rejected(request);
    }
    if (!state->second.objectInstanceDiscoveryEvents.empty()) {
      result.event = std::move(
          state->second.objectInstanceDiscoveryEvents.front());
      state->second.objectInstanceDiscoveryEvents.pop_front();
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveObjectInstanceDiscoveryResult(result));
}

bool ProcessFederationService::appendSelectedServiceReportRecord(
    ProcessTransportSession& session,
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint16_t serviceGroup,
    FederateServiceReportRecordEncoder encodeRecord) {
  if (!encodeRecord) {
    return false;
  }

  std::scoped_lock lock(mutex_);
  auto const state = sessions_.find(&session);
  if (state == sessions_.end() ||
      !state->second.federationName.has_value() ||
      *state->second.federationName != federationName ||
      state->second.federateId != federateId) {
    return false;
  }

  auto const plan = registry_.planMomServiceReport(
      federationName, federateId, serviceGroup);
  switch (plan.disposition) {
    case MomServiceReportDisposition::suppressed:
    case MomServiceReportDisposition::interaction:
      return true;
    case MomServiceReportDisposition::inconsistent_catalog:
    case MomServiceReportDisposition::reported_federate_not_member:
    case MomServiceReportDisposition::invalid_service_group:
      return false;
    case MomServiceReportDisposition::report_to_file:
      break;
  }

  if (!state->second.serviceReportWriter) {
    return false;
  }
  auto const reserved = registry_.reserveMomServiceReport(
      federationName, federateId, serviceGroup);
  if (!reserved.acceptedForEmission ||
      reserved.routing.disposition != MomServiceReportDisposition::report_to_file) {
    return false;
  }
  try {
    state->second.serviceReportWriter->append(encodeRecord(reserved.serialNumber));
  } catch (std::exception const&) {
    return false;
  }
  return true;
}

TransportServiceMessage ProcessFederationService::rejected(
    TransportServiceMessage const& request) const {
  return responseFor(request, TransportServiceStatus::rejected);
}

TransportServiceMessage ProcessFederationService::invalid(
    TransportServiceMessage const& request) const {
  return responseFor(request, TransportServiceStatus::invalid_request);
}

TransportServiceMessage ProcessFederationService::internalError(
    TransportServiceMessage const& request) const {
  return responseFor(request, TransportServiceStatus::internal_error);
}

}  // namespace umbra::detail
