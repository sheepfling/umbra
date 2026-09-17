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
#error "The alternate-advance default-region interaction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-default-region-interaction-retract-alternate-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
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

  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
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

  void requestRetraction(MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({
        retraction.isValid(),
        retraction.encode(),
    });
    callbackOrder.push_back("retraction");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
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
    "Embedded timestamped default-region Send Interaction retracts before TARA and NMRA grants",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-default-region-interaction][default-region][tso][retract]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  unsigned char const parameterBytes[] = {0x52, 0x45, 0x54, 0x2D, 0x44};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x54, 0x2D, 0x41};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.wstring(),
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"default-region-retract-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"default-region-retract-tara", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"default-region-retract-nmra", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto prepareReceiver = [&](auto& rti) {
    auto const region = rti->createRegion(rti1516_2025::DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(rti->setRangeBounds(region, serverId, RangeBounds(8UL, 9UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));
    REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{region}));
    REQUIRE_FALSE(rti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(rti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    drainCallbacks(*rti);
    return region;
  };
  auto const taraRegion = prepareReceiver(tara);
  auto const nmraRegion = prepareReceiver(nmra);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  // Ordinary Send Interaction has no public source RegionHandle. Its private
  // default source overlaps both regional subscriptions and is queued for the
  // two constrained recipients at timestamp 8.
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(retraction.isValid());
  REQUIRE(taraReports.timestampedInteractionReports.empty());
  REQUIRE(nmraReports.timestampedInteractionReports.empty());

  // Keep both alternate requests pending while the publisher establishes GALT
  // 7, then retract before either callback boundary. Grants still complete,
  // but no recipient may observe the retracted interaction.
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));

  drainCallbacks(*publisher);
  drainCallbacks(*tara);
  drainCallbacks(*nmra);
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.back().value == L"5");

  REQUIRE(taraReports.timestampedInteractionReports.empty());
  REQUIRE(taraReports.requestRetractionReports.empty());
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE(nmraReports.timestampedInteractionReports.empty());
  REQUIRE(nmraReports.requestRetractionReports.empty());
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  // NMRA captured the queued message timestamp (8) when its request was
  // accepted. Retraction suppresses delivery but does not rewrite the grant.
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"8");
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(tara->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{taraRegion}));
  REQUIRE_NOTHROW(nmra->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{nmraRegion}));
  REQUIRE_NOTHROW(tara->deleteRegion(taraRegion));
  REQUIRE_NOTHROW(nmra->deleteRegion(nmraRegion));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
