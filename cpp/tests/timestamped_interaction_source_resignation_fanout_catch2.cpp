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
#error "The source-resignation timestamped-interaction test requires the Umbra source directory."
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
  return L"timestamped-interaction-source-resignation-fanout-" +
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

  void timeConstrainedEnabled(LogicalTime const&) override {
    ++timeConstrainedEnabledCount;
  }

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
  std::size_t timeConstrainedEnabledCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded queued timestamped interaction survives source resignation for each recipient",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][resignation][multi-federate-callback-ordering]"
    "[rti.service.send-interaction][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.next-message-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador clockReports;
  auto sender = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  unsigned char const identifierBytes[] = {0x46, 0x41, 0x4E, 0x2D, 0x4F};
  unsigned char const tagBytes[] = {0x46, 0x41, 0x4E, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"tso-resignation-fanout-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"tso-resignation-fanout-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"tso-resignation-fanout-second", L"subscriber", federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"tso-resignation-fanout-clock", L"publisher", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = sender->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  auto const reliableTransportation = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(reliableTransportation.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(first->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(second->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(clock->evokeCallback(0.0));

  auto const retraction = sender->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReports.timestampedInteractionReports.empty());
  REQUIRE(secondReports.timestampedInteractionReports.empty());

  // Admit both recipient frontiers while the producer is still present. The
  // producer then resigns, so the accepted payload must remain available in
  // each recipient-local queue and retain its valid retraction handle.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(second->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(firstReports.timeAdvanceGrantReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(sender->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE(firstReports.timestampedInteractionReports.empty());
  REQUIRE(secondReports.timestampedInteractionReports.empty());

  // An independent regulator supplies the federation-wide temporal frontier
  // after the producing federate has left the execution.
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (clockReports.timeAdvanceGrantReports.empty() &&
         clock->evokeCallback(0.0)) {
  }
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);

  while (firstReports.timeAdvanceGrantReports.empty() &&
         first->evokeCallback(0.0)) {
  }
  REQUIRE(firstReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(firstReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});
  REQUIRE(secondReports.timestampedInteractionReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());

  while (secondReports.timeAdvanceGrantReports.empty() &&
         second->evokeCallback(0.0)) {
  }
  REQUIRE(secondReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(secondReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto requireReport = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(identifier));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
            std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == reliableTransportation);
    REQUIRE(report.producingFederate == senderHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"6");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireReport(firstReports.timestampedInteractionReports.front());
  requireReport(secondReports.timestampedInteractionReports.front());
  REQUIRE(firstReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(secondReports.timeAdvanceGrantReports.front().value == L"6");

  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}

TEST_CASE(
    "Embedded queued timestamped interaction survives source resignation",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][resignation]"
    "[rti.service.send-interaction][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-constrained-enabled]"
    "[federate.callback.time-regulation-enabled][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador clockReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  unsigned char const identifierBytes[] = {0x53, 0x49, 0x4E, 0x47};
  unsigned char const tagBytes[] = {0x53, 0x49, 0x4E, 0x47, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"tso-resignation-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tso-resignation-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"tso-resignation-clock", L"publisher", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = sender->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  auto const reliableTransportation = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(reliableTransportation.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 1U);
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*sender);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*clock);

  auto const retraction = sender->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Queue the recipient's frontier while the producer is still present, then
  // resign the producer before the queued interaction can be delivered.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(sender->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The independent regulator releases the surviving receiver's queued TSO
  // message after the original producer has left the federation.
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drain(*clock);
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);
  drain(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes,
                                     identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.transportationType == reliableTransportation);
  REQUIRE(report.producingFederate == senderHandle);
  REQUIRE_FALSE(report.sentRegionsSupplied);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().timeImplementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"6");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
