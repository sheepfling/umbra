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
#error "The timed save/restore cancel-then-delete-then-divest test requires the Umbra source directory."
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
  return L"timed-live-tso-regional-attribute-multi-recipient-cancel-then-delete-then-divest-" +
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

  struct RemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
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
    timeAdvanceGrants.push_back({time.implementationName(), time.toString()});
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

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer) override {
    removals.push_back({objectInstance, tag, producer});
    callbackOrder.push_back("remove");
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    removals.push_back({
        objectInstance,
        tag,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushGrants.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  std::vector<ReflectionReport> reflections;
  std::vector<RemovalReport> removals;
  std::vector<FlushQueueGrantReport> flushGrants;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrants;
  std::vector<std::string> callbackOrder;
  std::vector<std::wstring> initiateSaveLabels;
  std::vector<TimeAdvanceGrantReport> initiateSaveTimes;
  std::size_t federationSavedReportCount = 0U;
  std::size_t federationRestoredReportCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-then-delete-then-divest][timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.get-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.federation-saved]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-restored]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.flush-queue-grant][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  ReportingFederateAmbassador clockReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  unsigned char const valueBytes[] = {0x44, 0x45, 0x4C, 0x45, 0x54, 0x45};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x53, 0x54, 0x4F, 0x52, 0x45};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel =
      L"restore-live-explicit-source-regional-cancel-then-delete-then-divest";
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timed-live-regional-cancel-delete-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"timed-live-regional-cancel-delete-first",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"timed-live-regional-cancel-delete-second",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"timed-live-regional-cancel-delete-clock",
      L"publisher",
      federationName));

  auto const soda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(
      soda,
      fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  auto const firstReceiverSoda = firstReceiver->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const firstReceiverFlavor = firstReceiver->getAttributeHandle(
      firstReceiverSoda,
      fixture_hla::fixture::flavor);
  auto const firstReceiverSodaFlavor = firstReceiver->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  auto const secondReceiverSoda = secondReceiver->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const secondReceiverFlavor = secondReceiver->getAttributeHandle(
      secondReceiverSoda,
      fixture_hla::fixture::flavor);
  auto const secondReceiverSodaFlavor = secondReceiver->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(firstReceiverSoda.isValid());
  REQUIRE(firstReceiverFlavor.isValid());
  REQUIRE(firstReceiverSodaFlavor.isValid());
  REQUIRE(secondReceiverSoda.isValid());
  REQUIRE(secondReceiverFlavor.isValid());
  REQUIRE(secondReceiverSodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  AttributeHandleSet const firstReceiverFlavorOnly{firstReceiverFlavor};
  AttributeHandleSet const secondReceiverFlavorOnly{secondReceiverFlavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion =
      publisher->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  auto const firstReceiverRegion = firstReceiver->createRegion(
      rti1516_2025::DimensionHandleSet{firstReceiverSodaFlavor});
  auto const secondReceiverRegion = secondReceiver->createRegion(
      rti1516_2025::DimensionHandleSet{secondReceiverSodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(firstReceiver->setRangeBounds(
      firstReceiverRegion,
      firstReceiverSodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(secondReceiver->setRangeBounds(
      secondReceiverRegion,
      secondReceiverSodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(firstReceiver->commitRegionModifications(
      RegionHandleSet{firstReceiverRegion}));
  REQUIRE_NOTHROW(secondReceiver->commitRegionModifications(
      RegionHandleSet{secondReceiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const firstReceiverPair{{
      firstReceiverFlavorOnly,
      RegionHandleSet{firstReceiverRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const secondReceiverPair{{
      secondReceiverFlavorOnly,
      RegionHandleSet{secondReceiverRegion},
  }};
  REQUIRE_NOTHROW(firstReceiver->subscribeObjectClassAttributesWithRegions(
      firstReceiverSoda,
      firstReceiverPair));
  REQUIRE_NOTHROW(secondReceiver->subscribeObjectClassAttributesWithRegions(
      secondReceiverSoda,
      secondReceiverPair));
  REQUIRE_NOTHROW(firstReceiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(secondReceiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE(objectInstance.isValid());
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));

  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
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
  REQUIRE(firstReceiverReports.reflections.empty());
  REQUIRE(secondReceiverReports.reflections.empty());

  REQUIRE_NOTHROW(publisher->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  for (int pass = 0; pass != 4; ++pass) {
    drain(*publisher);
    drain(*firstReceiver);
    drain(*secondReceiver);
    drain(*clock);
  }
  REQUIRE(publisherReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReceiverReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReceiverReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(clockReports.initiateSaveLabels ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(publisherReports.initiateSaveTimes.front().value == L"6");
  REQUIRE(firstReceiverReports.initiateSaveTimes.front().value == L"6");
  REQUIRE(secondReceiverReports.initiateSaveTimes.front().value == L"6");
  REQUIRE(clockReports.initiateSaveTimes.front().value == L"6");
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(clock->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(clock->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  drain(*clock);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(clockReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.reflections.empty());
  REQUIRE(secondReceiverReports.reflections.empty());

  // The first mutation is outside the saved image. Restore must recover the
  // original [0,2) source region before the disjoint post-restore mutation.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  auto const mutatedBeforeRestore =
      publisher->getRangeBounds(publisherRegion, sodaFlavor);
  REQUIRE(mutatedBeforeRestore.getLowerBound() == 0UL);
  REQUIRE(mutatedBeforeRestore.getUpperBound() == 1UL);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  drain(*clock);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(clock->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  drain(*clock);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(clockReports.federationRestoredReportCount == 1U);
  auto const restoredBounds =
      publisher->getRangeBounds(publisherRegion, sodaFlavor);
  REQUIRE(restoredBounds.getLowerBound() == 0UL);
  REQUIRE(restoredBounds.getUpperBound() == 2UL);

  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  auto const disjointBounds =
      publisher->getRangeBounds(publisherRegion, sodaFlavor);
  REQUIRE(disjointBounds.getLowerBound() == 3UL);
  REQUIRE(disjointBounds.getUpperBound() == 4UL);

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE(firstReceiverReports.reflections.empty());
  REQUIRE(secondReceiverReports.reflections.empty());
  REQUIRE(firstReceiverReports.removals.empty());
  REQUIRE(secondReceiverReports.removals.empty());

  // CANCEL_THEN_DELETE_THEN_DIVEST is a receive-order removal at each
  // recipient's
  // delivery boundary. The queued timestamp-8 reflection is stale and must
  // never precede or follow the removal.
  REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  while (firstReceiver->evokeCallback(0.0)) {
  }
  REQUIRE(firstReceiverReports.reflections.empty());
  REQUIRE(firstReceiverReports.removals.size() == 1U);
  REQUIRE(firstReceiverReports.flushGrants.size() == 1U);
  REQUIRE(firstReceiverReports.callbackOrder ==
          std::vector<std::string>{"remove", "flush-grant"});
  REQUIRE(firstReceiverReports.removals.front().objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(firstReceiverReports.removals.front().tag).empty());
  REQUIRE(firstReceiverReports.removals.front().producer == publisherHandle);
  REQUIRE_THROWS_AS(
      firstReceiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  while (secondReceiver->evokeCallback(0.0)) {
  }
  REQUIRE(secondReceiverReports.reflections.empty());
  REQUIRE(secondReceiverReports.removals.size() == 1U);
  REQUIRE(secondReceiverReports.flushGrants.size() == 1U);
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"remove", "flush-grant"});
  REQUIRE(secondReceiverReports.removals.front().objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(secondReceiverReports.removals.front().tag).empty());
  REQUIRE(secondReceiverReports.removals.front().producer == publisherHandle);
  REQUIRE_THROWS_AS(
      secondReceiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(firstReceiver->unsubscribeObjectClassAttributesWithRegions(
      firstReceiverSoda,
      firstReceiverPair));
  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributesWithRegions(
      secondReceiverSoda,
      secondReceiverPair));
  REQUIRE_NOTHROW(firstReceiver->deleteRegion(firstReceiverRegion));
  REQUIRE_NOTHROW(secondReceiver->deleteRegion(secondReceiverRegion));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(firstReceiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
