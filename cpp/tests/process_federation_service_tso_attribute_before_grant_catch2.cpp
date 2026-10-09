#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "process_federation_service_test_support.hpp"
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationCreateRequest;
using umbra::detail::ProcessFederationJoinRequest;
using umbra::detail::ProcessFederationJoinResult;
using umbra::detail::ProcessFederationResignRequest;
using umbra::detail::ProcessFederationLogicalTime;
using umbra::detail::ProcessFederationLogicalTimeInterval;
using umbra::detail::ProcessFederationEnableTimeRegulationRequest;
using umbra::detail::ProcessFederationTimeEnableStatus;
using umbra::detail::ProcessFederationDeleteObjectInstanceRequest;
using umbra::detail::ProcessFederationDeleteObjectInstanceResult;
using umbra::detail::ProcessFederationLocalDeleteObjectInstanceRequest;
using umbra::detail::ProcessFederationLocalDeleteObjectInstanceResult;
using umbra::detail::ProcessFederationReceiveInteractionRequest;
using umbra::detail::ProcessFederationReceiveInteractionResult;
using umbra::detail::ProcessFederationAcknowledgeTsoDeliveryRequest;
using umbra::detail::ProcessFederationTsoDeliveryAcknowledgementStatus;
using umbra::detail::ProcessFederationSendInteractionRequest;
using umbra::detail::ProcessFederationSendInteractionResult;
using umbra::detail::ProcessFederationReceiveAttributeUpdateResult;
using umbra::detail::ProcessFederationReceiveObjectInstanceDiscoveryResult;
using umbra::detail::ProcessFederationRequestAttributeValueUpdateRequest;
using umbra::detail::ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest;
using umbra::detail::ProcessFederationRequestAttributeValueUpdateResult;
using umbra::detail::ProcessFederationUpdateAttributeValuesRequest;
using umbra::detail::ProcessFederationUpdateAttributeValuesResult;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::test::process_federation_service_support::composedRestaurantDefinition;
using umbra::test::process_federation_service_support::request;
using umbra::test::process_federation_service_support::resourcePath;
using umbra::test::process_federation_service_support::validatedModule;
}  // namespace

