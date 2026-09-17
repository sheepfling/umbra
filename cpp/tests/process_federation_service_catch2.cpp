#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
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
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The process service FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedRestaurantDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-service-mim"),
      validatedModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-service-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != umbra::detail::FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The process service FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

TransportServiceMessage request(
    TransportServiceOperation operation,
    std::uint64_t requestId,
    std::vector<std::uint8_t> payload) {
  return {
      TransportServiceMessageKind::request,
      operation,
      TransportServiceStatus::ok,
      requestId,
      std::move(payload)};
}

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

TEST_CASE(
    "Private process service binds create join and receive-order interaction to the federation registry",
    "[unit][foundation][transport][process-boundary][service-dispatch][registry-binding][transport-contract][interaction-management][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto senderConnection = listener->accept(
          nullptr,
          {"process-service", 0x51U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(std::move(senderConnection));
      auto receiverConnection = listener->accept(
          nullptr,
          {"process-service", 0x52U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(std::move(receiverConnection));
      auto senderHandler = service.handlerFor(sender);
      auto receiverHandler = service.handlerFor(receiver);

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler) ||
          !ProcessTransportServiceDispatcher::serveOne(sender, senderHandler) ||
          !ProcessTransportServiceDispatcher::serveOne(receiver, receiverHandler)) {
        throw std::runtime_error("The process federation service lost a join request.");
      }

      auto const members = registry.membersFor(L"process-execution");
      if (!members || members->size() != 2U) {
        throw std::runtime_error("The process service did not register both members.");
      }
      auto const senderMember = registry.memberByName(
          L"process-execution", L"process-sender");
      auto const receiverMember = registry.memberByName(
          L"process-execution", L"process-receiver");
      auto const interactionClass = registry.interactionClassHandleFor(
          L"process-execution",
          "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
      if (!senderMember || !receiverMember || !interactionClass) {
        throw std::runtime_error("The process service registry identities are incomplete.");
      }
      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler)) {
        throw std::runtime_error("The process logical-time query was not served.");
      }
      if (!ProcessTransportServiceDispatcher::serveOne(
              sender,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::enable_time_regulation) {
                  throw std::runtime_error(
                      "The process service received an unexpected time-role operation.");
                }
                return senderHandler(request);
              })) {
        throw std::runtime_error(
            "The process time-regulation enable request was not served.");
      }
      if (registry.setInteractionClassPublication(
              L"process-execution", senderMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied ||
          registry.setInteractionClassSubscription(
              L"process-execution", receiverMember->id, *interactionClass, true) !=
              InteractionClassDeclarationStatus::applied) {
        throw std::runtime_error("The process service declarations were rejected.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler) ||
          !ProcessTransportServiceDispatcher::serveOne(receiver, receiverHandler)) {
        throw std::runtime_error("The process federation service lost an interaction request.");
      }
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
        {"process-sender", 0x61U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    receiverConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-receiver", 0x62U},
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
                    L"process-sender",
                    L"process-sender"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const senderResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::join_federation_execution,
            3U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-receiver",
                    L"process-receiver"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const receiverResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(senderResult.federateId != receiverResult.federateId);

    // Join establishes one federation-owned temporal object per process
    // federate.  Query Logical Time must read that retained object rather
    // than manufacturing a fresh initial value for every request.
    auto senderTimeState = registry.timeStateFor(
        L"process-execution", senderResult.federateId);
    REQUIRE(senderTimeState != nullptr);
    REQUIRE(senderTimeState->currentTime() != nullptr);
    REQUIRE(senderTimeState->currentTime()->isInitial());

    REQUIRE(sender.request(
        request(
            TransportServiceOperation::query_logical_time,
            4U,
            umbra::detail::encodeProcessFederationQueryLogicalTimeRequest(
                umbra::detail::ProcessFederationQueryLogicalTimeRequest{
                    L"process-execution", senderResult.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const logicalTime =
        umbra::detail::decodeProcessFederationQueryLogicalTimeResult(
            response.payload);
    REQUIRE(logicalTime.time.implementationName == L"HLAinteger64Time");
    REQUIRE_FALSE(logicalTime.time.encoding.empty());

    // Enable Time Regulation crosses the same private seam with an official
    // HLA interval encoding.  The service applies the role to the retained
    // federate state and returns the exact callback time representation.
    rti1516_2025::HLAinteger64Interval lookahead(1);
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
            5U,
            umbra::detail::encodeProcessFederationEnableTimeRegulationRequest(
                ProcessFederationEnableTimeRegulationRequest{
                    L"process-execution",
                    senderResult.federateId,
                    ProcessFederationLogicalTimeInterval{
                        L"HLAinteger64Time", std::move(lookaheadBytes)}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const enableResult =
        umbra::detail::decodeProcessFederationEnableTimeRegulationResult(
            response.payload);
    REQUIRE(enableResult.status == ProcessFederationTimeEnableStatus::applied);
    REQUIRE(enableResult.enabledTime.has_value());
    REQUIRE(enableResult.enabledTime->implementationName == L"HLAinteger64Time");
    auto const enabledSnapshot = senderTimeState->snapshot();
    REQUIRE(enabledSnapshot.timeRegulating);
    REQUIRE(enabledSnapshot.lookahead != nullptr);
    auto const* enabledLookahead = dynamic_cast<rti1516_2025::HLAinteger64Interval const*>(
        enabledSnapshot.lookahead.get());
    REQUIRE(enabledLookahead != nullptr);
    REQUIRE(enabledLookahead->getInterval() == 1);

    auto const interactionClass = registry.interactionClassHandleFor(
        L"process-execution",
        "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
    REQUIRE(interactionClass.has_value());

    std::vector<std::uint8_t> const interactionPayload{0x50U, 0x52U, 0x4fU, 0x43U};
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::send_interaction,
            6U,
            umbra::detail::encodeProcessFederationSendInteractionRequest(
                ProcessFederationSendInteractionRequest{
                    L"process-execution",
                    senderResult.federateId,
                    *interactionClass,
                    {},
                    interactionPayload})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const sendResult =
        umbra::detail::decodeProcessFederationSendInteractionResult(response.payload);
    REQUIRE(sendResult.recipientCount == 1U);

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::receive_interaction,
            7U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", receiverResult.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const receiveResult =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(response.payload);
    REQUIRE(receiveResult.event.has_value());
    REQUIRE(receiveResult.event->producingFederateId == senderResult.federateId);
    REQUIRE(receiveResult.event->receivingFederateId == receiverResult.federateId);
    REQUIRE(receiveResult.event->interactionClassHandle == *interactionClass);
    REQUIRE(receiveResult.event->parameterHandles.empty());
    REQUIRE(receiveResult.event->payload == interactionPayload);
    REQUIRE(receiveResult.event->transportationName == "HLAreliable");

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

TEST_CASE(
    "Private process service routes ordinary Update Attribute Values to a subscribed receiver",
    "[unit][foundation][transport][process-boundary][service-dispatch][registry-binding][transport-contract][object-management][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][rti.service.update-attribute-values][federate.callback.reflect-attribute-values][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::atomic_uint64_t objectClassHandle{0U};
  std::atomic_uint64_t attributeHandle{0U};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto senderConnection = listener->accept(
          nullptr,
          {"process-attribute-update", 0x71U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(std::move(senderConnection));
      auto receiverConnection = listener->accept(
          nullptr,
          {"process-attribute-update", 0x72U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(std::move(receiverConnection));
      auto baseSenderHandler = service.handlerFor(sender);
      auto receiverHandler = service.handlerFor(receiver);
      auto senderHandler = baseSenderHandler;

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler) ||
          !ProcessTransportServiceDispatcher::serveOne(sender, senderHandler) ||
          !ProcessTransportServiceDispatcher::serveOne(receiver, receiverHandler)) {
        throw std::runtime_error("The process attribute-update service lost a join request.");
      }

      auto const senderMember = registry.memberByName(
          L"process-execution", L"process-attribute-sender");
      auto const receiverMember = registry.memberByName(
          L"process-execution", L"process-attribute-receiver");
      auto const objectClass = registry.objectClassHandleFor(
          L"process-execution", "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          L"process-execution", "HLAobjectRoot.Employee", "Name");
      if (!senderMember || !receiverMember || !objectClass || !attribute) {
        throw std::runtime_error("The process attribute-update registry identities are incomplete.");
      }
      objectClassHandle.store(*objectClass, std::memory_order_release);
      attributeHandle.store(*attribute, std::memory_order_release);

      if (!ProcessTransportServiceDispatcher::serveOne(
              sender,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::publish_object_class_attributes) {
                  throw std::runtime_error(
                      "The process attribute-update service received an unexpected publication operation.");
                }
                return senderHandler(request);
              }) ||
          !ProcessTransportServiceDispatcher::serveOne(
              receiver,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::subscribe_object_class_attributes) {
                  throw std::runtime_error(
                      "The process attribute-update service received an unexpected subscription operation.");
                }
                return receiverHandler(request);
              })) {
        throw std::runtime_error(
            "The process attribute-update service lost its declarations.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler)) {
        throw std::runtime_error("The process attribute-update service lost registration.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(
              receiver,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::receive_object_instance_discovery) {
                  throw std::runtime_error(
                      "The process attribute-update service received an unexpected discovery poll.");
                }
                return receiverHandler(request);
              })) {
        throw std::runtime_error(
            "The process attribute-update service lost object discovery.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler) ||
          !ProcessTransportServiceDispatcher::serveOne(receiver, receiverHandler)) {
        throw std::runtime_error(
            "The process attribute-update service lost update or receive.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(
              receiver,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::local_delete_object_instance) {
                  throw std::runtime_error(
                      "The process attribute-update service received an unexpected local-delete operation.");
                }
                return receiverHandler(request);
              })) {
        throw std::runtime_error(
            "The process attribute-update service lost its local delete.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(
              receiver,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::unsubscribe_object_class_attributes) {
                  throw std::runtime_error(
                      "The process attribute-update service received an unexpected unsubscribe operation.");
                }
                return receiverHandler(request);
              })) {
        throw std::runtime_error(
            "The process attribute-update service lost its unsubscribe.");
      }

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
        {"process-attribute-sender", 0x81U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    receiverConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-attribute-receiver", 0x82U},
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
                    L"process-attribute-sender",
                    L"process-attribute-sender"})),
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
                    L"process-attribute-receiver",
                    L"process-attribute-receiver"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const receiverResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(senderResult.federateId != receiverResult.federateId);

    while (objectClassHandle.load(std::memory_order_acquire) == 0U ||
           attributeHandle.load(std::memory_order_acquire) == 0U) {
      std::this_thread::yield();
    }
    auto const objectClass = objectClassHandle.load(std::memory_order_acquire);
    auto const attribute = attributeHandle.load(std::memory_order_acquire);
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::publish_object_class_attributes,
            3U,
            umbra::detail::encodeProcessFederationObjectClassAttributeDeclarationRequest(
                umbra::detail::ProcessFederationObjectClassAttributeDeclarationRequest{
                    L"process-execution", senderResult.federateId, objectClass, {attribute}})),
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
                    objectClass,
                    {attribute},
                    true,
                    ""})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    REQUIRE(sender.request(
        request(
            TransportServiceOperation::register_object_instance,
            4U,
            umbra::detail::encodeProcessFederationRegisterObjectInstanceRequest(
                umbra::detail::ProcessFederationRegisterObjectInstanceRequest{
                    L"process-execution",
                    senderResult.federateId,
                    objectClass,
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
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const discoveryResult =
        umbra::detail::decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
            response.payload);
    REQUIRE(discoveryResult.event.has_value());
    REQUIRE(discoveryResult.event->receivingFederateId ==
            receiverResult.federateId);
    REQUIRE(discoveryResult.event->objectInstanceHandle ==
            registration.objectInstanceHandle);
    REQUIRE(discoveryResult.event->objectClassHandle == objectClass);
    REQUIRE_FALSE(discoveryResult.event->objectInstanceName.empty());
    REQUIRE(discoveryResult.event->producingFederateId == senderResult.federateId);

    std::vector<std::uint8_t> const value{0x45U, 0x6DU, 0x70U};
    std::vector<std::uint8_t> const tag{0x54U, 0x41U, 0x47U};
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::update_attribute_values,
            6U,
            umbra::detail::encodeProcessFederationUpdateAttributeValuesRequest(
                ProcessFederationUpdateAttributeValuesRequest{
                    L"process-execution",
                    senderResult.federateId,
                    registration.objectInstanceHandle,
                    {{attribute, value}},
                    tag})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const updateResult =
        umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
            response.payload);
    REQUIRE(updateResult.recipientCount == 1U);

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::receive_attribute_update,
            5U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", receiverResult.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const receiveResult =
        umbra::detail::decodeProcessFederationReceiveAttributeUpdateResult(
            response.payload);
    REQUIRE(receiveResult.event.has_value());
    REQUIRE(receiveResult.event->producingFederateId == senderResult.federateId);
    REQUIRE(receiveResult.event->receivingFederateId == receiverResult.federateId);
    REQUIRE(receiveResult.event->objectInstanceHandle ==
            registration.objectInstanceHandle);
    REQUIRE(receiveResult.event->attributeValues.size() == 1U);
    REQUIRE(receiveResult.event->attributeValues.front().first == attribute);
    REQUIRE(receiveResult.event->attributeValues.front().second == value);
    REQUIRE(receiveResult.event->userSuppliedTag == tag);
    REQUIRE(receiveResult.event->transportationName == "HLAreliable");

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::local_delete_object_instance,
            6U,
            umbra::detail::encodeProcessFederationLocalDeleteObjectInstanceRequest(
                ProcessFederationLocalDeleteObjectInstanceRequest{
                    L"process-execution",
                    receiverResult.federateId,
                    registration.objectInstanceHandle})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const localDeleteResult =
        umbra::detail::decodeProcessFederationLocalDeleteObjectInstanceResult(
            response.payload);
    REQUIRE(localDeleteResult.status ==
            umbra::detail::LocalObjectInstanceDeletionStatus::applied);
    REQUIRE_FALSE(registry.knownObjectInstanceFor(
        L"process-execution",
        receiverResult.federateId,
        registration.objectInstanceHandle));

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::unsubscribe_object_class_attributes,
            7U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-execution",
                    receiverResult.federateId,
                    objectClass,
                    {},
                    false,
                    ""})),
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

TEST_CASE(
    "Private process service projects owner-directed Attribute Relevance Advisory events",
    "[unit][foundation][object-management][data-distribution-management][callbacks][transport][process-boundary][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.publish-object-class-attributes][rti.service.register-object-instance][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-relevance-owner", 0xA1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession owner(std::move(ownerConnection));
      auto receiverConnection = listener->accept(
          nullptr,
          {"process-relevance-receiver", 0xA2U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(std::move(receiverConnection));
      auto const ownerHandler = service.handlerFor(owner);
      auto const receiverHandler = service.handlerFor(receiver);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process relevance service lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          L"process-execution", "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          L"process-execution", "HLAobjectRoot.Employee", "Name");
      auto const payRate = registry.attributeHandleFor(
          L"process-execution", "HLAobjectRoot.Employee", "PayRate");
      if (!objectClass || !attribute || !payRate) {
        throw std::runtime_error(
            "The process relevance service could not resolve its FOM handles.");
      }
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process relevance service lost owner Join.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process relevance service lost receiver Join.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::set_attribute_relevance_advisory_switch,
          "The process relevance service lost relevance-switch Set.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::get_attribute_relevance_advisory_switch,
          "The process relevance service lost relevance-switch Get.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process relevance service lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process relevance service lost initial Subscribe.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The process relevance service lost Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_object_instance_discovery,
          "The process relevance service lost discovery polling.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process relevance service lost initial Turn Updates On polling.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process relevance service lost PayRate Subscribe.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process relevance service lost Turn Updates On polling.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::unsubscribe_object_class_attributes,
          "The process relevance service lost PayRate Unsubscribe.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The process relevance service lost Turn Updates Off polling.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process relevance service lost owner Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process relevance service lost receiver Resign.");
      service.detach(owner);
      service.detach(receiver);
      owner.connection()->close();
      receiver.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> ownerConnection;
  std::shared_ptr<ProcessTransportConnection> receiverConnection;
  try {
    ownerConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-relevance-owner", 0xB1U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    receiverConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-relevance-receiver", 0xB2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession owner(ownerConnection);
    ProcessTransportSession receiver(receiverConnection);
    TransportServiceMessage response;

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{L"process-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(owner.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-relevance-owner",
                    L"process-relevance-owner"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const ownerJoin = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::join_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-relevance-receiver",
                    L"process-relevance-receiver"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const receiverJoin = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(ownerJoin.federateId != receiverJoin.federateId);

    auto const objectClass = registry.objectClassHandleFor(
        L"process-execution", "HLAobjectRoot.Employee");
    auto const attribute = registry.attributeHandleFor(
        L"process-execution", "HLAobjectRoot.Employee", "Name");
    auto const payRate = registry.attributeHandleFor(
        L"process-execution", "HLAobjectRoot.Employee", "PayRate");
    REQUIRE(objectClass.has_value());
    REQUIRE(attribute.has_value());
    REQUIRE(payRate.has_value());

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            3U,
            umbra::detail::encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
                umbra::detail::ProcessFederationAttributeScopeAdvisorySwitchRequest{
                    L"process-execution", ownerJoin.federateId, true})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(owner.request(
        request(
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            4U,
            umbra::detail::encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
                umbra::detail::ProcessFederationAttributeScopeAdvisorySwitchRequest{
                    L"process-execution", ownerJoin.federateId, false})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(umbra::detail::decodeProcessFederationBooleanResult(response.payload).value);

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::publish_object_class_attributes,
            5U,
            umbra::detail::encodeProcessFederationObjectClassAttributeDeclarationRequest(
                umbra::detail::ProcessFederationObjectClassAttributeDeclarationRequest{
                    L"process-execution",
                    ownerJoin.federateId,
                    *objectClass,
                    {*attribute, *payRate}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::subscribe_object_class_attributes,
            2U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-execution",
                    receiverJoin.federateId,
                    *objectClass,
                    {*attribute},
                    true,
                    ""})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(owner.request(
        request(
            TransportServiceOperation::register_object_instance,
            6U,
            umbra::detail::encodeProcessFederationRegisterObjectInstanceRequest(
                umbra::detail::ProcessFederationRegisterObjectInstanceRequest{
                    L"process-execution",
                    ownerJoin.federateId,
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
            3U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", receiverJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const discovery =
        umbra::detail::decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
            response.payload);
    REQUIRE(discovery.event.has_value());
    REQUIRE(discovery.event->objectInstanceHandle ==
            registration.objectInstanceHandle);

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::receive_interaction,
            7U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", ownerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const initialTurnOn =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE(initialTurnOn.attributeRelevanceAdvisoryEvent.has_value());
    REQUIRE(initialTurnOn.attributeRelevanceAdvisoryEvent->providingFederateId ==
            ownerJoin.federateId);
    REQUIRE(initialTurnOn.attributeRelevanceAdvisoryEvent->receivingFederateId == 0U);
    REQUIRE(initialTurnOn.attributeRelevanceAdvisoryEvent->objectInstanceHandle ==
            registration.objectInstanceHandle);
    REQUIRE(initialTurnOn.attributeRelevanceAdvisoryEvent->attributeHandles ==
            std::set<std::uint64_t>{*attribute});
    REQUIRE(initialTurnOn.attributeRelevanceAdvisoryEvent->turnUpdatesOn);
    REQUIRE_FALSE(initialTurnOn.attributeRelevanceAdvisoryEvent->updateRateDesignator.has_value());

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::subscribe_object_class_attributes,
            4U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-execution",
                    receiverJoin.federateId,
                    *objectClass,
                    {*payRate},
                    true,
                    "High"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::receive_interaction,
            8U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", ownerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const turnOn =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent.has_value());
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent->providingFederateId ==
            ownerJoin.federateId);
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent->receivingFederateId == 0U);
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent->objectInstanceHandle ==
            registration.objectInstanceHandle);
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent->attributeHandles ==
            std::set<std::uint64_t>{*payRate});
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent->turnUpdatesOn);
    REQUIRE(turnOn.attributeRelevanceAdvisoryEvent->updateRateDesignator ==
            std::optional<std::string>{"High"});

    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::unsubscribe_object_class_attributes,
            5U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-execution",
                    receiverJoin.federateId,
                    *objectClass,
                    {*payRate},
                    false,
                    ""})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::receive_interaction,
            9U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", ownerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const turnOff =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE(turnOff.attributeRelevanceAdvisoryEvent.has_value());
    REQUIRE(turnOff.attributeRelevanceAdvisoryEvent->providingFederateId ==
            ownerJoin.federateId);
    REQUIRE(turnOff.attributeRelevanceAdvisoryEvent->objectInstanceHandle ==
            registration.objectInstanceHandle);
    REQUIRE(turnOff.attributeRelevanceAdvisoryEvent->attributeHandles ==
            std::set<std::uint64_t>{*payRate});
    REQUIRE_FALSE(turnOff.attributeRelevanceAdvisoryEvent->turnUpdatesOn);
    REQUIRE_FALSE(turnOff.attributeRelevanceAdvisoryEvent->updateRateDesignator.has_value());

    REQUIRE(owner.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            10U,
            umbra::detail::encodeProcessFederationResignRequest(
                umbra::detail::ProcessFederationResignRequest{
                    L"process-execution", ownerJoin.federateId, rti1516_2025::DELETE_OBJECTS})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(receiver.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            5U,
            umbra::detail::encodeProcessFederationResignRequest(
                umbra::detail::ProcessFederationResignRequest{
                    L"process-execution", receiverJoin.federateId, rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    owner.connection()->close();
    receiver.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (ownerConnection) {
      ownerConnection->close();
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

TEST_CASE(
    "Private process local-delete request and result preserve the official status vocabulary",
    "[unit][foundation][transport][process-boundary][object-management][local-delete-object-instance][transport-contract][rti.service.local-delete-object-instance]") {
  ProcessFederationLocalDeleteObjectInstanceRequest const requestValue{
      L"process-execution", 0x51U, 0x8aU};
  auto const encodedRequest =
      umbra::detail::encodeProcessFederationLocalDeleteObjectInstanceRequest(
          requestValue);
  auto const decodedRequest =
      umbra::detail::decodeProcessFederationLocalDeleteObjectInstanceRequest(
          encodedRequest);
  REQUIRE(decodedRequest.federationName == requestValue.federationName);
  REQUIRE(decodedRequest.federateId == requestValue.federateId);
  REQUIRE(decodedRequest.objectInstanceHandle ==
          requestValue.objectInstanceHandle);

  for (auto const status : {
           umbra::detail::LocalObjectInstanceDeletionStatus::applied,
           umbra::detail::LocalObjectInstanceDeletionStatus::federation_does_not_exist,
           umbra::detail::LocalObjectInstanceDeletionStatus::federate_not_member,
           umbra::detail::LocalObjectInstanceDeletionStatus::object_instance_not_known,
           umbra::detail::LocalObjectInstanceDeletionStatus::ownership_acquisition_pending,
           umbra::detail::LocalObjectInstanceDeletionStatus::federate_owns_attributes}) {
    ProcessFederationLocalDeleteObjectInstanceResult const resultValue{status};
    auto const encodedResult =
        umbra::detail::encodeProcessFederationLocalDeleteObjectInstanceResult(
            resultValue);
    auto const decodedResult =
        umbra::detail::decodeProcessFederationLocalDeleteObjectInstanceResult(
            encodedResult);
    REQUIRE(decodedResult.status == status);
  }
}

TEST_CASE(
    "Private process Delete Object Instance request and result preserve the official status vocabulary",
    "[unit][foundation][transport][process-boundary][object-management][delete-object-instance][transport-contract][2025][rti.service.delete-object-instance]") {
  ProcessFederationDeleteObjectInstanceRequest const requestValue{
      L"process-execution",
      0x52U,
      0x8bU,
      {0x44U, 0x45U, 0x4cU},
      ProcessFederationLogicalTime{L"HLAinteger64Time", {0x01U, 0x02U}}};
  auto const encodedRequest =
      umbra::detail::encodeProcessFederationDeleteObjectInstanceRequest(
          requestValue);
  auto const decodedRequest =
      umbra::detail::decodeProcessFederationDeleteObjectInstanceRequest(
          encodedRequest);
  REQUIRE(decodedRequest.federationName == requestValue.federationName);
  REQUIRE(decodedRequest.federateId == requestValue.federateId);
  REQUIRE(decodedRequest.objectInstanceHandle ==
          requestValue.objectInstanceHandle);
  REQUIRE(decodedRequest.userSuppliedTag == requestValue.userSuppliedTag);
  REQUIRE(decodedRequest.timestamp.has_value());
  REQUIRE(decodedRequest.timestamp->implementationName ==
          requestValue.timestamp->implementationName);
  REQUIRE(decodedRequest.timestamp->encoding ==
          requestValue.timestamp->encoding);

  for (auto const status : {
           umbra::detail::ObjectInstanceDeletionStatus::applied,
           umbra::detail::ObjectInstanceDeletionStatus::federation_does_not_exist,
           umbra::detail::ObjectInstanceDeletionStatus::federate_not_member,
           umbra::detail::ObjectInstanceDeletionStatus::object_instance_not_known,
           umbra::detail::ObjectInstanceDeletionStatus::delete_privilege_not_held,
           umbra::detail::ObjectInstanceDeletionStatus::inconsistent_catalog}) {
    auto const recipientCount =
        status == umbra::detail::ObjectInstanceDeletionStatus::applied ? 3U : 0U;
    auto const messageId =
        status == umbra::detail::ObjectInstanceDeletionStatus::applied ? 0x91U : 0U;
    ProcessFederationDeleteObjectInstanceResult const resultValue{
        status, recipientCount, messageId};
    auto const encodedResult =
        umbra::detail::encodeProcessFederationDeleteObjectInstanceResult(
            resultValue);
    auto const decodedResult =
        umbra::detail::decodeProcessFederationDeleteObjectInstanceResult(
            encodedResult);
    REQUIRE(decodedResult.status == status);
    REQUIRE(decodedResult.recipientCount == recipientCount);
    REQUIRE(decodedResult.messageId == messageId);
  }
}

TEST_CASE(
    "Private process service routes object-instance Request Attribute Value Update to the owning provider",
    "[unit][foundation][object-management][callbacks][transport][process-boundary][service-dispatch][registry-binding][transport-contract][rti.service.request-attribute-value-update][rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes][rti.service.register-object-instance][federate.callback.provide-attribute-value-update][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::atomic_uint64_t objectClassHandle{0U};
  std::atomic_uint64_t attributeHandle{0U};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto providerConnection = listener->accept(
          nullptr,
          {"process-attribute-request-provider", 0xC1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession provider(std::move(providerConnection));
      auto requesterConnection = listener->accept(
          nullptr,
          {"process-attribute-request-requester", 0xC2U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requester(std::move(requesterConnection));
      auto const providerHandler = service.handlerFor(provider);
      auto const requesterHandler = service.handlerFor(requester);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& incoming) {
                  if (incoming.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(incoming);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process attribute-request service lost Create.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process attribute-request service lost provider Join.");
      auto const objectClass = registry.objectClassHandleFor(
          L"process-execution", "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          L"process-execution", "HLAobjectRoot.Employee", "Name");
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The process attribute-request service could not resolve its FOM handles.");
      }
      objectClassHandle.store(*objectClass, std::memory_order_release);
      attributeHandle.store(*attribute, std::memory_order_release);
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process attribute-request service lost requester Join.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process attribute-request service lost Publish.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process attribute-request service lost Subscribe.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::register_object_instance,
          "The process attribute-request service lost Register.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::receive_object_instance_discovery,
          "The process attribute-request service lost discovery polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The process attribute-request service lost initial relevance polling.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::request_attribute_value_update,
          "The process attribute-request service lost Request Attribute Value Update.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The process attribute-request service lost provider callback polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process attribute-request service lost provider Resign.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process attribute-request service lost requester Resign.");
      service.detach(provider);
      service.detach(requester);
      provider.connection()->close();
      requester.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> providerConnection;
  std::shared_ptr<ProcessTransportConnection> requesterConnection;
  try {
    providerConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-attribute-request-provider", 0xD1U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    requesterConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-attribute-request-requester", 0xD2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession provider(providerConnection);
    ProcessTransportSession requester(requesterConnection);
    TransportServiceMessage response;

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{L"process-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-attribute-request-provider",
                    L"process-attribute-request-provider"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const providerJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::join_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-execution",
                    L"process-attribute-request-requester",
                    L"process-attribute-request-requester"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requesterJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(providerJoin.federateId != requesterJoin.federateId);

    while (objectClassHandle.load(std::memory_order_acquire) == 0U ||
           attributeHandle.load(std::memory_order_acquire) == 0U) {
      std::this_thread::yield();
    }
    auto const objectClass = objectClassHandle.load(std::memory_order_acquire);
    auto const attribute = attributeHandle.load(std::memory_order_acquire);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::publish_object_class_attributes,
            3U,
            umbra::detail::encodeProcessFederationObjectClassAttributeDeclarationRequest(
                umbra::detail::ProcessFederationObjectClassAttributeDeclarationRequest{
                    L"process-execution",
                    providerJoin.federateId,
                    objectClass,
                    {attribute}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::subscribe_object_class_attributes,
            2U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-execution",
                    requesterJoin.federateId,
                    objectClass,
                    {attribute},
                    true,
                    ""})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::register_object_instance,
            4U,
            umbra::detail::encodeProcessFederationRegisterObjectInstanceRequest(
                umbra::detail::ProcessFederationRegisterObjectInstanceRequest{
                    L"process-execution",
                    providerJoin.federateId,
                    objectClass,
                    std::nullopt})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const registration =
        umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
            response.payload);
    REQUIRE(registration.objectInstanceHandle != 0U);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::receive_object_instance_discovery,
            3U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", requesterJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const discovery =
        umbra::detail::decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
            response.payload);
    REQUIRE(discovery.event.has_value());
    REQUIRE(discovery.event->objectInstanceHandle ==
            registration.objectInstanceHandle);

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::receive_interaction,
            5U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", providerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const initialAdvisory =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE(initialAdvisory.attributeRelevanceAdvisoryEvent.has_value());
    REQUIRE(initialAdvisory.attributeRelevanceAdvisoryEvent->providingFederateId ==
            providerJoin.federateId);

    auto const requestPlan = registry.planAttributeValueUpdateRequest(
        L"process-execution",
        requesterJoin.federateId,
        registration.objectInstanceHandle,
        std::set<std::uint64_t>{attribute});
    REQUIRE(requestPlan.status ==
            umbra::detail::AttributeValueUpdateRequestStatus::applied);
    REQUIRE(requestPlan.recipients.size() == 1U);
    REQUIRE(requestPlan.recipients.front().providingFederateId ==
            providerJoin.federateId);

    std::vector<std::uint8_t> const tag{0x52U, 0x45U, 0x51U};
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::request_attribute_value_update,
            6U,
            umbra::detail::encodeProcessFederationRequestAttributeValueUpdateRequest(
                ProcessFederationRequestAttributeValueUpdateRequest{
                    L"process-execution",
                    requesterJoin.federateId,
                    registration.objectInstanceHandle,
                    {attribute},
                    tag})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requestResult =
        umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
            response.payload);
    REQUIRE(requestResult.recipientCount == 1U);

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::receive_interaction,
            7U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-execution", providerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const callback =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE(callback.attributeValueUpdateRequestEvent.has_value());
    REQUIRE(callback.attributeValueUpdateRequestEvent->requestingFederateId ==
            requesterJoin.federateId);
    REQUIRE(callback.attributeValueUpdateRequestEvent->providingFederateId ==
            providerJoin.federateId);
    REQUIRE(callback.attributeValueUpdateRequestEvent->objectInstanceHandle ==
            registration.objectInstanceHandle);
    REQUIRE(callback.attributeValueUpdateRequestEvent->requestedAttributeHandles ==
            std::set<std::uint64_t>{attribute});
    REQUIRE(callback.attributeValueUpdateRequestEvent->userSuppliedTag == tag);

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            6U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-execution",
                    providerJoin.federateId,
                    rti1516_2025::DELETE_OBJECTS})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            6U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-execution",
                    requesterJoin.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    provider.connection()->close();
    requester.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (providerConnection) {
      providerConnection->close();
    }
  if (requesterConnection) {
      requesterConnection->close();
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

TEST_CASE(
    "Private process service expands object-class Request Attribute Value Update across registered instances",
    "[unit][foundation][object-management][callbacks][transport][process-boundary][service-dispatch][registry-binding][transport-contract][rti.service.request-attribute-value-update][rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes][rti.service.register-object-instance][federate.callback.provide-attribute-value-update][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::atomic_uint64_t objectClassHandle{0U};
  std::atomic_uint64_t attributeHandle{0U};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto providerConnection = listener->accept(
          nullptr,
          {"process-class-attribute-provider", 0xE1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession provider(std::move(providerConnection));
      auto requesterConnection = listener->accept(
          nullptr,
          {"process-class-attribute-requester", 0xE2U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requester(std::move(requesterConnection));
      auto const providerHandler = service.handlerFor(provider);
      auto const requesterHandler = service.handlerFor(requester);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& incoming) {
                  if (incoming.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(incoming);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::create_federation_execution,
          "The process class-request service lost Create.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::join_federation_execution,
          "The process class-request service lost provider Join.");
      auto const objectClass = registry.objectClassHandleFor(
          L"process-class-execution", "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          L"process-class-execution", "HLAobjectRoot.Employee", "Name");
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The process class-request service could not resolve its FOM handles.");
      }
      objectClassHandle.store(*objectClass, std::memory_order_release);
      attributeHandle.store(*attribute, std::memory_order_release);
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The process class-request service lost requester Join.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process class-request service lost Publish.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process class-request service lost Subscribe.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::register_object_instance,
          "The process class-request service lost first Register.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::receive_object_instance_discovery,
          "The process class-request service lost first discovery polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The process class-request service lost first relevance polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::register_object_instance,
          "The process class-request service lost second Register.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::receive_object_instance_discovery,
          "The process class-request service lost second discovery polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The process class-request service lost second relevance polling.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::request_attribute_value_update_class,
          "The process class-request service lost class Request Attribute Value Update.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The process class-request service lost first provider callback polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The process class-request service lost second provider callback polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process class-request service lost provider Resign.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process class-request service lost requester Resign.");
      service.detach(provider);
      service.detach(requester);
      provider.connection()->close();
      requester.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> providerConnection;
  std::shared_ptr<ProcessTransportConnection> requesterConnection;
  try {
    providerConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-class-attribute-provider", 0xF1U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    requesterConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-class-attribute-requester", 0xF2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession provider(providerConnection);
    ProcessTransportSession requester(requesterConnection);
    TransportServiceMessage response;

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{L"process-class-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-class-execution",
                    L"process-class-attribute-provider",
                    L"process-class-attribute-provider"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const providerJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::join_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-class-execution",
                    L"process-class-attribute-requester",
                    L"process-class-attribute-requester"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requesterJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(providerJoin.federateId != requesterJoin.federateId);

    while (objectClassHandle.load(std::memory_order_acquire) == 0U ||
           attributeHandle.load(std::memory_order_acquire) == 0U) {
      std::this_thread::yield();
    }
    auto const objectClass = objectClassHandle.load(std::memory_order_acquire);
    auto const attribute = attributeHandle.load(std::memory_order_acquire);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::publish_object_class_attributes,
            3U,
            umbra::detail::encodeProcessFederationObjectClassAttributeDeclarationRequest(
                umbra::detail::ProcessFederationObjectClassAttributeDeclarationRequest{
                    L"process-class-execution",
                    providerJoin.federateId,
                    objectClass,
                    {attribute}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::subscribe_object_class_attributes,
            2U,
            umbra::detail::encodeProcessFederationObjectClassAttributeSubscriptionRequest(
                umbra::detail::ProcessFederationObjectClassAttributeSubscriptionRequest{
                    L"process-class-execution",
                    requesterJoin.federateId,
                    objectClass,
                    {attribute},
                    true,
                    ""})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    std::vector<std::uint64_t> objectInstances(2U);
    for (std::size_t index = 0U; index < objectInstances.size(); ++index) {
      REQUIRE(provider.request(
          request(
              TransportServiceOperation::register_object_instance,
              static_cast<std::uint64_t>(4U + index),
              umbra::detail::encodeProcessFederationRegisterObjectInstanceRequest(
                  umbra::detail::ProcessFederationRegisterObjectInstanceRequest{
                      L"process-class-execution",
                      providerJoin.federateId,
                      objectClass,
                      std::nullopt})),
          response));
      REQUIRE(response.status == TransportServiceStatus::ok);
      auto const registration =
          umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
              response.payload);
      REQUIRE(registration.objectInstanceHandle != 0U);
      objectInstances[index] = registration.objectInstanceHandle;
      REQUIRE(requester.request(
          request(
              TransportServiceOperation::receive_object_instance_discovery,
              static_cast<std::uint64_t>(3U + index),
              umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                  ProcessFederationReceiveInteractionRequest{
                      L"process-class-execution", requesterJoin.federateId})),
          response));
      REQUIRE(response.status == TransportServiceStatus::ok);
      auto const discovery =
          umbra::detail::decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
              response.payload);
      REQUIRE(discovery.event.has_value());
      REQUIRE(discovery.event->objectInstanceHandle ==
              objectInstances[index]);
      REQUIRE(provider.request(
          request(
              TransportServiceOperation::receive_interaction,
              static_cast<std::uint64_t>(5U + index),
              umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                  ProcessFederationReceiveInteractionRequest{
                      L"process-class-execution", providerJoin.federateId})),
          response));
      REQUIRE(response.status == TransportServiceStatus::ok);
      auto const advisory =
          umbra::detail::decodeProcessFederationReceiveInteractionResult(
              response.payload);
      REQUIRE(advisory.attributeRelevanceAdvisoryEvent.has_value());
    }

    std::vector<std::uint8_t> const tag{0x43U, 0x4CU, 0x53U};
    auto const encodedClassRequest =
        umbra::detail::encodeProcessFederationRequestAttributeValueUpdateClassRequest(
            umbra::detail::ProcessFederationRequestAttributeValueUpdateClassRequest{
                L"process-class-execution",
                requesterJoin.federateId,
                objectClass,
                {attribute},
                tag});
    auto const decodedClassRequest =
        umbra::detail::decodeProcessFederationRequestAttributeValueUpdateClassRequest(
            encodedClassRequest);
    REQUIRE(decodedClassRequest.objectClassHandle == objectClass);
    REQUIRE(decodedClassRequest.requestedAttributeHandles ==
            std::vector<std::uint64_t>{attribute});
    REQUIRE(decodedClassRequest.userSuppliedTag == tag);

    REQUIRE(requester.request(
        request(
            TransportServiceOperation::request_attribute_value_update_class,
            6U,
            encodedClassRequest),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requestResult =
        umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
            response.payload);
    REQUIRE(requestResult.recipientCount == 2U);

    std::set<std::uint64_t> observedInstances;
    for (std::size_t index = 0U; index < objectInstances.size(); ++index) {
      REQUIRE(provider.request(
          request(
              TransportServiceOperation::receive_interaction,
              static_cast<std::uint64_t>(8U + index),
              umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                  ProcessFederationReceiveInteractionRequest{
                      L"process-class-execution", providerJoin.federateId})),
          response));
      REQUIRE(response.status == TransportServiceStatus::ok);
      auto const callback =
          umbra::detail::decodeProcessFederationReceiveInteractionResult(
              response.payload);
      REQUIRE(callback.attributeValueUpdateRequestEvent.has_value());
      REQUIRE(callback.attributeValueUpdateRequestEvent->requestingFederateId ==
              requesterJoin.federateId);
      REQUIRE(callback.attributeValueUpdateRequestEvent->providingFederateId ==
              providerJoin.federateId);
      REQUIRE(callback.attributeValueUpdateRequestEvent->requestedAttributeHandles ==
              std::set<std::uint64_t>{attribute});
      REQUIRE(callback.attributeValueUpdateRequestEvent->userSuppliedTag == tag);
      observedInstances.insert(
          callback.attributeValueUpdateRequestEvent->objectInstanceHandle);
    }
    REQUIRE(observedInstances ==
            std::set<std::uint64_t>{objectInstances.begin(), objectInstances.end()});

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            10U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-class-execution",
                    providerJoin.federateId,
                    rti1516_2025::DELETE_OBJECTS})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            11U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-class-execution",
                    requesterJoin.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    provider.connection()->close();
    requester.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (providerConnection) {
      providerConnection->close();
    }
    if (requesterConnection) {
      requesterConnection->close();
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

TEST_CASE(
    "Private process service preserves regions for regional class Request Attribute Value Update",
    "[unit][foundation][object-management][ddm][callbacks][transport][process-boundary][service-dispatch][registry-binding][transport-contract][rti.service.request-attribute-value-update-with-regions][rti.service.register-object-instance-with-regions][federate.callback.provide-attribute-value-update][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::atomic_uint64_t objectClassHandle{0U};
  std::atomic_uint64_t attributeHandle{0U};
  std::atomic_uint64_t requesterRegionHandle{0U};
  std::atomic_uint64_t registeredObjectHandle{0U};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto providerConnection = listener->accept(
          nullptr,
          {"process-regional-class-provider", 0xE3U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession provider(std::move(providerConnection));
      auto requesterConnection = listener->accept(
          nullptr,
          {"process-regional-class-requester", 0xE4U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requester(std::move(requesterConnection));
      auto const providerHandler = service.handlerFor(provider);
      auto const requesterHandler = service.handlerFor(requester);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& incoming) {
                  if (incoming.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(incoming);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::create_federation_execution,
          "The regional class-request service lost Create.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional class-request service lost provider Join.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional class-request service lost requester Join.");

      auto const providerMember = registry.memberByName(
          L"process-regional-class-execution",
          L"process-regional-class-provider");
      auto const requesterMember = registry.memberByName(
          L"process-regional-class-execution",
          L"process-regional-class-requester");
      auto const objectClass = registry.objectClassHandleFor(
          L"process-regional-class-execution", "HLAobjectRoot.Food.Drink.Soda");
      auto const attribute = registry.attributeHandleFor(
          L"process-regional-class-execution",
          "HLAobjectRoot.Food.Drink.Soda",
          "Flavor");
      auto const dimension = registry.dimensionHandleFor(
          L"process-regional-class-execution", "SodaFlavor");
      if (!providerMember || !requesterMember || !objectClass || !attribute ||
          !dimension) {
        throw std::runtime_error(
            "The regional class-request service could not resolve its FOM handles.");
      }
      if (registry.setObjectClassAttributePublication(
              L"process-regional-class-execution",
              providerMember->id,
              *objectClass,
              std::set<std::uint64_t>{*attribute},
              true) !=
              umbra::detail::ObjectClassAttributeDeclarationStatus::applied ||
          registry.setObjectClassAttributeSubscription(
              L"process-regional-class-execution",
              requesterMember->id,
              *objectClass,
              std::set<std::uint64_t>{*attribute},
              true) !=
              umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
        throw std::runtime_error(
            "The regional class-request service rejected its declarations.");
      }
      auto const providerRegion = registry.createRegion(
          L"process-regional-class-execution", providerMember->id, {*dimension});
      auto const requesterRegion = registry.createRegion(
          L"process-regional-class-execution", requesterMember->id, {*dimension});
      if (providerRegion.status != umbra::detail::RegionServiceStatus::applied ||
          requesterRegion.status != umbra::detail::RegionServiceStatus::applied ||
          registry.setRangeBounds(
              L"process-regional-class-execution",
              providerMember->id,
              providerRegion.regionHandle,
              *dimension,
              umbra::detail::RegionRangeBounds{0UL, 2UL}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.setRangeBounds(
              L"process-regional-class-execution",
              requesterMember->id,
              requesterRegion.regionHandle,
              *dimension,
              umbra::detail::RegionRangeBounds{0UL, 2UL}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.commitRegionModifications(
              L"process-regional-class-execution",
              providerMember->id,
              {providerRegion.regionHandle}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.commitRegionModifications(
              L"process-regional-class-execution",
              requesterMember->id,
              {requesterRegion.regionHandle}) !=
              umbra::detail::RegionServiceStatus::applied) {
        throw std::runtime_error(
            "The regional class-request service rejected its regions.");
      }
      std::map<std::uint64_t, std::set<std::uint64_t>> updateRegions{
          {*attribute, {providerRegion.regionHandle}}};
      auto const registration = registry.registerObjectInstance(
          L"process-regional-class-execution",
          providerMember->id,
          *objectClass,
          &updateRegions);
      if (registration.status !=
              umbra::detail::ObjectInstanceRegistrationStatus::applied ||
          registration.objectInstanceHandle == 0U) {
        throw std::runtime_error(
            "The regional class-request service rejected its object instance.");
      }
      objectClassHandle.store(*objectClass, std::memory_order_release);
      attributeHandle.store(*attribute, std::memory_order_release);
      requesterRegionHandle.store(
          requesterRegion.regionHandle, std::memory_order_release);
      registeredObjectHandle.store(
          registration.objectInstanceHandle, std::memory_order_release);

      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::request_attribute_value_update_class_with_regions,
          "The regional class-request service lost regional Request Attribute Value Update.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The regional class-request service lost provider callback polling.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional class-request service lost provider Resign.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional class-request service lost requester Resign.");
      service.detach(provider);
      service.detach(requester);
      provider.connection()->close();
      requester.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> providerConnection;
  std::shared_ptr<ProcessTransportConnection> requesterConnection;
  try {
    providerConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-regional-class-provider", 0xF3U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    requesterConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-regional-class-requester", 0xF4U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession provider(providerConnection);
    ProcessTransportSession requester(requesterConnection);
    TransportServiceMessage response;
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{
                    L"process-regional-class-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-regional-class-execution",
                    L"process-regional-class-provider",
                    L"process-regional-class-provider"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const providerJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::join_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-regional-class-execution",
                    L"process-regional-class-requester",
                    L"process-regional-class-requester"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requesterJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(providerJoin.federateId != requesterJoin.federateId);

    while (objectClassHandle.load(std::memory_order_acquire) == 0U ||
           attributeHandle.load(std::memory_order_acquire) == 0U ||
           requesterRegionHandle.load(std::memory_order_acquire) == 0U ||
           registeredObjectHandle.load(std::memory_order_acquire) == 0U) {
      std::this_thread::yield();
    }
    auto const objectClass = objectClassHandle.load(std::memory_order_acquire);
    auto const attribute = attributeHandle.load(std::memory_order_acquire);
    auto const requesterRegion =
        requesterRegionHandle.load(std::memory_order_acquire);
    auto const registeredObject =
        registeredObjectHandle.load(std::memory_order_acquire);
    std::vector<std::uint8_t> const tag{0x52U, 0x45U, 0x47U};
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::request_attribute_value_update_class_with_regions,
            3U,
            umbra::detail::encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
                ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest{
                    L"process-regional-class-execution",
                    requesterJoin.federateId,
                    objectClass,
                    {attribute},
                    std::map<std::uint64_t, std::set<std::uint64_t>>{
                        {attribute, {requesterRegion}}},
                    tag})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requestResult =
        umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
            response.payload);
    REQUIRE(requestResult.recipientCount == 1U);

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::receive_interaction,
            4U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-regional-class-execution", providerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const callback =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE(callback.attributeValueUpdateRequestEvent.has_value());
    REQUIRE(callback.attributeValueUpdateRequestEvent->requestingFederateId ==
            requesterJoin.federateId);
    REQUIRE(callback.attributeValueUpdateRequestEvent->providingFederateId ==
            providerJoin.federateId);
    REQUIRE(callback.attributeValueUpdateRequestEvent->objectInstanceHandle ==
            registeredObject);
    REQUIRE(callback.attributeValueUpdateRequestEvent->requestedAttributeHandles ==
            std::set<std::uint64_t>{attribute});
    REQUIRE(callback.attributeValueUpdateRequestEvent->userSuppliedTag == tag);

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            5U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-regional-class-execution",
                    providerJoin.federateId,
                    rti1516_2025::DELETE_OBJECTS})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            5U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-regional-class-execution",
                    requesterJoin.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    provider.connection()->close();
    requester.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (providerConnection) {
      providerConnection->close();
    }
    if (requesterConnection) {
      requesterConnection->close();
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
