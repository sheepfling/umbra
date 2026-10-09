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
    "Private process service preserves regions for regional class Request Attribute Value Update",
    "[unit][foundation][object-management][ddm][callbacks][transport][process-boundary][service-dispatch][registry-binding][transport-contract][disjoint-regional-selector][rti.service.request-attribute-value-update-with-regions][rti.service.register-object-instance-with-regions][federate.callback.provide-attribute-value-update][2025]") {
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
      if (registry.setRangeBounds(
              L"process-regional-class-execution",
              requesterMember->id,
              requesterRegion.regionHandle,
              *dimension,
              umbra::detail::RegionRangeBounds{3UL, 4UL}) !=
              umbra::detail::RegionServiceStatus::applied ||
          registry.commitRegionModifications(
              L"process-regional-class-execution",
              requesterMember->id,
              {requesterRegion.regionHandle}) !=
              umbra::detail::RegionServiceStatus::applied) {
        throw std::runtime_error(
            "The regional class-request service rejected its disjoint requester region.");
      }
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::request_attribute_value_update_class_with_regions,
          "The regional class-request service lost its disjoint regional request.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::receive_interaction,
          "The regional class-request service lost disjoint callback polling.");
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

    std::vector<std::uint8_t> const disjointTag{0x44U, 0x49U, 0x53U};
    REQUIRE(requester.request(
        request(
            TransportServiceOperation::request_attribute_value_update_class_with_regions,
            4U,
            umbra::detail::encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
                ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest{
                    L"process-regional-class-execution",
                    requesterJoin.federateId,
                    objectClass,
                    {attribute},
                    std::map<std::uint64_t, std::set<std::uint64_t>>{
                        {attribute, {requesterRegion}}},
                    disjointTag})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const disjointResult =
        umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
            response.payload);
    REQUIRE(disjointResult.recipientCount == 0U);
    REQUIRE(provider.request(
        request(
            TransportServiceOperation::receive_interaction,
            5U,
            umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                ProcessFederationReceiveInteractionRequest{
                    L"process-regional-class-execution", providerJoin.federateId})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const disjointCallback =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            response.payload);
    REQUIRE_FALSE(disjointCallback.attributeValueUpdateRequestEvent.has_value());

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
