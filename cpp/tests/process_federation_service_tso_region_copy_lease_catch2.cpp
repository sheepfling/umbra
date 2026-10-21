#include <catch2/catch_test_macros.hpp>

#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/transport_protocol.hpp"
#include "process_federation_service_test_support.hpp"

#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace umbra::detail;

[[nodiscard]] FederationDefinition regionalProcessDefinition() {
  using namespace umbra::test::process_fom_support_2025;
  auto const regionalFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" /
      "two-dimensional-regional-interaction-fom.xml";
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-tso-lease-mim"),
      validatedProcessModule(
          regionalFom,
          FomModuleKind::fom,
          L"urn:umbra:test:process-tso-lease-regional-object"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error(
        "The process TSO lease test FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

[[nodiscard]] std::vector<std::uint8_t> encodedBytes(
    rti1516_2025::VariableLengthData const& encoded) {
  if (encoded.size() == 0U) {
    return {};
  }
  auto const* data = static_cast<std::uint8_t const*>(encoded.data());
  if (data == nullptr) {
    throw std::runtime_error("The test logical-time value has no encoding.");
  }
  return {data, data + encoded.size()};
}

TEST_CASE(
    "Private process TSO callback passel projection retains send-time region copies until lease retirement",
    "[unit][internal][process-boundary][time-management][ddm][tso]"
    "[ddm-process-tso-region-callback-snapshot][passel-event-projection]"
    "[callback-region-copy-lease]"
    "[no-normative-mapping]") {
  constexpr wchar_t const* federationName = L"process-tso-region-copy-lease";
  constexpr std::uint64_t sourceRegionHandle = 41U;
  constexpr std::uint64_t secondSourceRegionHandle = 42U;
  constexpr std::uint64_t dimensionHandle = 7U;
  constexpr std::uint64_t attributeHandle = 13U;
  constexpr std::uint64_t messageId = 99U;

  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(
              federationName,
              umbra::test::process_fom_support_2025::composedProcessDefinition())
              .status == FederationRegistryStatus::applied);
  auto const joined = registry.join(federationName, L"lease-test-federate");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());

  RegionSpecificationSnapshot sentSnapshot;
  sentSnapshot.dimensionHandles.insert(dimensionHandle);
  sentSnapshot.committedRangeBounds.emplace(
      dimensionHandle, RegionRangeBounds{1UL, 3UL});
  sentSnapshot.specificationCommitted = true;

  std::uint8_t const valueBytes[]{0x31U, 0x42U};
  std::uint8_t const tagBytes[]{0x53U};
  TsoAttributeUpdateMessage message;
  message.messageId = messageId;
  message.producingFederateId = 17U;
  message.objectInstanceHandle = 23U;
  message.attributes.emplace_back(
      attributeHandle,
      rti1516_2025::VariableLengthData(valueBytes, sizeof(valueBytes)));
  message.userSuppliedTag =
      rti1516_2025::VariableLengthData(tagBytes, sizeof(tagBytes));
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time const>(5);

  TsoAttributeUpdatePassel firstPassel;
  firstPassel.transportationName = "HLAreliable";
  firstPassel.sentAttributeHandles.push_back(attributeHandle);
  firstPassel.sentRegionHandles.insert(sourceRegionHandle);
  firstPassel.sentRegionSnapshots.emplace(sourceRegionHandle, sentSnapshot);
  TsoAttributeUpdatePassel secondPassel;
  secondPassel.transportationName = "HLAreliable";
  secondPassel.sentAttributeHandles.push_back(attributeHandle);
  secondPassel.sentRegionHandles.insert(secondSourceRegionHandle);
  secondPassel.sentRegionSnapshots.emplace(
      secondSourceRegionHandle, sentSnapshot);

  ReceiveOrderAttributeUpdateRecipient projection;
  projection.federateId = joined.membership->id;
  projection.receivedAttributeHandles.insert(attributeHandle);
  projection.conveyRegionDesignatorSets = true;

  std::map<std::uint64_t, ProcessTsoCallbackRegionDesignatorCopyLease>
      pendingLeases;
  {
    ProcessTsoCallbackRegionDesignatorCopyLease lease(
        registry, federationName, joined.membership->id);
    auto firstEvent = projectProcessTsoAttributeUpdatePasselToCallback(
        registry,
        federationName,
        joined.membership->id,
        message,
        firstPassel,
        projection,
        lease);
    REQUIRE(firstEvent.has_value());
    REQUIRE(firstEvent->sentRegionHandles.has_value());
    REQUIRE(firstEvent->sentRegionHandles->size() == 1U);
    CHECK(firstEvent->attributeValues.size() == 1U);
    CHECK(firstEvent->attributeValues.front().first == attributeHandle);
    CHECK(firstEvent->attributeValues.front().second ==
          std::vector<std::uint8_t>{0x31U, 0x42U});
    CHECK(firstEvent->userSuppliedTag == std::vector<std::uint8_t>{0x53U});
    CHECK(firstEvent->retractionMessageId == messageId);
    CHECK(firstEvent->timestamp.has_value());

    auto secondEvent = projectProcessTsoAttributeUpdatePasselToCallback(
        registry,
        federationName,
        joined.membership->id,
        message,
        secondPassel,
        projection,
        lease);
    REQUIRE(secondEvent.has_value());
    REQUIRE(secondEvent->sentRegionHandles.has_value());
    REQUIRE(secondEvent->sentRegionHandles->size() == 1U);

    auto const [pending, inserted] = pendingLeases.try_emplace(
        messageId, std::move(lease));
    static_cast<void>(pending);
    REQUIRE(inserted);

    auto const firstCopiedRegionHandle =
        *firstEvent->sentRegionHandles->begin();
    auto const secondCopiedRegionHandle =
        *secondEvent->sentRegionHandles->begin();
    auto const firstBounds = registry.rangeBoundsForRegion(
        federationName,
        joined.membership->id,
        firstCopiedRegionHandle,
        dimensionHandle);
    REQUIRE(firstBounds.status == RegionServiceStatus::applied);
    CHECK(firstBounds.range.lowerBound == 1UL);
    CHECK(firstBounds.range.upperBound == 3UL);
    auto const secondBounds = registry.rangeBoundsForRegion(
        federationName,
        joined.membership->id,
        secondCopiedRegionHandle,
        dimensionHandle);
    REQUIRE(secondBounds.status == RegionServiceStatus::applied);
    CHECK(secondBounds.range.lowerBound == 1UL);
    CHECK(secondBounds.range.upperBound == 3UL);

    CHECK(pendingLeases.erase(messageId) == 1U);
    CHECK(registry.rangeBoundsForRegion(
              federationName,
              joined.membership->id,
              firstCopiedRegionHandle,
              dimensionHandle)
              .status == RegionServiceStatus::invalid_region);
    CHECK(registry.rangeBoundsForRegion(
              federationName,
              joined.membership->id,
              secondCopiedRegionHandle,
              dimensionHandle)
              .status == RegionServiceStatus::invalid_region);
  }
}

TEST_CASE(
    "Process handlerFor TSO dispatch retains regional callback copies until acknowledge",
    "[unit][internal][process-boundary][time-management][ddm][tso]"
    "[ddm-process-tso-region-callback-snapshot]"
    "[dispatch-ack-send-only-transport-test-seam]"
    "[no-normative-mapping]") {
  constexpr wchar_t const* federationName = L"process-handler-tso-region-lease";
  EmbeddedFederationRegistry registry;
  ProcessFederationService service(registry, regionalProcessDefinition());

  std::vector<TransportServiceMessage> senderFrames;
  std::vector<TransportServiceMessage> receiverFrames;
  auto collectFrames = [](std::vector<TransportServiceMessage>& messages) {
    return [&messages](TransportFrame const& frame) {
      if (frame.kind != TransportFrameKind::data) {
        return false;
      }
      messages.push_back(decodeTransportServiceMessage(frame.payload));
      return true;
    };
  };
  ProcessTransportSession sender(
      ProcessTransportConnection::createForTesting(
          collectFrames(senderFrames)));
  ProcessTransportSession receiver(
      ProcessTransportConnection::createForTesting(
          collectFrames(receiverFrames)));
  auto const senderHandler = service.handlerFor(sender);
  auto const receiverHandler = service.handlerFor(receiver);

  auto invoke = [](
                   ProcessTransportServiceDispatcher::Handler const& handler,
                   TransportServiceOperation operation,
                   std::uint64_t requestId,
                   std::vector<std::uint8_t> payload) {
    auto const response = handler(TransportServiceMessage{
        TransportServiceMessageKind::request,
        operation,
        TransportServiceStatus::ok,
        requestId,
        std::move(payload)});
    REQUIRE(response.kind == TransportServiceMessageKind::response);
    REQUIRE(response.operation == operation);
    REQUIRE(response.requestId == requestId);
    REQUIRE(response.status == TransportServiceStatus::ok);
    return response;
  };

  std::uint64_t senderRequestId = 1U;
  std::uint64_t receiverRequestId = 1U;
  auto senderCall = [&](TransportServiceOperation operation,
                        std::vector<std::uint8_t> payload) {
    return invoke(senderHandler, operation, senderRequestId++, std::move(payload));
  };
  auto receiverCall = [&](TransportServiceOperation operation,
                          std::vector<std::uint8_t> payload) {
    return invoke(
        receiverHandler, operation, receiverRequestId++, std::move(payload));
  };

  REQUIRE(senderCall(
              TransportServiceOperation::create_federation_execution,
              encodeProcessFederationCreateRequest(
                  ProcessFederationCreateRequest{federationName}))
              .status == TransportServiceStatus::ok);
  auto const senderJoined = decodeProcessFederationJoinResult(
      senderCall(
          TransportServiceOperation::join_federation_execution,
          encodeProcessFederationJoinRequest(ProcessFederationJoinRequest{
              federationName, L"process-tso-publisher", L"publisher"}))
          .payload);
  auto const receiverJoined = decodeProcessFederationJoinResult(
      receiverCall(
          TransportServiceOperation::join_federation_execution,
          encodeProcessFederationJoinRequest(ProcessFederationJoinRequest{
              federationName, L"process-tso-receiver", L"receiver"}))
          .payload);
  REQUIRE(senderJoined.federateId != 0U);
  REQUIRE(receiverJoined.federateId != 0U);

  auto const objectClass = registry.objectClassHandleFor(
      federationName, "HLAobjectRoot.UmbraTwoDimensionalRegionObject");
  auto const attribute = registry.attributeHandleFor(
      federationName, "HLAobjectRoot.UmbraTwoDimensionalRegionObject", "Value");
  auto const dimensionX = registry.dimensionHandleFor(
      federationName, "UmbraRegionX");
  auto const dimensionY = registry.dimensionHandleFor(
      federationName, "UmbraRegionY");
  REQUIRE(objectClass.has_value());
  REQUIRE(attribute.has_value());
  REQUIRE(dimensionX.has_value());
  REQUIRE(dimensionY.has_value());

  REQUIRE(senderCall(
              TransportServiceOperation::publish_object_class_attributes,
              encodeProcessFederationObjectClassAttributeDeclarationRequest(
                  ProcessFederationObjectClassAttributeDeclarationRequest{
                      federationName,
                      senderJoined.federateId,
                      *objectClass,
                      {*attribute}}))
              .status == TransportServiceStatus::ok);
  REQUIRE(senderCall(
              TransportServiceOperation::change_default_attribute_order_type,
              encodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
                  ProcessFederationChangeDefaultAttributeOrderTypeRequest{
                      federationName,
                      senderJoined.federateId,
                      *objectClass,
                      {*attribute},
                      rti1516_2025::TIMESTAMP}))
              .status == TransportServiceStatus::ok);

  auto createRegion = [&](ProcessTransportServiceDispatcher::Handler const& handler,
                          std::uint64_t& requestId,
                          std::uint64_t federateId) {
    auto const result = invoke(
        handler,
        TransportServiceOperation::create_region,
        requestId++,
        encodeProcessFederationCreateRegionRequest(
            ProcessFederationCreateRegionRequest{
                federationName, federateId, {*dimensionX, *dimensionY}}));
    auto const decoded = decodeProcessFederationCreateRegionResult(result.payload);
    REQUIRE(decoded.status == RegionServiceStatus::applied);
    REQUIRE(decoded.regionHandle != 0U);
    return decoded.regionHandle;
  };
  auto const sourceRegion =
      createRegion(senderHandler, senderRequestId, senderJoined.federateId);
  auto const subscriptionRegion = createRegion(
      receiverHandler, receiverRequestId, receiverJoined.federateId);

  auto setRegionBounds = [&](ProcessTransportServiceDispatcher::Handler const& handler,
                             std::uint64_t& requestId,
                             std::uint64_t federateId,
                             std::uint64_t regionHandle,
                             unsigned long lower,
                             unsigned long upper) {
    for (auto const dimension : {*dimensionX, *dimensionY}) {
      auto const result = invoke(
          handler,
          TransportServiceOperation::set_range_bounds,
          requestId++,
          encodeProcessFederationSetRangeBoundsRequest(
              ProcessFederationSetRangeBoundsRequest{
                  federationName,
                  federateId,
                  regionHandle,
                  dimension,
                  lower,
                  upper}));
      CHECK(decodeProcessFederationRegionStatusResult(result.payload).status ==
            RegionServiceStatus::applied);
    }
    auto const committed = invoke(
        handler,
        TransportServiceOperation::commit_region_modifications,
        requestId++,
        encodeProcessFederationRegionSetRequest(
            ProcessFederationRegionSetRequest{
                federationName, federateId, {regionHandle}}));
    CHECK(decodeProcessFederationRegionStatusResult(committed.payload).status ==
          RegionServiceStatus::applied);
  };
  setRegionBounds(
      senderHandler,
      senderRequestId,
      senderJoined.federateId,
      sourceRegion,
      1UL,
      3UL);
  setRegionBounds(
      receiverHandler,
      receiverRequestId,
      receiverJoined.federateId,
      subscriptionRegion,
      1UL,
      3UL);

  REQUIRE(receiverCall(
              TransportServiceOperation::subscribe_object_class_attributes_with_regions,
              encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
                  ProcessFederationObjectClassAttributeRegionalSubscriptionRequest{
                      federationName,
                      receiverJoined.federateId,
                      *objectClass,
                      {{*attribute, {subscriptionRegion}}},
                      true,
                      "HLAdefault"}))
              .status == TransportServiceStatus::ok);
  REQUIRE(receiverCall(
              TransportServiceOperation::set_convey_region_designator_sets_switch,
              encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
                  ProcessFederationAttributeScopeAdvisorySwitchRequest{
                      federationName, receiverJoined.federateId, true}))
              .status == TransportServiceStatus::ok);
  auto const registration = decodeProcessFederationRegisterObjectInstanceResult(
      senderCall(
          TransportServiceOperation::register_object_instance_with_regions,
          encodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
              ProcessFederationRegisterObjectInstanceWithRegionsRequest{
                  federationName,
                  senderJoined.federateId,
                  *objectClass,
                  {{*attribute, {sourceRegion}}},
                  std::nullopt}))
          .payload);
  REQUIRE(registration.objectInstanceHandle != 0U);

  rti1516_2025::HLAinteger64Interval lookahead(0);
  REQUIRE(senderCall(
              TransportServiceOperation::enable_time_regulation,
              encodeProcessFederationEnableTimeRegulationRequest(
                  ProcessFederationEnableTimeRegulationRequest{
                      federationName,
                      senderJoined.federateId,
                      ProcessFederationLogicalTimeInterval{
                          L"HLAinteger64Time", encodedBytes(lookahead.encode())}}))
              .status == TransportServiceStatus::ok);
  auto const constrainedEnable = receiverCall(
      TransportServiceOperation::enable_time_constrained,
      encodeProcessFederationEnableTimeConstrainedRequest(
          ProcessFederationEnableTimeConstrainedRequest{
              federationName, receiverJoined.federateId}));
  auto const constrainedEnableResult =
      decodeProcessFederationEnableTimeConstrainedResult(
          constrainedEnable.payload);
  INFO("receiver constrained-enable status="
       << static_cast<int>(constrainedEnableResult.status)
       << " awaiting GALT=" << constrainedEnableResult.awaitingGalt);

  auto const encodedOne = encodedBytes(rti1516_2025::HLAinteger64Time(1).encode());
  auto const warmupAdvance = senderCall(
      TransportServiceOperation::time_advance_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              federationName,
              senderJoined.federateId,
              ProcessFederationLogicalTime{L"HLAinteger64Time", encodedOne}}));
  auto const warmupAdvanceResult =
      decodeProcessFederationTimeAdvanceResult(warmupAdvance.payload);
  REQUIRE(warmupAdvanceResult.status ==
          ProcessFederationTimeAdvanceStatus::applied);
  auto const constrainedEnableEvent = std::find_if(
      receiverFrames.begin(),
      receiverFrames.end(),
      [](TransportServiceMessage const& message) {
        return message.kind == TransportServiceMessageKind::event &&
            message.operation == TransportServiceOperation::time_advance_grant;
      });
  REQUIRE(constrainedEnableEvent != receiverFrames.end());
  CHECK(decodeProcessFederationTimeAdvanceResult(
            constrainedEnableEvent->payload)
            .constrainedEnabled);

  auto const encodedFive = encodedBytes(rti1516_2025::HLAinteger64Time(5).encode());
  auto const logicalFive = ProcessFederationLogicalTime{
      L"HLAinteger64Time", encodedFive};
  auto const receiverAdvance = receiverCall(
      TransportServiceOperation::time_advance_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              federationName, receiverJoined.federateId, logicalFive}));
  auto const receiverAdvanceResult =
      decodeProcessFederationTimeAdvanceResult(receiverAdvance.payload);
  REQUIRE(receiverAdvanceResult.status ==
          ProcessFederationTimeAdvanceStatus::applied);
  INFO("receiver TAR status=" << static_cast<int>(receiverAdvanceResult.status));

  std::vector<std::uint8_t> const value{0x31U, 0x42U};
  auto const updateResult = decodeProcessFederationUpdateAttributeValuesResult(
      senderCall(
          TransportServiceOperation::update_attribute_values,
          encodeProcessFederationUpdateAttributeValuesRequest(
              ProcessFederationUpdateAttributeValuesRequest{
                  federationName,
                  senderJoined.federateId,
                  registration.objectInstanceHandle,
                  {{*attribute, value}},
                  {},
                  logicalFive}))
          .payload);
  REQUIRE(updateResult.messageId != 0U);
  REQUIRE(updateResult.recipientCount == 1U);

  // Mutate the publisher's source region after enqueue. The callback copy
  // must still carry the committed send-time bounds until its ACK retires it.
  setRegionBounds(
      senderHandler,
      senderRequestId,
      senderJoined.federateId,
      sourceRegion,
      4UL,
      6UL);
  auto const senderAdvance = senderCall(
      TransportServiceOperation::time_advance_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              federationName, senderJoined.federateId, logicalFive}));
  auto const senderAdvanceResult =
      decodeProcessFederationTimeAdvanceResult(senderAdvance.payload);
  REQUIRE(senderAdvanceResult.status ==
          ProcessFederationTimeAdvanceStatus::applied);
  std::string receiverFrameOperations;
  for (auto const& frame : receiverFrames) {
    receiverFrameOperations +=
        std::to_string(static_cast<unsigned int>(frame.operation)) + ",";
  }
  INFO("sender TAR status=" << static_cast<int>(senderAdvanceResult.status)
       << " receiver frame operation ids=" << receiverFrameOperations);

  auto const eventMessage = std::find_if(
      receiverFrames.begin(),
      receiverFrames.end(),
      [](TransportServiceMessage const& message) {
        return message.kind == TransportServiceMessageKind::event &&
            message.operation ==
                TransportServiceOperation::receive_attribute_update;
      });
  REQUIRE(eventMessage != receiverFrames.end());
  auto const reflected = decodeProcessFederationReceiveAttributeUpdateResult(
      eventMessage->payload);
  REQUIRE(reflected.event.has_value());
  REQUIRE(reflected.event->retractionMessageId == updateResult.messageId);
  REQUIRE(reflected.event->sentRegionHandles.has_value());
  REQUIRE(reflected.event->sentRegionHandles->size() == 1U);
  auto const copiedRegion = *reflected.event->sentRegionHandles->begin();
  auto const copiedBounds = registry.rangeBoundsForRegion(
      federationName,
      receiverJoined.federateId,
      copiedRegion,
      *dimensionX);
  REQUIRE(copiedBounds.status == RegionServiceStatus::applied);
  CHECK(copiedBounds.range.lowerBound == 1UL);
  CHECK(copiedBounds.range.upperBound == 3UL);

  auto const acknowledgement = invoke(
      receiverHandler,
      TransportServiceOperation::acknowledge_tso_delivery,
      receiverRequestId++,
      encodeProcessFederationAcknowledgeTsoDeliveryRequest(
          ProcessFederationAcknowledgeTsoDeliveryRequest{
              federationName,
              receiverJoined.federateId,
              updateResult.messageId}));
  CHECK(decodeProcessFederationTsoDeliveryAcknowledgementResult(
            acknowledgement.payload)
            .status == ProcessFederationTsoDeliveryAcknowledgementStatus::applied);
  CHECK(registry.rangeBoundsForRegion(
            federationName,
            receiverJoined.federateId,
            copiedRegion,
            *dimensionX)
            .status == RegionServiceStatus::invalid_region);

  service.detach(sender);
  service.detach(receiver);
}

}  // namespace
