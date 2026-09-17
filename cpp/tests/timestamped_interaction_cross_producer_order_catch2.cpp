#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

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
#error "The cross-producer timestamped-interaction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectInstanceHandle;
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
  return L"timestamped-interaction-cross-producer-order-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimestampedInteractionReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
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

  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
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
        ObjectInstanceHandle{},
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

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped interactions preserve different-timestamp order for each constrained recipient",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][multi-federate-callback-ordering]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador earlyReports;
  ReportingFederateAmbassador lateReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador receiverPeerReports;
  auto early = makeRti();
  auto late = makeRti();
  auto receiver = makeRti();
  auto receiverPeer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  unsigned char const earlyTagBytes[] = {0x45, 0x41};
  unsigned char const lateTagBytes[] = {0x4C, 0x41};
  unsigned char const lateEqualTagBytes[] = {0x4C, 0x45};
  VariableLengthData const earlyTag(earlyTagBytes, sizeof(earlyTagBytes));
  VariableLengthData const lateTag(lateTagBytes, sizeof(lateTagBytes));
  VariableLengthData const lateEqualTag(
      lateEqualTagBytes,
      sizeof(lateEqualTagBytes));

  REQUIRE_NOTHROW(early->connect(earlyReports, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiverPeer->connect(receiverPeerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(early->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));

  FederateHandle earlyHandle;
  FederateHandle lateHandle;
  REQUIRE_NOTHROW(earlyHandle = early->joinFederationExecution(
      L"timestamp-order-early",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(lateHandle = late->joinFederationExecution(
      L"timestamp-order-late",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamp-order-receiver",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(receiverPeer->joinFederationExecution(
      L"timestamp-order-receiver-peer",
      L"subscriber",
      federationName));

  auto const interactionClass = early->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = early->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  auto const receiverPeerInteractionClass = receiverPeer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const reliableTransportation = early->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(receiverPeerInteractionClass.isValid());
  REQUIRE(reliableTransportation.isValid());
  unsigned char const identifierBytes[] = {0x53, 0x54, 0x4F};
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(early->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(late->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(early->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(late->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiverPeer->subscribeInteractionClass(receiverPeerInteractionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiverPeer->enableTimeConstrained());
  REQUIRE_FALSE(receiverPeer->evokeCallback(0.0));
  REQUIRE_NOTHROW(early->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(early->evokeCallback(0.0));
  REQUIRE_NOTHROW(late->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(late->evokeCallback(0.0));

  // Submit the later timestamp first. Different timestamps must still be
  // delivered in timestamp order to every constrained recipient; the two
  // equal-timestamp messages deliberately have no tie-break requirement.
  auto const lateMessageHandle = late->sendInteraction(
      interactionClass,
      parameterValues,
      lateTag,
      rti1516_2025::HLAinteger64Time(7));
  auto const earlyMessageHandle = early->sendInteraction(
      interactionClass,
      parameterValues,
      earlyTag,
      rti1516_2025::HLAinteger64Time(5));
  auto const lateEqualMessageHandle = late->sendInteraction(
      interactionClass,
      parameterValues,
      lateEqualTag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(lateMessageHandle.isValid());
  REQUIRE(earlyMessageHandle.isValid());
  REQUIRE(lateEqualMessageHandle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverPeerReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(early->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(late->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(early->evokeCallback(0.0));
  REQUIRE_FALSE(late->evokeCallback(0.0));

  // Advance both recipients to the first boundary, then to the later one.
  // This keeps the two callback queues independent and checks the ordering
  // contract separately for each constrained federate.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(receiverPeer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  while (receiver->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  while (receiverPeer->evokeMultipleCallbacks(0.0, 0.0)) {
  }

  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(receiverPeerReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverPeerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverPeerReports.callbackOrder ==
          std::vector<std::string>{"interaction", "interaction", "grant"});
  REQUIRE(receiverPeerReports.timeAdvanceGrantReports.front().value == L"5");

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(receiverPeer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  while (receiver->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  while (receiverPeer->evokeMultipleCallbacks(0.0, 0.0)) {
  }

  REQUIRE(receiverReports.timestampedInteractionReports.size() == 3U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "interaction", "grant", "interaction", "grant"});
  REQUIRE(receiverPeerReports.timestampedInteractionReports.size() == 3U);
  REQUIRE(receiverPeerReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverPeerReports.callbackOrder ==
          std::vector<std::string>{"interaction", "interaction", "grant", "interaction", "grant"});

  auto requireCommon = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(identifier));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
            std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
    REQUIRE(report.transportationType == reliableTransportation);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  auto requireOrder = [&](auto const& reports) {
    auto const& first = reports.timestampedInteractionReports[0];
    auto const& second = reports.timestampedInteractionReports[1];
    auto const& third = reports.timestampedInteractionReports[2];
    REQUIRE(first.timeValue == L"5");
    REQUIRE(second.timeValue == L"5");
    REQUIRE(third.timeValue == L"7");
    REQUIRE(((first.producingFederate == earlyHandle && second.producingFederate == lateHandle) ||
             (first.producingFederate == lateHandle && second.producingFederate == earlyHandle)));
    REQUIRE(third.producingFederate == lateHandle);
    REQUIRE(((variableLengthDataBytes(first.userSuppliedTag) ==
              std::vector<unsigned char>(earlyTagBytes, earlyTagBytes + sizeof(earlyTagBytes)) &&
              variableLengthDataBytes(second.userSuppliedTag) ==
                  std::vector<unsigned char>(lateEqualTagBytes, lateEqualTagBytes + sizeof(lateEqualTagBytes))) ||
             (variableLengthDataBytes(first.userSuppliedTag) ==
                  std::vector<unsigned char>(lateEqualTagBytes, lateEqualTagBytes + sizeof(lateEqualTagBytes)) &&
              variableLengthDataBytes(second.userSuppliedTag) ==
                  std::vector<unsigned char>(earlyTagBytes, earlyTagBytes + sizeof(earlyTagBytes)))));
    REQUIRE(variableLengthDataBytes(third.userSuppliedTag) ==
            std::vector<unsigned char>(lateTagBytes, lateTagBytes + sizeof(lateTagBytes)));
    requireCommon(first);
    requireCommon(second);
    requireCommon(third);
  };

  requireOrder(receiverReports);
  requireOrder(receiverPeerReports);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(receiverPeerReports.timeAdvanceGrantReports.back().value == L"7");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiverPeer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(late->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(early->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(early->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(receiverPeer->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(early->disconnect());
}
