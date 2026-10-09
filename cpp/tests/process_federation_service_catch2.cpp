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
using umbra::detail::ProcessFederationLogicalTimeInterval;
using umbra::detail::ProcessFederationEnableTimeRegulationRequest;
using umbra::detail::ProcessFederationTimeEnableStatus;
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
    "Private process service allocates one filesystem service-report file and MOM identity per joined federate",
    "[integration][development-profile][federation-management][mom]"
    "[service-reporting][service-report-file][process-boundary][registry-binding][2025]") {
  static std::atomic_uint64_t sequence{0U};
  auto const reportDirectory = std::filesystem::temp_directory_path() /
      ("umbra-process-service-report-" + std::to_string(
          sequence.fetch_add(1U, std::memory_order_relaxed)));
  std::error_code cleanupError;
  std::filesystem::remove_all(reportDirectory, cleanupError);

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions options;
  options.serviceReportDirectory = reportDirectory;
  ProcessFederationService service(registry, composedRestaurantDefinition(), options);
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-report-server", 0xE1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      if (!ProcessTransportServiceDispatcher::serveOne(session, handler) ||
          !ProcessTransportServiceDispatcher::serveOne(session, handler)) {
        throw std::runtime_error(
            "The process service lost the report-file lifecycle requests.");
      }

      auto const member = registry.memberByName(
          L"process-report-execution", L"process-report-federate");
      if (!member) {
        throw std::runtime_error(
            "The process report-file test lost its joined member.");
      }
      auto const snapshot = registry.joinedFederateMomObjectFor(
          L"process-report-execution", member->id);
      auto const federationMom = registry.federationMomObjectFor(
          L"process-report-execution");
      if (!snapshot || !federationMom || snapshot->reportServiceFile.empty() ||
          !std::filesystem::exists(snapshot->reportServiceFile)) {
        throw std::runtime_error(
            "The process Join did not establish filesystem-backed MOM state.");
      }
      std::ifstream input(
          std::filesystem::path(snapshot->reportServiceFile), std::ios::binary);
      std::string initialText{
          std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
      if (initialText.find("HLAfederateName") == std::string::npos ||
          initialText.find("process-report-federate") == std::string::npos) {
        throw std::runtime_error(
            "The process report file does not begin with the joined-federate initial record.");
      }
      if (!ProcessTransportServiceDispatcher::serveOne(session, handler)) {
        throw std::runtime_error(
            "The process service lost the report-file resign request.");
      }
      if (registry.joinedFederateMomObjectFor(
              L"process-report-execution", member->id)) {
        throw std::runtime_error(
            "The process report-file test retained the MOM object after resign.");
      }
      if (!std::filesystem::exists(snapshot->reportServiceFile)) {
        throw std::runtime_error(
            "The process report file was removed when the federate resigned.");
      }
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> connection;
  std::filesystem::path reportFile;
  try {
    connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-report-client", 0xE2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession session(connection);
    TransportServiceMessage response;
    REQUIRE(session.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{L"process-report-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(session.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-report-execution",
                    L"process-report-federate",
                    L"process-report-federate"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const joinResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE_FALSE(joinResult.reportServiceFile.empty());
    reportFile = joinResult.reportServiceFile;
    REQUIRE(reportFile.is_absolute());
    REQUIRE(std::filesystem::exists(reportFile));

    REQUIRE(session.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            3U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-report-execution",
                    joinResult.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(std::filesystem::exists(reportFile));
    connection->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (connection) {
      connection->close();
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
  std::filesystem::remove_all(reportDirectory, cleanupError);
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
    "Private process service rejects invalid regional class Request Attribute Value Update selectors deterministically",
    "[unit][foundation][object-management][ddm][callbacks][transport][process-boundary][service-dispatch][registry-binding][transport-contract][invalid-regional-selector][rti.service.request-attribute-value-update-with-regions][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::atomic_uint64_t objectClassHandle{0U};
  std::atomic_uint64_t attributeHandle{0U};
  std::atomic_uint64_t providerRegionHandle{0U};
  std::atomic_uint64_t uncommittedRegionHandle{0U};
  std::atomic_uint64_t wrongContextRegionHandle{0U};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto providerConnection = listener->accept(
          nullptr,
          {"process-regional-error-provider", 0xE5U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession provider(std::move(providerConnection));
      auto requesterConnection = listener->accept(
          nullptr,
          {"process-regional-error-requester", 0xE6U},
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
          "The regional error matrix lost Create.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional error matrix lost provider Join.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional error matrix lost requester Join.");

      auto const providerMember = registry.memberByName(
          L"process-regional-error-execution",
          L"process-regional-error-provider");
      auto const requesterMember = registry.memberByName(
          L"process-regional-error-execution",
          L"process-regional-error-requester");
      auto const objectClass = registry.objectClassHandleFor(
          L"process-regional-error-execution", "HLAobjectRoot.Food.Drink.Soda");
      auto const attribute = registry.attributeHandleFor(
          L"process-regional-error-execution",
          "HLAobjectRoot.Food.Drink.Soda",
          "Flavor");
      auto const sodaDimension = registry.dimensionHandleFor(
          L"process-regional-error-execution", "SodaFlavor");
      auto const federateDimension = registry.dimensionHandleFor(
          L"process-regional-error-execution", "HLAfederate");
      if (!providerMember || !requesterMember || !objectClass || !attribute ||
          !sodaDimension || !federateDimension) {
        throw std::runtime_error(
            "The regional error matrix could not resolve its FOM handles.");
      }

      auto const providerRegion = registry.createRegion(
          L"process-regional-error-execution",
          providerMember->id,
          {*sodaDimension});
      auto const uncommittedRegion = registry.createRegion(
          L"process-regional-error-execution",
          requesterMember->id,
          {*sodaDimension});
      auto const wrongContextRegion = registry.createRegion(
          L"process-regional-error-execution",
          requesterMember->id,
          {*federateDimension});
      auto const invalidBoundsRegion = registry.createRegion(
          L"process-regional-error-execution",
          requesterMember->id,
          {*sodaDimension});
      if (providerRegion.status != umbra::detail::RegionServiceStatus::applied ||
          uncommittedRegion.status !=
              umbra::detail::RegionServiceStatus::applied ||
          wrongContextRegion.status !=
              umbra::detail::RegionServiceStatus::applied ||
          invalidBoundsRegion.status !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.setRangeBounds(
              L"process-regional-error-execution",
              providerMember->id,
              providerRegion.regionHandle,
              *sodaDimension,
              umbra::detail::RegionRangeBounds{0UL, 2UL}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.commitRegionModifications(
              L"process-regional-error-execution",
              providerMember->id,
              {providerRegion.regionHandle}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.setRangeBounds(
              L"process-regional-error-execution",
              requesterMember->id,
              uncommittedRegion.regionHandle,
              *sodaDimension,
              umbra::detail::RegionRangeBounds{0UL, 2UL}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.setRangeBounds(
              L"process-regional-error-execution",
              requesterMember->id,
              wrongContextRegion.regionHandle,
              *federateDimension,
              umbra::detail::RegionRangeBounds{0UL, 2UL}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.commitRegionModifications(
              L"process-regional-error-execution",
              requesterMember->id,
              {wrongContextRegion.regionHandle}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.setRangeBounds(
              L"process-regional-error-execution",
              requesterMember->id,
              invalidBoundsRegion.regionHandle,
              *sodaDimension,
              umbra::detail::RegionRangeBounds{4UL, 3UL}) !=
              umbra::detail::RegionServiceStatus::invalid_range_bound) {
        throw std::runtime_error(
            "The regional error matrix could not establish its region cases.");
      }

      objectClassHandle.store(*objectClass, std::memory_order_release);
      attributeHandle.store(*attribute, std::memory_order_release);
      providerRegionHandle.store(
          providerRegion.regionHandle, std::memory_order_release);
      uncommittedRegionHandle.store(
          uncommittedRegion.regionHandle, std::memory_order_release);
      wrongContextRegionHandle.store(
          wrongContextRegion.regionHandle, std::memory_order_release);

      for (int i = 0; i < 4; ++i) {
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::request_attribute_value_update_class_with_regions,
            "The regional error matrix lost a rejected regional request.");
      }
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional error matrix lost provider Resign.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional error matrix lost requester Resign.");
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
        {"process-regional-error-provider", 0xF5U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    requesterConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-regional-error-requester", 0xF6U},
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
                    L"process-regional-error-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-regional-error-execution",
                    L"process-regional-error-provider",
                    L"process-regional-error-provider"})),
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
                    L"process-regional-error-execution",
                    L"process-regional-error-requester",
                    L"process-regional-error-requester"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const requesterJoin =
        umbra::detail::decodeProcessFederationJoinResult(response.payload);
    REQUIRE(providerJoin.federateId != requesterJoin.federateId);

    while (objectClassHandle.load(std::memory_order_acquire) == 0U ||
           attributeHandle.load(std::memory_order_acquire) == 0U ||
           providerRegionHandle.load(std::memory_order_acquire) == 0U ||
           uncommittedRegionHandle.load(std::memory_order_acquire) == 0U ||
           wrongContextRegionHandle.load(std::memory_order_acquire) == 0U) {
      std::this_thread::yield();
    }
    auto const objectClass = objectClassHandle.load(std::memory_order_acquire);
    auto const attribute = attributeHandle.load(std::memory_order_acquire);
    auto const providerRegion =
        providerRegionHandle.load(std::memory_order_acquire);
    auto const uncommittedRegion =
        uncommittedRegionHandle.load(std::memory_order_acquire);
    auto const wrongContextRegion =
        wrongContextRegionHandle.load(std::memory_order_acquire);
    auto sendRejected = [&](std::uint64_t requestId,
                            std::uint64_t regionHandle,
                            std::vector<std::uint8_t> tag) {
      REQUIRE(requester.request(
          request(
              TransportServiceOperation::request_attribute_value_update_class_with_regions,
              requestId,
              umbra::detail::encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
                  ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest{
                      L"process-regional-error-execution",
                      requesterJoin.federateId,
                      objectClass,
                      {attribute},
                      std::map<std::uint64_t, std::set<std::uint64_t>>{
                          {attribute, {regionHandle}}},
                      std::move(tag)})),
          response));
      REQUIRE(response.status == TransportServiceStatus::rejected);
    };
    sendRejected(3U, std::uint64_t{0xFFFFFFFFFFFFFFFFULL}, {0x55U});
    sendRejected(4U, providerRegion, {0x46U});
    sendRejected(5U, uncommittedRegion, {0x55U, 0x4EU});
    sendRejected(6U, wrongContextRegion, {0x43U});

    REQUIRE(provider.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            7U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-regional-error-execution",
                    providerJoin.federateId,
                    rti1516_2025::DELETE_OBJECTS})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            7U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-regional-error-execution",
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
    "Private process Exception Reporting Switch preserves save restore gates and session identity",
    "[unit][support-services][mom][save-restore][transport][process-boundary]"
    "[process-exception-reporting-switch-gates][registry-binding][2025]") {
  using umbra::detail::FederationServiceOperationStatus;
  using umbra::detail::ProcessFederationClient;
  using umbra::detail::ProcessFederationClientError;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr, {"exception-switch-gates-server", 0xF7U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      while (ProcessTransportServiceDispatcher::serveOne(session, handler)) {}
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });
  std::exception_ptr clientFailure;
  try {
    ProcessFederationClient client(
        {"127.0.0.1", listener->address().port},
        {"exception-switch-gates-client", 0xF8U});
    std::wstring const federation = L"exception-switch-gates";
    client.createFederationExecution(federation);
    auto const joined = client.joinFederationExecution(
        federation, L"tester", L"exception-switch-gates-member");
    auto const federate = joined.federateId;
    auto initial = client.getExceptionReportingSwitch(federation, federate);
    REQUIRE(initial.status == FederationServiceOperationStatus::available);
    REQUIRE_FALSE(initial.value);
    REQUIRE(client.setExceptionReportingSwitch(federation, federate, true).status ==
            FederationServiceOperationStatus::available);
    REQUIRE(client.getExceptionReportingSwitch(federation, federate).value);
    REQUIRE_THROWS_AS(
        client.setExceptionReportingSwitch(federation, federate + 1U, false),
        ProcessFederationClientError);
    REQUIRE_THROWS_AS(
        client.getExceptionReportingSwitch(L"other-federation", federate),
        ProcessFederationClientError);
    REQUIRE(registry.exceptionReportingSwitchFor(federation, federate) == true);

    REQUIRE(registry.requestFederationSave(federation, federate, L"checkpoint").status ==
            umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(client.getExceptionReportingSwitch(federation, federate).status ==
            FederationServiceOperationStatus::save_in_progress);
    REQUIRE(client.setExceptionReportingSwitch(federation, federate, false).status ==
            FederationServiceOperationStatus::save_in_progress);
    REQUIRE(registry.exceptionReportingSwitchFor(federation, federate) == true);
    REQUIRE(registry.federateSaveBegun(federation, federate).status ==
            umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveComplete(federation, federate).saveCompletedSuccessfully);
    REQUIRE(client.getExceptionReportingSwitch(federation, federate).status ==
            FederationServiceOperationStatus::available);

    REQUIRE(registry.requestFederationRestore(federation, federate, L"checkpoint").status ==
            umbra::detail::FederationRestoreControlStatus::applied);
    REQUIRE(client.getExceptionReportingSwitch(federation, federate).status ==
            FederationServiceOperationStatus::restore_in_progress);
    REQUIRE(client.setExceptionReportingSwitch(federation, federate, false).status ==
            FederationServiceOperationStatus::restore_in_progress);
    REQUIRE(registry.exceptionReportingSwitchFor(federation, federate) == true);
    REQUIRE(registry.federateRestoreComplete(federation, federate).status ==
            umbra::detail::FederationRestoreControlStatus::applied);
    REQUIRE(client.setExceptionReportingSwitch(federation, federate, false).status ==
            FederationServiceOperationStatus::available);
    REQUIRE_FALSE(client.getExceptionReportingSwitch(federation, federate).value);
    REQUIRE(client.getExceptionReportingSwitch(federation, federate).status ==
            FederationServiceOperationStatus::available);
    client.resignFederationExecution(federation, federate, rti1516_2025::NO_ACTION);
    REQUIRE_THROWS_AS(client.getExceptionReportingSwitch(federation, federate),
                      ProcessFederationClientError);
    REQUIRE_THROWS_AS(client.setExceptionReportingSwitch(federation, federate, true),
                      ProcessFederationClientError);
  } catch (...) {
    clientFailure = std::current_exception();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
}

TEST_CASE(
    "Private ProcessFederationClient close releases a blocked exception-report recheck before draining projection leases",
    "[unit][internal][callbacks][process-boundary][process-client-close-exception-recheck]") {
  using umbra::detail::ProcessFederationClient;
  using umbra::detail::ProcessFederationInteractionEvent;
  using umbra::detail::TransportServiceOperation;

  class CountingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const&,
        rti1516_2025::ParameterHandleValueMap const&,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*) override {
      ++callbacks;
    }

    std::size_t callbacks = 0U;
  } recipient;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, composedRestaurantDefinition());
  std::promise<void> recheckStartedPromise;
  auto recheckStarted = recheckStartedPromise.get_future();
  std::promise<void> releaseRecheckPromise;
  auto releaseRecheck = releaseRecheckPromise.get_future().share();
  std::atomic_bool releaseRequested{false};
  std::atomic_bool closeRequested{false};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"blocked-recheck-server", 0xE7U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto serviceHandler = service.handlerFor(session);
      ProcessTransportServiceDispatcher::Handler handler =
          [&](TransportServiceMessage const& request) {
            if (request.operation ==
                TransportServiceOperation::recheck_exception_report) {
              recheckStartedPromise.set_value();
              releaseRecheck.wait();
            }
            return serviceHandler(request);
          };
      while (ProcessTransportServiceDispatcher::serveOne(session, handler)) {}
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      if (!closeRequested.load(std::memory_order_acquire)) {
        serverFailure = std::current_exception();
      }
    }
  });

  auto releaseServer = [&] {
    if (!releaseRequested.exchange(true, std::memory_order_acq_rel)) {
      releaseRecheckPromise.set_value();
    }
  };
  std::unique_ptr<ProcessFederationClient> client;
  std::future<bool> evoker;
  std::future<void> closer;
  std::exception_ptr clientFailure;
  bool recheckWasBlocked = false;
  bool closeFinishedBeforeResponseRelease = false;
  bool closeFinishedAfterCleanup = false;
  bool evokerFinished = false;
  try {
    client = std::make_unique<ProcessFederationClient>(
        umbra::detail::ProcessTransportAddress{
            "127.0.0.1", listener->address().port},
        umbra::detail::TransportEndpointIdentity{
            "blocked-recheck-client", 0xE8U});
    std::wstring const federationName = L"blocked-recheck-execution";
    client->createFederationExecution(federationName);
    auto const joined = client->joinFederationExecution(
        federationName, L"tester", L"blocked-recheck-member");
    client->attachCallbackBridge(
        recipient, umbra::detail::CallbackDispatchModel::evoked);

    ProcessFederationInteractionEvent event;
    event.receivingFederateId = joined.federateId;
    event.interactionClassHandle = 0xE9U;
    event.parameterHandles = {0xEAU};
    event.payload = {0xEBU};
    event.transportationName = "HLAreliable";
    event.rtiOwnedMomInteraction = true;
    event.exceptionReportFederateId = joined.federateId;
    client->dispatchReceiveOrder(std::move(event));
    evoker = std::async(std::launch::async, [&] {
      return client->evokeOne(std::chrono::milliseconds{0});
    });

    recheckWasBlocked =
        recheckStarted.wait_for(std::chrono::seconds{3}) ==
        std::future_status::ready;
    if (recheckWasBlocked) {
      closeRequested.store(true, std::memory_order_release);
      closer = std::async(std::launch::async, [&] { client->close(); });
      auto const closeStatus = closer.wait_for(std::chrono::seconds{2});
      closeFinishedBeforeResponseRelease =
          closeStatus == std::future_status::ready &&
          !releaseRequested.load(std::memory_order_acquire);
      releaseServer();
      closeFinishedAfterCleanup =
          closeStatus == std::future_status::ready ||
          closer.wait_for(std::chrono::seconds{5}) ==
              std::future_status::ready;
      if (closeFinishedAfterCleanup) {
        closer.get();
      }
      evokerFinished =
          evoker.wait_for(std::chrono::seconds{5}) ==
          std::future_status::ready;
      if (evokerFinished) {
        (void)evoker.get();
      }
    } else {
      releaseServer();
      closeRequested.store(true, std::memory_order_release);
      client->close();
      evokerFinished =
          evoker.wait_for(std::chrono::seconds{5}) ==
          std::future_status::ready;
      if (evokerFinished) {
        (void)evoker.get();
      }
    }
  } catch (...) {
    clientFailure = std::current_exception();
    releaseServer();
    closeRequested.store(true, std::memory_order_release);
    if (client) {
      client->close();
    }
    if (evoker.valid() &&
        evoker.wait_for(std::chrono::seconds{5}) == std::future_status::ready) {
      try {
        (void)evoker.get();
      } catch (...) {
      }
    }
    if (closer.valid() &&
        closer.wait_for(std::chrono::seconds{5}) == std::future_status::ready) {
      closer.get();
    }
  }
  releaseServer();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }

  REQUIRE(recheckWasBlocked);
  REQUIRE(closeFinishedBeforeResponseRelease);
  REQUIRE(closeFinishedAfterCleanup);
  REQUIRE(evokerFinished);
  REQUIRE(recipient.callbacks == 0U);
  REQUIRE(client->connection() == nullptr);
  REQUIRE_FALSE(serverFailure);
}