TEST_CASE(
    "Private process service releases timestamped Update Attribute Values before a constrained grant",
    "[unit][foundation][object-management][time-management][time-advance][transport][process-boundary][registry-binding][timestamped-attribute-update][rti.service.update-attribute-values][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.time-advance-request][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto senderConnection = listener->accept(
          nullptr,
          {"process-tso-attribute", 0xB1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(std::move(senderConnection));
      auto receiverConnection = listener->accept(
          nullptr,
          {"process-tso-attribute", 0xB2U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(std::move(receiverConnection));
      auto const senderHandler = service.handlerFor(sender);
      auto const receiverHandler = service.handlerFor(receiver);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        "The process TSO attribute service received an unexpected operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(
              "The process TSO attribute service lost a request.");
        }
      };

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_object_instance_discovery);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request);

      // The timestamped reflection and the matching grant are unsolicited
      // frames on the constrained receiver's process connection. The client
      // reads them after the sender's TAR response has completed.
      // The timestamped reflection and matching grant are unsolicited frames
      // on the constrained receiver connection. The receiver's explicit ACK
      // below is the transport-level proof that the delivery remained in the
      // registry until the callback boundary; no private queue snapshot is
      // required here.
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution);
      service.detach(sender);
      service.detach(receiver);
      sender.connection()->close();
      receiver.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> senderConnection;
  std::shared_ptr<ProcessTransportConnection> receiverConnection;
  try {
    senderConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-tso-attribute", 0xC1U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    receiverConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-tso-attribute", 0xC2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession sender(senderConnection);
    ProcessTransportSession receiver(receiverConnection);
    TransportServiceMessage response;

    REQUIRE(sender.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{L"process-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-tso-attribute-sender",
                    L"process-tso-attribute-sender"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const senderResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::join_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-tso-attribute-receiver",
                    L"process-tso-attribute-receiver"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const receiverResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(senderResult.federateId != receiverResult.federateId);

    auto const objectClass = registry.objectClassHandleFor(
        L"process-execution", "HLAobjectRoot.Employee");
    auto const attribute = registry.attributeHandleFor(
        L"process-execution", "HLAobjectRoot.Employee", "Name");
    REQUIRE(objectClass.has_value());
    REQUIRE(attribute.has_value());

    REQUIRE(sender.request(
        request(
            TransportServiceOperation::publish_object_class_attributes,
            3U,
            umbra::detail::encodeProcessFederationObjectClassAttributeDeclarationRequest(
                umbra::detail::ProcessFederationObjectClassAttributeDeclarationRequest{
                    L"process-execution",
                    senderResult.federateId,
                    *objectClass,
                    {*attribute}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::subscribe_object_class_attributes,
            2U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-execution",
                    receiverResult.federateId,
                    *objectClass,
                    {*attribute},
                    true,
                    "HLAdefault"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    rti1516_2025::HLAinteger64Interval lookahead(0);
    auto const encodedLookahead = lookahead.encode();
    std::vector<std::uint8_t> lookaheadBytes;
    if (encodedLookahead.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(encodedLookahead.data());
      REQUIRE(data != nullptr);
      lookaheadBytes.assign(data, data + encodedLookahead.size());
    }
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::enable_time_regulation,
            4U,
            umbra::detail::encodeProcessFederationEnableTimeRegulationRequest(
                umbra::detail::ProcessFederationEnableTimeRegulationRequest{
                    L"process-execution",
                    senderResult.federateId,
                    ProcessFederationLogicalTimeInterval{
                        L"HLAinteger64Time", std::move(lookaheadBytes)}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(
        umbra::detail::decodeProcessFederationEnableTimeRegulationResult(
            response.payload)
            .status == ProcessFederationTimeEnableStatus::applied);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::enable_time_constrained,
            3U,
            umbra::detail::encodeProcessFederationEnableTimeConstrainedRequest(
                umbra::detail::ProcessFederationEnableTimeConstrainedRequest{
                    L"process-execution", receiverResult.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(
        umbra::detail::decodeProcessFederationEnableTimeConstrainedResult(
            response.payload)
            .status == ProcessFederationTimeEnableStatus::applied);

    REQUIRE(sender.request(
        request(
            TransportServiceOperation::register_object_instance,
            5U,
            umbra::detail::encodeProcessFederationRegisterObjectInstanceRequest(
                umbra::detail::ProcessFederationRegisterObjectInstanceRequest{
                    L"process-execution",
                    senderResult.federateId,
                    *objectClass,
                    std::nullopt})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const registration =
        umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
            response.payload);
    REQUIRE(registration.objectInstanceHandle != 0U);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::receive_object_instance_discovery,
            4U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", receiverResult.federateId})),
        response));
    REQUIRE(
        umbra::detail::decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
            response.payload)
            .event.has_value());

    auto const timestamp = rti1516_2025::HLAinteger64Time(5);
    auto const encodedTimestamp = timestamp.encode();
    std::vector<std::uint8_t> timestampBytes;
    if (encodedTimestamp.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(encodedTimestamp.data());
      REQUIRE(data != nullptr);
      timestampBytes.assign(data, data + encodedTimestamp.size());
    }
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::time_advance_request,
            5U,
            umbra::detail::encodeProcessFederationTimeAdvanceRequest(
                umbra::detail::ProcessFederationTimeAdvanceRequest{
                    L"process-execution",
                    receiverResult.federateId,
                    ProcessFederationLogicalTime{
                        L"HLAinteger64Time", std::move(timestampBytes)}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    std::vector<std::uint8_t> const value{0x54U, 0x53U, 0x4FU};
    std::vector<std::uint8_t> const tag{0x54U, 0x41U, 0x47U};
    auto timestampForUpdate = timestamp.encode();
    std::vector<std::uint8_t> updateTimestampBytes;
    if (timestampForUpdate.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(timestampForUpdate.data());
      REQUIRE(data != nullptr);
      updateTimestampBytes.assign(
          data,
          data + timestampForUpdate.size());
    }
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::update_attribute_values,
            6U,
            umbra::detail::encodeProcessFederationUpdateAttributeValuesRequest(
                ProcessFederationUpdateAttributeValuesRequest{
                    L"process-execution",
                    senderResult.federateId,
                    registration.objectInstanceHandle,
                    {{*attribute, value}},
                    tag,
                    ProcessFederationLogicalTime{
                        L"HLAinteger64Time", std::move(updateTimestampBytes)}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const updateResult =
        umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
            response.payload);
    REQUIRE(updateResult.recipientCount == 1U);
    REQUIRE(updateResult.messageId != 0U);

    auto encodedSenderTimestamp = timestamp.encode();
    std::vector<std::uint8_t> senderTimestampBytes;
    if (encodedSenderTimestamp.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(
          encodedSenderTimestamp.data());
      REQUIRE(data != nullptr);
      senderTimestampBytes.assign(
          data,
          data + encodedSenderTimestamp.size());
    }
    REQUIRE(sender.send(request(
        TransportServiceOperation::time_advance_request,
        7U,
        umbra::detail::encodeProcessFederationTimeAdvanceRequest(
            umbra::detail::ProcessFederationTimeAdvanceRequest{
                L"process-execution",
                senderResult.federateId,
                ProcessFederationLogicalTime{
                    L"HLAinteger64Time", std::move(senderTimestampBytes)}}))));
    bool senderResponseReceived = false;
    while (!senderResponseReceived) {
      REQUIRE(sender.receive(response));
      if (response.kind == TransportServiceMessageKind::response &&
          response.requestId == 7U &&
          response.operation == TransportServiceOperation::time_advance_request) {
        senderResponseReceived = true;
      }
    }
    REQUIRE(response.status == TransportServiceStatus::ok);

    TransportServiceMessage receiverEvent;
    REQUIRE(receiver.receive(receiverEvent));
    REQUIRE(receiverEvent.kind == TransportServiceMessageKind::event);
    REQUIRE(receiverEvent.operation ==
            TransportServiceOperation::receive_attribute_update);
    auto const received =
        umbra::detail::decodeProcessFederationReceiveAttributeUpdateResult(
            receiverEvent.payload);
    REQUIRE(received.event.has_value());
    REQUIRE(received.event->attributeValues.size() == 1U);
    REQUIRE(received.event->attributeValues.front().first == *attribute);
    REQUIRE(received.event->attributeValues.front().second == value);
    REQUIRE(received.event->userSuppliedTag == tag);
    REQUIRE(received.event->timestamp.has_value());
    REQUIRE(received.event->retractionMessageId == updateResult.messageId);

    TransportServiceMessage receiverGrant;
    REQUIRE(receiver.receive(receiverGrant));
    REQUIRE(receiverGrant.kind == TransportServiceMessageKind::event);
    REQUIRE(receiverGrant.operation == TransportServiceOperation::time_advance_grant);
    auto const grant = umbra::detail::decodeProcessFederationTimeAdvanceResult(
        receiverGrant.payload);
    REQUIRE(grant.status == umbra::detail::ProcessFederationTimeAdvanceStatus::applied);
    REQUIRE(grant.grantedTime.has_value());

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::acknowledge_tso_delivery,
            6U,
            umbra::detail::encodeProcessFederationAcknowledgeTsoDeliveryRequest(
                ProcessFederationAcknowledgeTsoDeliveryRequest{
                    L"process-execution",
                    receiverResult.federateId,
                    updateResult.messageId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(
        umbra::detail::decodeProcessFederationTsoDeliveryAcknowledgementResult(
            response.payload)
            .status == ProcessFederationTsoDeliveryAcknowledgementStatus::applied);

    REQUIRE(registry.memberById(
                L"process-execution", senderResult.federateId)
                .has_value());

    REQUIRE(sender.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            8U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-execution",
                    senderResult.federateId,
                    rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            7U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-execution",
                    receiverResult.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    sender.connection()->close();
    receiver.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (senderConnection) {
      senderConnection->close();
    }
    if (receiverConnection) {
      receiverConnection->close();
    }
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
}
