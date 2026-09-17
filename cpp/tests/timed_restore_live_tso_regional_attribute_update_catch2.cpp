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
#error "The timed save/restore explicit-source regional attribute tests require the Umbra source directory."
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
  return L"timed-restore-live-tso-explicit-regional-attribute-" +
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
    VariableLengthData encodedRetraction;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
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
        optionalRetraction == nullptr ? VariableLengthData{}
                                      : optionalRetraction->encode(),
    });
    callbackOrder.push_back("reflect");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractions.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
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
  std::vector<RequestRetractionReport> requestRetractions;
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
    "Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso]"
    "[timestamped-regional-attribute-update][explicit-source]"
    "[timestamped-regional-attribute-timed-restore]"
    "[tso-regional-attribute-update-timed-live-restore-state]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.federation-saved]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-restored]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.update-attribute-values]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x53, 0x54, 0x4F, 0x52, 0x45};
  unsigned char const tagBytes[] = {0x4C, 0x49, 0x56, 0x45, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-tso-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-tso-regional-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(
      soda,
      fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion =
      publisher->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  auto const receiverRegion =
      receiver->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*publisher);

  AttributeHandleValueMap values{{flavor, value}};
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.reflections.empty());

  REQUIRE_NOTHROW(publisher->requestFederationSave(
      L"restore-live-explicit-source-regional-tso",
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  for (int pass = 0; pass != 3; ++pass) {
    drain(*publisher);
    drain(*receiver);
  }
  REQUIRE(receiverReports.initiateSaveLabels ==
          std::vector<std::wstring>{L"restore-live-explicit-source-regional-tso"});
  REQUIRE(publisherReports.timeAdvanceGrants.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrants.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrants.front().value == L"6");
  REQUIRE(receiverReports.timeAdvanceGrants.front().value == L"6");
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  drain(*publisher);
  drain(*receiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  // Terminalize the post-save live designator, then remove the live
  // update-region association. Restore must reconstitute the object/update
  // association, source region, queued passel, and retraction ledger.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      publisherPair));
  REQUIRE(receiverReports.reflections.empty());

  REQUIRE_NOTHROW(publisher->requestFederationRestore(
      L"restore-live-explicit-source-regional-tso"));
  drain(*publisher);
  drain(*receiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  drain(*publisher);
  drain(*receiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.reflections.size() == 1U);
  REQUIRE(receiverReports.flushGrants.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});
  auto const& reflection = receiverReports.reflections.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.producer == publisherHandle);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"8");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(receiverReports.flushGrants.front().value == L"7");
  REQUIRE(receiverReports.flushGrants.front().optimisticValue == L"8");

  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractions.size() == 1U);
  REQUIRE(receiverReports.requestRetractions.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractions.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"request-retraction"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
