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
#error "The Flush Queue delivered-callback test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"fqr-delivered-callbacks-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class FqrDeliveryReports final : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Interaction final {
    InteractionClassHandle interactionClass;
    std::wstring timestamp;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {}

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      OrderType,
      OrderType,
      MessageRetractionHandle const*) override {
    interactions.push_back({interactionClass, time.toString()});
    callbackOrder.push_back("interaction");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    grantedTime = time.toString();
    optimisticTimeValue = optimisticTime.toString();
    callbackOrder.push_back("flush-grant");
  }

  std::vector<Interaction> interactions;
  std::vector<std::string> callbackOrder;
  std::wstring grantedTime;
  std::wstring optimisticTimeValue;
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

enum class SuppressionMode {
  earliest,
  all,
  retract_earliest,
};

void verifyDeliveredFqrTimes(
    SuppressionMode suppression,
    bool constrainByGalt) {
  FqrDeliveryReports publisherReports;
  FqrDeliveryReports receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const timeSwitchFom = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "switch-nrg-enabled-fom.xml").wstring();
  auto const interactionFom = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();
  std::vector<std::wstring> fomModules{timeSwitchFom, interactionFom};
  unsigned char const valueBytes[] = {0x46U, 0x51U, 0x52U};
  unsigned char const tagBytes[] = {0x44U, 0x43U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"fqr-delivered-producer", L"producer", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"fqr-delivered-receiver", L"receiver", federationName));
  REQUIRE(receiver->getNonRegulatedGrantSwitch());

  auto const baseClass = publisher->getInteractionClassHandle(
      umbra::test::hla::wide::fom::parameter_fixture_base);
  auto const childClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const baseParameter = publisher->getParameterHandle(
      baseClass, fixture_hla::fixture::identifier);
  auto const childParameter = publisher->getParameterHandle(
      childClass, fixture_hla::fixture::identifier);
  REQUIRE(baseClass.isValid());
  REQUIRE(childClass.isValid());
  REQUIRE(baseParameter.isValid());
  REQUIRE(childParameter.isValid());

  REQUIRE_NOTHROW(publisher->publishInteractionClass(baseClass));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(childClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(baseClass, TIMESTAMP));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(childClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(baseClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(childClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(constrainByGalt ? 4 : 1)));
  drainCallbacks(*publisher);

  ParameterHandleValueMap earliestValues;
  earliestValues.emplace(
      baseParameter, VariableLengthData(valueBytes, sizeof(valueBytes)));
  ParameterHandleValueMap laterValues;
  laterValues.emplace(
      childParameter, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const earliest = publisher->sendInteraction(
      baseClass,
      earliestValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(earliest.isValid());
  REQUIRE(publisher->sendInteraction(
              childClass,
              laterValues,
              tag,
              rti1516_2025::HLAinteger64Time(7))
              .isValid());

  if (suppression == SuppressionMode::earliest ||
      suppression == SuppressionMode::all) {
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(baseClass));
  }
  if (suppression == SuppressionMode::all) {
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(childClass));
  }
  if (!constrainByGalt && suppression != SuppressionMode::retract_earliest) {
    REQUIRE_NOTHROW(publisher->disableTimeRegulation());
    drainCallbacks(*publisher);
  }

  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.interactions.empty());
  REQUIRE(receiverReports.grantedTime.empty());
  if (suppression == SuppressionMode::retract_earliest) {
    REQUIRE_NOTHROW(publisher->retract(earliest));
  }
  drainCallbacks(*receiver);

  auto const expectLater = suppression != SuppressionMode::all;
  REQUIRE(receiverReports.interactions.size() == (expectLater ? 1U : 0U));
  if (expectLater) {
    REQUIRE(receiverReports.interactions.front().interactionClass == childClass);
    REQUIRE(receiverReports.interactions.front().timestamp == L"7");
    REQUIRE(receiverReports.callbackOrder ==
            std::vector<std::string>{"interaction", "flush-grant"});
    REQUIRE(receiverReports.optimisticTimeValue == L"7");
  } else {
    REQUIRE(receiverReports.callbackOrder ==
            std::vector<std::string>{"flush-grant"});
    REQUIRE(receiverReports.optimisticTimeValue == L"10");
  }
  auto const expectedActualTime = constrainByGalt
      ? L"4"
      : suppression == SuppressionMode::retract_earliest
          ? L"1"
          : expectLater ? L"7" : L"10";
  REQUIRE(receiverReports.grantedTime == expectedActualTime);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace

TEST_CASE(
    "Embedded FQG derives grant times from TSO callbacks actually delivered",
    "[integration][development-profile][2025][time-management]"
    "[flush-queue-request][fqr-delivered-tso-frontier][fqr-olt-minimum-delivered-tso]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  SECTION("a suppressed earliest message does not lower either grant time") {
    verifyDeliveredFqrTimes(SuppressionMode::earliest, false);
  }
  SECTION("no delivered TSO message leaves both grant times at the request") {
    verifyDeliveredFqrTimes(SuppressionMode::all, false);
  }
  SECTION("GALT may independently limit actual time") {
    verifyDeliveredFqrTimes(SuppressionMode::earliest, true);
  }
  SECTION("retraction before callback dispatch removes the candidate timestamp") {
    verifyDeliveredFqrTimes(SuppressionMode::retract_earliest, false);
  }
}
