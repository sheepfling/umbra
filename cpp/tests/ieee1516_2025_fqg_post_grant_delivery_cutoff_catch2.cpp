#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
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
#error "The FQG post-grant delivery-cutoff test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

class FqgDeliveryCutoffFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const& time,
      OrderType,
      OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    receivedTimes.push_back(time.toString());
    callbackOrder.push_back("interaction");
  }

  void flushQueueGrant(
      LogicalTime const& time,
      LogicalTime const&) override {
    ++flushQueueGrantCount;
    flushQueueGrantTime = time.toString();
    callbackOrder.push_back("flush-grant");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    ++timeAdvanceGrantCount;
    timeAdvanceGrantTime = time.toString();
    callbackOrder.push_back("grant");
  }

  std::vector<std::wstring> receivedTimes;
  std::vector<std::string> callbackOrder;
  std::wstring flushQueueGrantTime;
  std::wstring timeAdvanceGrantTime;
  std::size_t flushQueueGrantCount = 0U;
  std::size_t timeAdvanceGrantCount = 0U;
};

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

void drainCallbacks(rti1516_2025::RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"fqg-post-grant-delivery-cutoff-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded FQG never delivers a later TSO below its grant time",
    "[integration][development-profile][2025][time-management][tso]"
    "[fqg-post-grant-delivery-cutoff]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[rti.service.send-interaction][federate.callback.receive-interaction]"
    "[federate.callback.flush-queue-grant][federate.callback.time-advance-grant]") {
  FqgDeliveryCutoffFederateAmbassador firstPublisherReports;
  FqgDeliveryCutoffFederateAmbassador receiverReports;
  FqgDeliveryCutoffFederateAmbassador latePublisherReports;
  auto firstPublisher = makeRti();
  auto receiver = makeRti();
  auto latePublisher = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0x46U, 0x51U, 0x47U};
  std::vector<unsigned char> const tagBytes{0x43U, 0x55U, 0x54U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(firstPublisher->connect(firstPublisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(latePublisher->connect(latePublisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstPublisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(firstPublisher->joinFederationExecution(
      L"fqg-cutoff-first-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"fqg-cutoff-receiver", L"subscriber", federationName));

  auto const interactionClass = firstPublisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = firstPublisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());

  auto preparePublisher = [&](rti1516_2025::RTIambassador& publisher) {
    REQUIRE_NOTHROW(publisher.publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher.changeInteractionOrderType(interactionClass, TIMESTAMP));
  };
  preparePublisher(*firstPublisher);
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(firstPublisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*firstPublisher);

  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const pregrantMessage = firstPublisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(pregrantMessage.isValid());
  REQUIRE(receiverReports.receivedTimes.empty());
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(firstPublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*firstPublisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receivedTimes == std::vector<std::wstring>{L"7"});
  REQUIRE(receiverReports.flushQueueGrantCount == 1U);
  REQUIRE(receiverReports.flushQueueGrantTime == L"7");
  REQUIRE(receiverReports.callbackOrder ==
      std::vector<std::string>{"interaction", "flush-grant"});

  // A separate, newly joined producer has a valid floor below the receiver's
  // FQG grant. After FQG(10)->FQG(7), its timestamp-6 message is strictly below
  // that grant, while timestamp 8 is a legal later TSO control.
  REQUIRE_NOTHROW(latePublisher->joinFederationExecution(
      L"fqg-cutoff-late-publisher", L"publisher", federationName));
  preparePublisher(*latePublisher);
  REQUIRE_NOTHROW(latePublisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*latePublisher);
  auto const belowGrantMessage = latePublisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(6));
  auto const legalMessage = latePublisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(8));
  REQUIRE(belowGrantMessage.isValid());
  REQUIRE(legalMessage.isValid());
  // Move the two regulating producers in staggered steps. Requesting 9 from
  // both at once deadlocks their TARs: each waits for the other's GALT.
  REQUIRE_NOTHROW(latePublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*latePublisher);
  REQUIRE_NOTHROW(firstPublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(3)));
  drainCallbacks(*firstPublisher);
  REQUIRE_NOTHROW(latePublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*latePublisher);
  REQUIRE_NOTHROW(firstPublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*firstPublisher);
  REQUIRE_NOTHROW(latePublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*latePublisher);
  REQUIRE_NOTHROW(firstPublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drainCallbacks(*firstPublisher);
  REQUIRE_NOTHROW(latePublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  drainCallbacks(*latePublisher);
  REQUIRE_NOTHROW(firstPublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  drainCallbacks(*firstPublisher);
  REQUIRE_NOTHROW(latePublisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*latePublisher);
  REQUIRE(firstPublisherReports.timeAdvanceGrantTime == L"8");
  REQUIRE(latePublisherReports.timeAdvanceGrantTime == L"10");

  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(9)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantCount == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantTime == L"8");
  REQUIRE(receiverReports.receivedTimes ==
      std::vector<std::wstring>{L"7", L"8"});
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "interaction", "flush-grant", "interaction", "grant"});

  REQUIRE_NOTHROW(latePublisher->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(firstPublisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(firstPublisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(latePublisher->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(firstPublisher->disconnect());
}
