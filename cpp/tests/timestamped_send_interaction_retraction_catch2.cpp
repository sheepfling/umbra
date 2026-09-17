#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped Send Interaction retraction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-send-interaction-retraction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimestampedInteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("interaction");
  }

  void timeAdvanceGrant(LogicalTime const&) override {
    ++timeAdvanceGrantCount;
    callbackOrder.push_back("grant");
  }

  void flushQueueGrant(
      LogicalTime const&,
      LogicalTime const&) override {
    ++flushQueueGrantCount;
  }

  void requestRetraction(MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({
        retraction.isValid(),
        retraction.encode(),
    });
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::size_t timeAdvanceGrantCount = 0U;
  std::size_t flushQueueGrantCount = 0U;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded timestamped Send Interaction queues TSO and requests retraction after delivery",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath(
      "parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const identifierBytes{0xA1U, 0xB2U};
  std::vector<unsigned char> const tagBytes{0x31U, 0x42U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-receiver",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes.data(), identifierBytes.size()));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  REQUIRE_THROWS_AS(
      publisher->sendInteraction(
          interactionClass,
          parameterValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Retracting before the receiver's grant removes the pending TSO entry.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantCount == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const secondHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantCount == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "interaction", "grant"});
  auto const& secondReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(secondReport.interactionClass == interactionClass);
  REQUIRE(secondReport.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(secondReport.timeValue == L"7");
  REQUIRE(secondReport.sentOrderType == TIMESTAMP);
  REQUIRE(secondReport.receivedOrderType == TIMESTAMP);
  REQUIRE(secondReport.retractionSupplied);
  REQUIRE(secondReport.retractionValid);
  REQUIRE(secondReport.parameterValues.size() == 1U);
  REQUIRE(secondReport.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(secondReport.parameterValues.at(identifier)) ==
          identifierBytes);
  REQUIRE(variableLengthDataBytes(secondReport.userSuppliedTag) == tagBytes);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE(receiverReports.requestRetractionReports.empty());

  auto const terminalHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(terminalHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(3)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"8");
  REQUIRE(receiverReports.timestampedInteractionReports.back().retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(terminalHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const thirdHandle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(thirdHandle.isValid());
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(9)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 3U);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"9");
  REQUIRE(receiverReports.timestampedInteractionReports.back().retractionValid);
  REQUIRE(receiverReports.flushQueueGrantCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(thirdHandle));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(thirdHandle.encode()));

  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
