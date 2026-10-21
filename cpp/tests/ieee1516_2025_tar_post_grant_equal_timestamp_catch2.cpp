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
#error "The post-grant equal-timestamp test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

class GrantBoundaryFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    receivedTimes.push_back(time.toString());
    receivedTags.emplace_back(
        static_cast<char const*>(userSuppliedTag.data()), userSuppliedTag.size());
    callbackOrder.push_back("interaction");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    grantedTimes.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  std::vector<std::wstring> receivedTimes;
  std::vector<std::string> receivedTags;
  std::vector<std::wstring> grantedTimes;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
      relativePath;
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
  return L"tar-post-grant-equal-timestamp-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded TAR prevents later delivery of TSO timestamps at or below its grant",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][tar-post-grant-equal-timestamp]"
    "[rti.service.time-advance-request][rti.service.send-interaction]"
    "[federate.callback.time-advance-grant][federate.callback.receive-interaction]") {
  // This case is limited to the post-TAR boundary in 2025 clause 8.9.5:
  // TSO at the granted time may arrive later; an accepted TSO below that
  // time must not be delivered after the grant. It does not claim general
  // late-transport, TARA/NMR/NMRA, or cross-process behavior.
  GrantBoundaryFederateAmbassador receiverReports;
  GrantBoundaryFederateAmbassador publisherReports;
  GrantBoundaryFederateAmbassador frontierReports;
  auto receiver = makeRti();
  auto publisher = makeRti();
  auto frontier = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const payload{0x47U, 0x52U, 0x41U, 0x4EU, 0x54U};
  rti1516_2025::VariableLengthData const valuesBytes(payload.data(), payload.size());

  REQUIRE_NOTHROW(receiver->connect(receiverReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->connect(publisherReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(frontier->connect(frontierReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tar-post-grant-receiver", L"publisher", federationName));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tar-post-grant-late-regulator", L"publisher", federationName));
  REQUIRE_NOTHROW(frontier->joinFederationExecution(
      L"tar-post-grant-frontier-regulator", L"publisher", federationName));

  auto const interactionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const publisherInteractionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(publisherInteractionClass.isValid());
  auto const parameter = publisher->getParameterHandle(
      publisherInteractionClass, fixture_hla::fixture::identifier);
  REQUIRE(parameter.isValid());

  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteractionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(publisherInteractionClass, rti1516_2025::TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receivedTimes.empty());

  // Establish the TAR's grant frontier with a separate regulator. The later
  // publisher is deliberately not regulating yet, so it can become a new
  // TSO producer after this completed grant.
  REQUIRE_NOTHROW(frontier->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*frontier);
  REQUIRE_NOTHROW(frontier->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*frontier);
  REQUIRE(frontierReports.grantedTimes == std::vector<std::wstring>{L"5"});

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.grantedTimes == std::vector<std::wstring>{L"5"});
  REQUIRE(receiverReports.receivedTimes.empty());

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  rti1516_2025::ParameterHandleValueMap values;
  values.emplace(parameter, valuesBytes);
  std::vector<unsigned char> const belowTagBytes{'b', 'e', 'l', 'o', 'w'};
  std::vector<unsigned char> const equalTagBytes{'e', 'q', 'u', 'a', 'l'};
  rti1516_2025::VariableLengthData const belowTag(
      belowTagBytes.data(), belowTagBytes.size());
  rti1516_2025::VariableLengthData const equalTag(
      equalTagBytes.data(), equalTagBytes.size());

  try {
    static_cast<void>(publisher->sendInteraction(
        publisherInteractionClass,
        values,
        belowTag,
        rti1516_2025::HLAinteger64Time(4)));
  } catch (rti1516_2025::InvalidLogicalTime const&) {
    // The standard constrains delivery after the grant; it does not require
    // a particular sender-side rejection strategy for this edge.
  }

  auto const equalGrant = publisher->sendInteraction(
      publisherInteractionClass,
      values,
      equalTag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(equalGrant.isValid());
  REQUIRE(receiverReports.receivedTimes.empty());

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.grantedTimes == std::vector<std::wstring>{L"4"});
  REQUIRE(receiverReports.receivedTimes.empty());
  REQUIRE(receiverReports.receivedTags.empty());
  REQUIRE(receiverReports.grantedTimes == std::vector<std::wstring>{L"5"});

  // Once the producer moves its safe frontier beyond 5, request a later grant
  // so any delayed payload would cross the public callback boundary. Do not
  // pump the receiver after the late sends: stale in-transit timestamps must
  // not hold its next GALT calculation hostage to callback processing. Both
  // timestamp-4 and timestamp-5 payloads remain forbidden by the grant at 5.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.grantedTimes ==
          std::vector<std::wstring>{L"4", L"6"});
  REQUIRE_NOTHROW(frontier->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*frontier);
  REQUIRE(frontierReports.grantedTimes ==
          std::vector<std::wstring>{L"5", L"6"});
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receivedTimes.empty());
  REQUIRE(receiverReports.receivedTags.empty());
  REQUIRE(receiverReports.grantedTimes ==
          std::vector<std::wstring>{L"5", L"6"});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(frontier->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(frontier->disconnect());
}
