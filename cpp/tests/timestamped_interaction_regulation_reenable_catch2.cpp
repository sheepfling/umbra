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
#error "The timestamped interaction regulation re-enable test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
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
  return L"timestamped-interaction-regulation-reenable-" +
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

  void timeConstrainedEnabled(LogicalTime const&) override {
    ++timeConstrainedEnabledCount;
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void timeAdvanceGrant(LogicalTime const&) override {
    ++timeAdvanceGrantCount;
    callbackOrder.push_back("grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::size_t timeConstrainedEnabledCount = 0U;
  std::size_t timeRegulationEnabledCount = 0U;
  std::size_t timeAdvanceGrantCount = 0U;
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
    "Embedded queued timestamped interaction survives time-regulation disable and re-enable",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][re-enable][regulation-disable]"
    "[rti.service.send-interaction][rti.service.enable-time-regulation]"
    "[rti.service.disable-time-regulation][rti.service.time-advance-request]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath(
      "parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const parameterBytes{0xD1U, 0x5AU};
  std::vector<unsigned char> const tagBytes{
      0x52U, 0x45U, 0x47U, 0x45U, 0x4EU};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regulation-reenable-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regulation-reenable-receiver",
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
      VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledCount == 1U);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Disabling regulation must not discard an accepted TSO passel. Re-enabling
  // at the same lookahead establishes a new producer boundary for the same
  // joined-federate lifetime.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE(publisherReports.timeRegulationEnabledCount == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledCount == 2U);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantCount == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          parameterBytes);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) == tagBytes);
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE_FALSE(report.sentRegionsSupplied);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"5");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable",
    "[integration][development-profile][interaction-management][time-management]"
    "[tso][retraction][re-enable][regulation-disable]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request]"
    "[federate.callback.time-regulation-enabled]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath(
      "parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const parameterBytes{0xD1U, 0x5AU};
  std::vector<unsigned char> const tagBytes{
      0x52U, 0x45U, 0x54U, 0x52U, 0x41U, 0x43U, 0x54U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"retraction-lifetime-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"retraction-lifetime-receiver",
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
      VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledCount == 1U);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The designator belongs to the joined-federate lifetime. Disabling time
  // regulation temporarily removes the authority to retract, but must not
  // erase the accepted timestamped message or its producer-owned identity.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledCount == 2U);

  // After the callback-gated re-enable the same designator is legal again;
  // the successful retract terminalizes it and suppresses recipient delivery.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
