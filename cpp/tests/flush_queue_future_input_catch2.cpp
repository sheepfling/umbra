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
#error "The future-input Flush Queue interaction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
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
  return L"flush-queue-future-input-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
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

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
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

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  void requestRetraction(
      MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back(retraction.isValid());
    callbackOrder.push_back("request-retraction");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<bool> requestRetractionReports;
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

void verifyPrequeuedFqr(bool immediate) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x46, 0x55, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  if (immediate) {
    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, rti1516_2025::HLA_IMMEDIATE));
  } else {
    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  }
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"prequeued-fqr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"prequeued-fqr-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xF4, 0x54};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  if (!immediate) {
    drainCallbacks(*receiver);
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  if (!immediate) {
    drainCallbacks(*publisher);
  }

  auto const sent = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(12));
  REQUIRE(sent.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // FQR flushes TSO already queued at invocation, even when its timestamp is
  // greater than the requested time. The actual grant is still bounded by
  // min(requested time, GALT, delivered TSO); optimistic time omits GALT.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  if (!immediate) {
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    drainCallbacks(*receiver);
  }
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(receiverReports.flushQueueGrantReports.front().optimisticValue == L"10");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.timeValue == L"12");
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionValid);

  // FQR may deliver a timestamp later than the request. A later flush may
  // consequently deliver an earlier timestamp, and retracting the already
  // delivered future message must still notify this receiver.
  REQUIRE_THROWS_AS(
      publisher->sendInteraction(
          interactionClass,
          parameterValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);

  auto const earlier = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(earlier.isValid());
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  if (!immediate) {
    drainCallbacks(*receiver);
  }
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports[1].timeValue == L"6");
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports[0].timeValue == L"12");
  REQUIRE_NOTHROW(publisher->retract(sent));
  if (!immediate) {
    drainCallbacks(*receiver);
  }
  REQUIRE(receiverReports.requestRetractionReports == std::vector<bool>{true});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

void verifyFqrGrantUsesDeliveredTsoBoundary() {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  std::vector<unsigned char> const payload{0x46U, 0x51U, 0x52U};
  std::vector<unsigned char> const tagBytes{0x42U, 0x44U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"fqr-bound-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"fqr-bound-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(payload.data(), payload.size()));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Move the only regulator beyond timestamp 9 while the receiver's evoked
  // callback remains pending. FQR(10) must still deliver the queued TSO and
  // account for its timestamp in the grant/optimistic-time result.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(12)));
  drainCallbacks(*publisher);
  rti1516_2025::HLAinteger64Time publisherTime;
  REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherTime));
  REQUIRE(publisherTime.getTime() == 12);

  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.front().value == L"9");
  REQUIRE(receiverReports.flushQueueGrantReports.front().optimisticValue == L"9");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.timeValue == L"9");
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(parameter));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(parameter)) == payload);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) == tagBytes);
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionValid);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace

TEST_CASE(
    "Embedded Flush Queue Request admits TSO input queued after submission",
    "[integration][development-profile][interaction-management][time-management]"
    "[flush-queue-request][flush-queue-future-input][future-input][tso]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x46, 0x55, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"future-input-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"future-input-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xF4, 0x54};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  // This deliberately tests producer input accepted after FQR submission,
  // which is distinct from the standard's requirement to flush TSO already
  // queued when FQR is invoked. The late-arrival boundary is implementation
  // policy, not a normative snapshot rule. A later message is retracted before
  // callback entry so only the surviving timestamp contributes to OLT.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(12));
  REQUIRE(first.isValid());
  REQUIRE(second.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(second));

  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(receiverReports.flushQueueGrantReports.front().optimisticValue == L"9");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});
  REQUIRE(receiverReports.timestampedInteractionReports[0].timeValue == L"9");
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionValid);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Flush Queue grant accounts for a prequeued TSO timestamp",
    "[integration][development-profile][interaction-management][time-management]"
    "[flush-queue-request][fqr-delivered-tso-grant-bound][tso]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]"
    "[federate.callback.flush-queue-grant][2025]") {
  verifyFqrGrantUsesDeliveredTsoBoundary();
}

TEST_CASE(
    "Embedded Flush Queue Request permits later lower TSO and retraction callbacks",
    "[integration][development-profile][interaction-management][time-management]"
    "[flush-queue-request][fqr-postgrant-lower-tso-retraction][tso][callback-model]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  SECTION("HLA_EVOKED") {
    verifyPrequeuedFqr(false);
  }
  SECTION("HLA_IMMEDIATE") {
    verifyPrequeuedFqr(true);
  }
}
