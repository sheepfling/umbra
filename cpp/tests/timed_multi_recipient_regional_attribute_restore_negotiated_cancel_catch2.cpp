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
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timed regional save/restore test requires the Umbra source directory."
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
  return L"timed-multi-recipient-regional-negotiated-cancel-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
  };

  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct DivestitureConfirmationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back({objectInstance});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
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
      VariableLengthData const&,
      FederateHandle const&) override {
    objectRemovalReports.push_back({objectInstance});
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

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void initiateFederateSave(std::wstring const& label) override {
    initiateFederateSaveReports.push_back(label);
  }

  void initiateFederateSave(
      std::wstring const& label,
      rti1516_2025::LogicalTime const&) override {
    initiateFederateSaveReports.push_back(label);
  }

  void federationSaved() override {
    ++federationSavedReportCount;
  }

  void federationRestored() override {
    ++federationRestoredReportCount;
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::unavailable,
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipReleaseRequestReports.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  void requestDivestitureConfirmation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& releasedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    divestitureConfirmationReports.push_back({
        objectInstance,
        releasedAttributes,
        userSuppliedTag,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<ReflectionReport> attributeReflectionReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<TimeReport> timeAdvanceGrantReports;
  std::vector<std::wstring> initiateFederateSaveReports;
  std::size_t federationSavedReportCount = 0U;
  std::size_t federationRestoredReportCount = 0U;
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipReleaseRequestReport>
      attributeOwnershipReleaseRequestReports;
  std::vector<DivestitureConfirmationReport> divestitureConfirmationReports;
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

void runTimedMultiRecipientRegionalNegotiatedCancel(
    bool cancelBeforeDelivery) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  ReportingFederateAmbassador clockReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  unsigned char const valueBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x4D};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"timed-multi-recipient-regional-resignation";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timed-multi-regional-resignation-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"timed-multi-regional-resignation-first",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"timed-multi-regional-resignation-second",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"timed-multi-regional-resignation-clock",
      L"publisher",
      federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const privilegeToDelete = publisher->getAttributeHandle(
      soda,
      standard_hla::mom::privilege_to_delete_object);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));
  REQUIRE_NOTHROW(clock->subscribeObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};

  auto prepareReceiver = [&](RTIambassador& receiver) {
    auto const region = receiver.createRegion(
        rti1516_2025::DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(receiver.setRangeBounds(
        region,
        sodaFlavor,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(receiver.commitRegionModifications(RegionHandleSet{region}));
    AttributeHandleSetRegionHandleSetPairVector const pair{{
        flavorOnly,
        RegionHandleSet{region},
    }};
    REQUIRE_NOTHROW(receiver.subscribeObjectClassAttributesWithRegions(soda, pair));
    REQUIRE_NOTHROW(receiver.setConveyRegionDesignatorSetsSwitch(true));
    return std::pair{region, pair};
  };
  auto const [firstReceiverRegion, firstReceiverPair] = prepareReceiver(*firstReceiver);
  auto const [secondReceiverRegion, secondReceiverPair] = prepareReceiver(*secondReceiver);

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);
  REQUIRE(firstReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  drainCallbacks(*firstReceiver);
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  drainCallbacks(*secondReceiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*clock);

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      flavor,
      VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.empty());

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
  for (int pass = 0; pass != 3; ++pass) {
    drainCallbacks(*publisher);
    drainCallbacks(*firstReceiver);
    drainCallbacks(*secondReceiver);
    drainCallbacks(*clock);
  }
  REQUIRE(firstReceiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReceiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(publisherReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(clockReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(clock->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(clock->federateSaveComplete());
  drainCallbacks(*publisher);
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);
  drainCallbacks(*clock);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(clockReports.federationSavedReportCount == 1U);

  // Mutate the source region outside the saved image. Restore must recover the
  // original source realization before the resignation/ownership matrix runs.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drainCallbacks(*publisher);
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);
  drainCallbacks(*clock);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(clock->federateRestoreComplete());
  drainCallbacks(*publisher);
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);
  drainCallbacks(*clock);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(clockReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);

  // After restore, publish a real regular acquisition for the first receiver
  // and retain an If Available candidate on the independent clock. The owner
  // negotiates toward the first candidate, then replans to the retained clock
  // candidate when the first requester resigns with directive three.
  REQUIRE_NOTHROW(firstReceiver->publishObjectClassAttributes(soda, flavorOnly));
  unsigned char const acquisitionTagBytes[] = {0x43, 0x41, 0x4E, 0x2D, 0x54};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(firstReceiver->attributeOwnershipAcquisition(
      objectInstance,
      flavorOnly,
      acquisitionTag));
  REQUIRE(firstReceiverReports.attributeOwnershipAcquisitionReports.empty());

  drainCallbacks(*clock);
  REQUIRE(clockReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(clock->publishObjectClassAttributes(soda, flavorOnly));
  unsigned char const secondWillingTagBytes[] = {0x53, 0x45, 0x43, 0x2D, 0x54};
  VariableLengthData const secondWillingTag(
      secondWillingTagBytes,
      sizeof(secondWillingTagBytes));
  REQUIRE_NOTHROW(clock->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      flavorOnly,
      secondWillingTag));
  REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());

  unsigned char const negotiatedTagBytes[] = {0x4E, 0x45, 0x47, 0x2D, 0x54};
  VariableLengthData const negotiatedTag(
      negotiatedTagBytes,
      sizeof(negotiatedTagBytes));
  REQUIRE_NOTHROW(publisher->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      flavorOnly,
      negotiatedTag));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
  REQUIRE_FALSE(firstReceiver->isAttributeOwnedByFederate(objectInstance, flavor));
  REQUIRE(publisherReports.divestitureConfirmationReports.empty());

  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  drainCallbacks(*firstReceiver);
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(firstReceiverReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(publisherReports.divestitureConfirmationReports.empty());

  // Replanning selects the retained If Available candidate and queues its
  // confirmation. The companion pre-delivery lane cancels this state before
  // the callback enters user code; the baseline lane cancels after TSO flush.
  REQUIRE_NOTHROW(publisher->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      flavorOnly,
      negotiatedTag));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
  if (cancelBeforeDelivery) {
    REQUIRE_NOTHROW(publisher->cancelNegotiatedAttributeOwnershipDivestiture(
        objectInstance,
        flavorOnly));
    REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
    REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
    // The unavailable callback may be queued until the next clock dispatch;
    // whichever boundary observes it, it must never be an acquisition.
    for (auto const& report : clockReports.attributeOwnershipAcquisitionReports) {
      REQUIRE(report.kind ==
              ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
      REQUIRE(report.objectInstance == objectInstance);
      REQUIRE(report.attributes == flavorOnly);
    }
    REQUIRE_THROWS_AS(
        publisher->confirmDivestiture(
            objectInstance,
            flavorOnly,
            VariableLengthData{}),
        rti1516_2025::AttributeDivestitureWasNotRequested);
    drainCallbacks(*publisher);
    REQUIRE(publisherReports.divestitureConfirmationReports.empty());
  } else {
    drainCallbacks(*publisher);
    REQUIRE(publisherReports.divestitureConfirmationReports.size() == 1U);
    auto const& confirmation = publisherReports.divestitureConfirmationReports.front();
    REQUIRE(confirmation.objectInstance == objectInstance);
    REQUIRE(confirmation.attributes == flavorOnly);
    REQUIRE(variableLengthDataBytes(confirmation.userSuppliedTag) ==
            std::vector<unsigned char>(
                secondWillingTagBytes,
                secondWillingTagBytes + sizeof(secondWillingTagBytes)));
    REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
    REQUIRE(clockReports.attributeOwnershipAcquisitionReports.empty());
  }
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(9)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(9)));
  drainCallbacks(*clock);
  drainCallbacks(*firstReceiver);
  drainCallbacks(*secondReceiver);
  // A focused observer may retain the clock's pre-save grant as well as the
  // post-restore advance. The final grant boundary is the evidence relevant
  // to this lane, so assert its value rather than coupling to that history.
  REQUIRE_FALSE(clockReports.timeAdvanceGrantReports.empty());
  REQUIRE(clockReports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE(firstReceiverReports.objectRemovalReports.empty());
  REQUIRE(firstReceiverReports.attributeReflectionReports.empty());
  REQUIRE(secondReceiverReports.objectRemovalReports.empty());
  REQUIRE(secondReceiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(secondReceiverReports.attributeReflectionReports.front().objectInstance == objectInstance);
  REQUIRE(secondReceiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(secondReceiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "flush-grant"});

  if (cancelBeforeDelivery) {
    // The negotiated confirmation was already cancelled before it entered
    // user code; no second cancellation or callback is possible here.
    REQUIRE(publisherReports.divestitureConfirmationReports.empty());
    REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
    REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
    for (auto const& report : clockReports.attributeOwnershipAcquisitionReports) {
      REQUIRE(report.kind ==
              ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
      REQUIRE(report.objectInstance == objectInstance);
      REQUIRE(report.attributes == flavorOnly);
    }
  } else {
    REQUIRE_NOTHROW(publisher->cancelNegotiatedAttributeOwnershipDivestiture(
        objectInstance,
        flavorOnly));
    REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, flavor));
    REQUIRE_FALSE(clock->isAttributeOwnedByFederate(objectInstance, flavor));
    // Cancelling the owner's negotiated handoff leaves the retained If
    // Available candidate unowned. If its terminal callback has already been
    // dispatched, it is the standard unavailable outcome, never an acquisition.
    for (auto const& report : clockReports.attributeOwnershipAcquisitionReports) {
      REQUIRE(report.kind ==
              ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
      REQUIRE(report.objectInstance == objectInstance);
      REQUIRE(report.attributes == flavorOnly);
    }
    REQUIRE_THROWS_AS(
        publisher->confirmDivestiture(
            objectInstance,
            flavorOnly,
            VariableLengthData{}),
        rti1516_2025::AttributeDivestitureWasNotRequested);
    drainCallbacks(*publisher);
    REQUIRE(publisherReports.divestitureConfirmationReports.size() == 1U);
  }

  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      secondReceiverPair));
  REQUIRE_NOTHROW(secondReceiver->deleteRegion(secondReceiverRegion));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained negotiated owner confirmation after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-pending-ownership-acquisitions][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[negotiated-willing-to-acquire-continuation][willing-to-acquire]"
    "[timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-state]"
    "[tso-regional-attribute-update-timed-negotiated-continuation-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-candidate-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-confirmation-cancel-state]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  runTimedMultiRecipientRegionalNegotiatedCancel(false);
}

TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained negotiated owner confirmation before delivery after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-pending-ownership-acquisitions][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[negotiated-willing-to-acquire-continuation][willing-to-acquire]"
    "[timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-state]"
    "[tso-regional-attribute-update-timed-negotiated-continuation-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-candidate-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-confirmation-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-mixed-pre-delivery-cancel-state]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  runTimedMultiRecipientRegionalNegotiatedCancel(true);
}
