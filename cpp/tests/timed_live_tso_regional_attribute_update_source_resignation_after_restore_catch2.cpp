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
#error "The save/restore regional attribute tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
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
  return L"timed-live-tso-regional-attribute-source-resignation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData tag;
    TransportationTypeHandle transportationType;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
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
    std::wstring implementationName;
    std::wstring value;
  };

  void federationSaved() override {
    ++federationSavedReportCount;
    callbackOrder.push_back("save-complete");
  }

  void federationRestored() override {
    ++federationRestoredReportCount;
    callbackOrder.push_back("restore-complete");
  }

  void initiateFederateSave(std::wstring const& label) override {
    initiateSaveLabels.push_back(label);
    callbackOrder.push_back("initiate-save");
  }

  void initiateFederateSave(
      std::wstring const& label,
      rti1516_2025::LogicalTime const& time) override {
    initiateSaveLabels.push_back(label);
    initiateSaveTimes.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("initiate-save");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantValues.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    reflections.push_back({
        objectInstance,
        attributeValues,
        tag,
        transportationType,
        producer,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrants.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  std::vector<ReflectionReport> reflections;
  std::vector<FlushQueueGrantReport> flushQueueGrants;
  std::vector<TimeReport> timeAdvanceGrantValues;
  std::vector<TimeAdvanceGrantReport> initiateSaveTimes;
  std::vector<std::wstring> initiateSaveLabels;
  std::vector<std::string> callbackOrder;
  std::size_t federationSavedReportCount = 0U;
  std::size_t federationRestoredReportCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[timestamped-regional-attribute-update][explicit-source]"
    "[tso-regional-attribute-update-timed-live-resignation-state]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.federation-saved]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-restored]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador clockReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x53, 0x49, 0x47};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x53, 0x54, 0x4F, 0x52, 0x45};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"live-regional-restore-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"live-regional-restore-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"live-regional-restore-clock", L"publisher", federationName));

  auto const soda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(
      soda,
      fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  auto const receiverSoda = receiver->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const receiverFlavor = receiver->getAttributeHandle(
      receiverSoda,
      fixture_hla::fixture::flavor);
  auto const receiverSodaFlavor = receiver->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(receiverSoda.isValid());
  REQUIRE(receiverFlavor.isValid());
  REQUIRE(receiverSodaFlavor.isValid());
  AttributeHandleSet const publisherAttributes{flavor};
  AttributeHandleSet const receiverAttributes{receiverFlavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      soda,
      publisherAttributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      publisherAttributes,
      TIMESTAMP));

  auto const sourceRegion =
      publisher->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  auto const receiverRegion =
      receiver->createRegion(rti1516_2025::DimensionHandleSet{receiverSodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      receiverSodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publisherAttributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      receiverSoda,
      receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePair));
  REQUIRE(objectInstance.isValid());
  drain(*receiver);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*publisher);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*clock);

  AttributeHandleValueMap values{{flavor, value}};
  auto const update = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(update.isValid());
  REQUIRE(receiverReports.reflections.empty());

  // Reach the timed save boundary without delivering the timestamp-8 passel.
  // The save image must capture that still-live explicit source association
  // and its queued TSO payload.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(
      L"timed-live-regional-source-resignation-save",
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  for (int pass = 0; pass != 4; ++pass) {
    drain(*publisher);
    drain(*receiver);
    drain(*clock);
  }
  auto const saveLabel =
      std::wstring{L"timed-live-regional-source-resignation-save"};
  REQUIRE(receiverReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(publisherReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(clockReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.initiateSaveTimes.size() == 1U);
  REQUIRE(publisherReports.initiateSaveTimes.size() == 1U);
  REQUIRE(clockReports.initiateSaveTimes.size() == 1U);
  REQUIRE(receiverReports.initiateSaveTimes.front().value == L"6");
  REQUIRE(publisherReports.initiateSaveTimes.front().value == L"6");
  REQUIRE(clockReports.initiateSaveTimes.front().value == L"6");
  REQUIRE(publisherReports.timeAdvanceGrantValues.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantValues.size() == 1U);
  REQUIRE(clockReports.timeAdvanceGrantValues.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantValues.front().value == L"6");
  REQUIRE(receiverReports.timeAdvanceGrantValues.front().value == L"6");
  REQUIRE(clockReports.timeAdvanceGrantValues.front().value == L"6");
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(clock->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  REQUIRE_NOTHROW(clock->federateSaveComplete());
  drain(*publisher);
  drain(*receiver);
  drain(*clock);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);
  REQUIRE(clockReports.federationSavedReportCount == 1U);

  // Mutate the live source after the save. Restore must put the saved [0,2)
  // region back before the second mutation. [0,1) is a valid disjoint range
  // for RestaurantFOM's SodaFlavor dimension (whose upper bound is 4).
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  auto const mutatedBeforeRestore =
      publisher->getRangeBounds(sourceRegion, sodaFlavor);
  REQUIRE(mutatedBeforeRestore.getLowerBound() == 0UL);
  REQUIRE(mutatedBeforeRestore.getUpperBound() == 1UL);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*receiver);
  drain(*clock);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  REQUIRE_NOTHROW(clock->federateRestoreComplete());
  drain(*publisher);
  drain(*receiver);
  drain(*clock);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);
  REQUIRE(clockReports.federationRestoredReportCount == 1U);

  auto const restoredBounds = publisher->getRangeBounds(sourceRegion, sodaFlavor);
  REQUIRE(restoredBounds.getLowerBound() == 0UL);
  REQUIRE(restoredBounds.getUpperBound() == 2UL);

  // A second live mutation makes the source region disjoint, then the source
  // resigns. The queued update must use its saved invocation snapshot rather
  // than the current [3,4) region or a region that no longer exists.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  auto const disjointBounds = publisher->getRangeBounds(sourceRegion, sodaFlavor);
  REQUIRE(disjointBounds.getLowerBound() == 3UL);
  REQUIRE(disjointBounds.getUpperBound() == 4UL);
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(receiverReports.reflections.empty());

  // The independent regulator's lookahead advances the federation frontier
  // after the source has left, allowing the surviving constrained receiver to
  // flush its saved regional passel without another clock grant.
  REQUIRE(clockReports.timeAdvanceGrantValues.size() == 1U);
  REQUIRE(clockReports.timeAdvanceGrantValues.front().value == L"6");

  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.reflections.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrants.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  auto const& reflection = receiverReports.reflections.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(receiverFlavor));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(receiverFlavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.producer == publisherHandle);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"8");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  // The independent regulator is at 6 with a lookahead interval of 1, so the
  // restored receiver's federation-wide flush frontier is 7 while the
  // optimistic frontier retains the queued timestamp-8 update.
  REQUIRE(receiverReports.flushQueueGrants.front().value == L"7");
  REQUIRE(receiverReports.flushQueueGrants.front().optimisticValue == L"8");

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      receiverSoda,
      receiverPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
}
