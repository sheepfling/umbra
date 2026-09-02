#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

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
using umbra::detail::ProcessFederationLogicalTime;
using umbra::detail::ProcessFederationDeleteObjectInstanceRequest;
using umbra::detail::ProcessFederationDeleteObjectInstanceResult;
using umbra::detail::ProcessFederationLocalDeleteObjectInstanceRequest;
using umbra::detail::ProcessFederationLocalDeleteObjectInstanceResult;
using umbra::detail::ProcessFederationReceiveInteractionRequest;
using umbra::detail::ProcessFederationReceiveInteractionResult;
using umbra::detail::ProcessFederationSendInteractionRequest;
using umbra::detail::ProcessFederationSendInteractionResult;
using umbra::detail::ProcessFederationReceiveAttributeUpdateResult;
using umbra::detail::ProcessFederationReceiveObjectInstanceDiscoveryResult;
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
    "Private process service binds create join and receive-order interaction to the federation registry",
    "[unit][foundation][transport][process-boundary][service-dispatch][registry-binding][transport-contract][interaction-management]") {
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

    auto const interactionClass = registry.interactionClassHandleFor(
        L"process-execution",
        "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
    REQUIRE(interactionClass.has_value());

    std::vector<std::uint8_t> const interactionPayload{0x50U, 0x52U, 0x4fU, 0x43U};
    REQUIRE(sender.request(
        request(
            TransportServiceOperation::send_interaction,
            4U,
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
            5U,
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
    "[unit][foundation][transport][process-boundary][service-dispatch][registry-binding][transport-contract][object-management][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
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
    "[unit][foundation][transport][process-boundary][object-management][delete-object-instance][transport-contract][rti.service.delete-object-instance]") {
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
