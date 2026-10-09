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
