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
#error "The mixed timestamped-interaction test requires the Umbra source directory."
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
  return L"timestamped-interaction-mixed-advance-" +
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

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
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

  void flushQueueGrant(
      LogicalTime const& time,
      LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded mixed timestamped interactions deliver before FQR TARA and NMRA grants",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][multi-federate-callback-ordering][flush-queue-request]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador fqrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto fqr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  unsigned char const identifierBytes[] = {0x49, 0x4E, 0x54, 0x2D, 0x41};
  unsigned char const tagBytes[] = {0x49, 0x4E, 0x54, 0x2D, 0x46};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mixed-timestamped-interaction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"mixed-timestamped-interaction-fqr",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"mixed-timestamped-interaction-tara",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"mixed-timestamped-interaction-nmra",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  auto const reliableTransport = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(reliableTransport.isValid());
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(fqr->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(tara->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(nmra->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(fqr->enableTimeConstrained());
  REQUIRE_NOTHROW(tara->enableTimeConstrained());
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  while (fqr->evokeCallback(0.0)) {
  }
  while (tara->evokeCallback(0.0)) {
  }
  while (nmra->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  fqrReports.callbackOrder.clear();
  taraReports.callbackOrder.clear();
  nmraReports.callbackOrder.clear();
  REQUIRE(fqrReports.timestampedInteractionReports.empty());
  REQUIRE(taraReports.timestampedInteractionReports.empty());
  REQUIRE(nmraReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(fqr->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  // Move the regulator to four so its lookahead-one GALT reaches timestamp
  // five; the alternate grants must still deliver before their own grants.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));

  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE(taraReports.callbackOrder.empty());
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));

  auto requireInteraction = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(identifier));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
            std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == reliableTransport);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"5");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
  };

  REQUIRE(fqrReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(taraReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(nmraReports.timestampedInteractionReports.size() == 1U);
  requireInteraction(fqrReports.timestampedInteractionReports.front());
  requireInteraction(taraReports.timestampedInteractionReports.front());
  requireInteraction(nmraReports.timestampedInteractionReports.front());
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"5");
  REQUIRE(fqrReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(taraReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(nmraReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"4");

  REQUIRE_NOTHROW(fqr->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(tara->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(nmra->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(fqr->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
