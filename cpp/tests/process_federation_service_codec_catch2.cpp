#include <catch2/catch_test_macros.hpp>

#include "internal/federation/process_federation_service_protocol.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using umbra::detail::ProcessFederationDeleteObjectInstanceRequest;
using umbra::detail::ProcessFederationDeleteObjectInstanceResult;
using umbra::detail::ProcessFederationLocalDeleteObjectInstanceRequest;
using umbra::detail::ProcessFederationLocalDeleteObjectInstanceResult;
using umbra::detail::ProcessFederationLogicalTime;

}  // namespace

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
    "Private process ownership codecs round-trip complete requests and typed results",
    "[unit][internal][transport][process-boundary][attribute-ownership-codec][2025]") {
  using namespace umbra::detail;

  auto roundTrip = []<typename Value, typename Encoder, typename Decoder, typename Equal>(
                       Value const& expected,
                       Encoder encoder,
                       Decoder decoder,
                       Equal equal) {
    auto const encoded = encoder(expected);
    auto const decoded = decoder(encoded);
    CHECK(equal(expected, decoded));
  };

  std::vector<std::uint64_t> const attributeHandles{0x301U, 0x302U};
  std::vector<std::uint8_t> const tag{0x41U, 0x00U, 0x7fU};
  std::wstring const federationName = L"ownership-codec-execution";

  ProcessFederationAttributeOwnershipCheckRequest const checkRequest{
      federationName, 0x101U, 0x201U, 0x301U};
  roundTrip(
      checkRequest,
      encodeProcessFederationAttributeOwnershipCheckRequest,
      decodeProcessFederationAttributeOwnershipCheckRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.requestingFederateId == rhs.requestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.attributeHandle == rhs.attributeHandle;
      });

  ProcessFederationAttributeOwnershipQueryRequest const queryRequest{
      federationName, 0x102U, 0x202U, attributeHandles};
  roundTrip(
      queryRequest,
      encodeProcessFederationAttributeOwnershipQueryRequest,
      decodeProcessFederationAttributeOwnershipQueryRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.requestingFederateId == rhs.requestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.requestedAttributeHandles == rhs.requestedAttributeHandles;
      });

  ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest const ifAvailableRequest{
      federationName, 0x103U, 0x203U, attributeHandles, tag};
  roundTrip(
      ifAvailableRequest,
      encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest,
      decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.requestingFederateId == rhs.requestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.desiredAttributeHandles == rhs.desiredAttributeHandles &&
               lhs.userSuppliedTag == rhs.userSuppliedTag;
      });

  ProcessFederationAttributeOwnershipAcquisitionRequest const acquisitionRequest{
      federationName, 0x104U, 0x204U, attributeHandles, tag, false};
  roundTrip(
      acquisitionRequest,
      encodeProcessFederationAttributeOwnershipAcquisitionRequest,
      decodeProcessFederationAttributeOwnershipAcquisitionRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.requestingFederateId == rhs.requestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.desiredAttributeHandles == rhs.desiredAttributeHandles &&
               lhs.userSuppliedTag == rhs.userSuppliedTag &&
               lhs.callbacksEnabled == rhs.callbacksEnabled;
      });

  ProcessFederationAttributeOwnershipReleaseDeniedRequest const releaseDeniedRequest{
      federationName, 0x105U, 0x205U, attributeHandles, tag};
  roundTrip(
      releaseDeniedRequest,
      encodeProcessFederationAttributeOwnershipReleaseDeniedRequest,
      decodeProcessFederationAttributeOwnershipReleaseDeniedRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.owningFederateId == rhs.owningFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.attributeHandles == rhs.attributeHandles &&
               lhs.userSuppliedTag == rhs.userSuppliedTag;
      });

  ProcessFederationAttributeOwnershipAcquisitionCancellationRequest const cancellationRequest{
      federationName, 0x106U, 0x206U, attributeHandles};
  roundTrip(
      cancellationRequest,
      encodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest,
      decodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.requestingFederateId == rhs.requestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.attributeHandles == rhs.attributeHandles;
      });

  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest const cancelDivestitureRequest{
      federationName, 0x107U, 0x207U, attributeHandles};
  roundTrip(
      cancelDivestitureRequest,
      encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest,
      decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.divestingFederateId == rhs.divestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.attributeHandles == rhs.attributeHandles;
      });

  ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest const divestitureRequest{
      federationName, 0x108U, 0x208U, attributeHandles, tag};
  roundTrip(
      divestitureRequest,
      encodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest,
      decodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.divestingFederateId == rhs.divestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.attributeHandles == rhs.attributeHandles &&
               lhs.userSuppliedTag == rhs.userSuppliedTag;
      });

  ProcessFederationConfirmDivestitureRequest const confirmRequest{
      federationName, 0x109U, 0x209U, attributeHandles, tag};
  roundTrip(
      confirmRequest,
      encodeProcessFederationConfirmDivestitureRequest,
      decodeProcessFederationConfirmDivestitureRequest,
      [](auto const& lhs, auto const& rhs) {
        return lhs.federationName == rhs.federationName &&
               lhs.divestingFederateId == rhs.divestingFederateId &&
               lhs.objectInstanceHandle == rhs.objectInstanceHandle &&
               lhs.attributeHandles == rhs.attributeHandles &&
               lhs.userSuppliedTag == rhs.userSuppliedTag;
      });

  ProcessFederationAttributeOwnershipCheckResult const checkResult{
      AttributeOwnershipCheckStatus::applied, true};
  roundTrip(
      checkResult,
      encodeProcessFederationAttributeOwnershipCheckResult,
      decodeProcessFederationAttributeOwnershipCheckResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status &&
               lhs.ownedByRequestingFederate == rhs.ownedByRequestingFederate;
      });

  ProcessFederationAttributeOwnershipQueryResult const queryResult{
      AttributeOwnershipQueryStatus::object_instance_not_known, 0U};
  roundTrip(
      queryResult,
      encodeProcessFederationAttributeOwnershipQueryResult,
      decodeProcessFederationAttributeOwnershipQueryResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult const ifAvailableResult{
      AttributeOwnershipAcquisitionIfAvailableStatus::attribute_already_being_acquired, 0U};
  roundTrip(
      ifAvailableResult,
      encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult,
      decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationAttributeOwnershipAcquisitionResult const acquisitionResult{
      AttributeOwnershipAcquisitionStatus::attribute_not_defined, 0U};
  roundTrip(
      acquisitionResult,
      encodeProcessFederationAttributeOwnershipAcquisitionResult,
      decodeProcessFederationAttributeOwnershipAcquisitionResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationAttributeOwnershipReleaseDeniedResult const releaseDeniedResult{
      AttributeOwnershipReleaseDeniedStatus::attribute_not_owned, 0U};
  roundTrip(
      releaseDeniedResult,
      encodeProcessFederationAttributeOwnershipReleaseDeniedResult,
      decodeProcessFederationAttributeOwnershipReleaseDeniedResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationAttributeOwnershipAcquisitionCancellationResult const cancellationResult{
      AttributeOwnershipAcquisitionCancellationStatus::attribute_acquisition_was_not_requested,
      0U};
  roundTrip(
      cancellationResult,
      encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult,
      decodeProcessFederationAttributeOwnershipAcquisitionCancellationResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult const cancelDivestitureResult{
      CancelNegotiatedAttributeOwnershipDivestitureStatus::attribute_divestiture_was_not_requested,
      0U};
  roundTrip(
      cancelDivestitureResult,
      encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult,
      decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationNegotiatedAttributeOwnershipDivestitureResult const divestitureResult{
      NegotiatedAttributeOwnershipDivestitureStatus::attribute_already_being_divested,
      0U};
  roundTrip(
      divestitureResult,
      encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult,
      decodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });

  ProcessFederationConfirmDivestitureResult const confirmResult{
      ConfirmDivestitureStatus::no_acquisition_pending, 0U};
  roundTrip(
      confirmResult,
      encodeProcessFederationConfirmDivestitureResult,
      decodeProcessFederationConfirmDivestitureResult,
      [](auto const& lhs, auto const& rhs) {
        return lhs.status == rhs.status && lhs.recipientCount == rhs.recipientCount;
      });
}

TEST_CASE(
    "Process exception-report event codec preserves private source metadata and legacy MOM suffixes",
    "[transport][process-boundary][process-exception-report-codec]") {
  using umbra::detail::ProcessFederationInteractionEvent;
  using umbra::detail::ProcessFederationServiceProtocolError;
  using umbra::detail::decodeProcessFederationReceiveInteractionResult;
  using umbra::detail::encodeProcessFederationReceiveInteractionResult;

  ProcessFederationInteractionEvent event;
  event.receivingFederateId = 7U;
  event.interactionClassHandle = 11U;
  event.parameterHandles = {2U, 3U, 5U};
  event.payload = {10U, 20U, 30U};
  event.transportationName = "HLAreliable";
  event.rtiOwnedMomInteraction = true;
  event.exceptionReportFederateId = 19U;

  SECTION("Report without explicit order metadata") {
    auto const decoded = decodeProcessFederationReceiveInteractionResult(
        encodeProcessFederationReceiveInteractionResult({event}));
    REQUIRE(decoded.event.has_value());
    CHECK(decoded.event->rtiOwnedMomInteraction);
    CHECK(decoded.event->exceptionReportFederateId == 19U);
    CHECK(decoded.event->producingFederateId == 0U);
    CHECK(decoded.event->receivingFederateId == 7U);
    CHECK(decoded.event->parameterHandles == event.parameterHandles);
    CHECK(decoded.event->payload == event.payload);
    CHECK_FALSE(decoded.event->sentOrderType.has_value());
    CHECK_FALSE(decoded.event->receivedOrderType.has_value());
  }
  SECTION("Report after explicit order metadata") {
    event.sentOrderType = rti1516_2025::RECEIVE;
    event.receivedOrderType = rti1516_2025::RECEIVE;
    auto const decoded = decodeProcessFederationReceiveInteractionResult(
        encodeProcessFederationReceiveInteractionResult({event}));
    REQUIRE(decoded.event.has_value());
    CHECK(decoded.event->rtiOwnedMomInteraction);
    CHECK(decoded.event->exceptionReportFederateId == 19U);
    CHECK(decoded.event->sentOrderType == rti1516_2025::RECEIVE);
    CHECK(decoded.event->receivedOrderType == rti1516_2025::RECEIVE);
  }
  SECTION("Legacy RTI-owned marker without exception report source") {
    event.exceptionReportFederateId.reset();
    auto const decoded = decodeProcessFederationReceiveInteractionResult(
        encodeProcessFederationReceiveInteractionResult({event}));
    REQUIRE(decoded.event.has_value());
    CHECK(decoded.event->rtiOwnedMomInteraction);
    CHECK_FALSE(decoded.event->exceptionReportFederateId.has_value());
  }
  SECTION("Ordinary interaction remains unmarked") {
    event.exceptionReportFederateId.reset();
    event.rtiOwnedMomInteraction = false;
    event.producingFederateId = 19U;
    auto const decoded = decodeProcessFederationReceiveInteractionResult(
        encodeProcessFederationReceiveInteractionResult({event}));
    REQUIRE(decoded.event.has_value());
    CHECK_FALSE(decoded.event->rtiOwnedMomInteraction);
    CHECK_FALSE(decoded.event->exceptionReportFederateId.has_value());
    CHECK(decoded.event->producingFederateId == 19U);
  }
  SECTION("Malformed source suffixes are rejected") {
    auto truncated = encodeProcessFederationReceiveInteractionResult({event});
    truncated.pop_back();
    CHECK_THROWS_AS(decodeProcessFederationReceiveInteractionResult(truncated),
                    ProcessFederationServiceProtocolError);
    auto zeroSource = encodeProcessFederationReceiveInteractionResult({event});
    std::fill(zeroSource.end() - 8, zeroSource.end(), 0U);
    CHECK_THROWS_AS(decodeProcessFederationReceiveInteractionResult(zeroSource),
                    ProcessFederationServiceProtocolError);
    auto trailing = encodeProcessFederationReceiveInteractionResult({event});
    trailing.push_back(0U);
    CHECK_THROWS_AS(decodeProcessFederationReceiveInteractionResult(trailing),
                    ProcessFederationServiceProtocolError);
    event.exceptionReportFederateId = 0U;
    CHECK_THROWS_AS(encodeProcessFederationReceiveInteractionResult({event}),
                    ProcessFederationServiceProtocolError);
    event.exceptionReportFederateId = 19U;
    event.rtiOwnedMomInteraction = false;
    event.producingFederateId = 19U;
    CHECK_THROWS_AS(encodeProcessFederationReceiveInteractionResult({event}),
                    ProcessFederationServiceProtocolError);
  }
}
