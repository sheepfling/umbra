#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped reflected conveyed-region test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::string narrow(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (wchar_t const character : value) {
    result.push_back(character >= 0 && character <= 0x7f
        ? static_cast<char>(character)
        : '?');
  }
  return result;
}

class TimestampedReflectionSnapshotReceiver final : public NullFederateAmbassador {
 public:
  RTIambassador* receiver = nullptr;
  RTIambassador* publisher = nullptr;
  RegionHandle sourceRegion;
  DimensionHandle publisherDimension;
  DimensionHandle receiverDimension;
  std::size_t callbacks = 0U;
  bool timestampedCallback = false;
  std::optional<RegionHandle> conveyedRegion;
  DimensionHandleSet conveyedDimensions;
  std::optional<RangeBounds> boundsBeforeMutation;
  std::optional<RangeBounds> boundsAfterMutation;
  std::exception_ptr queryFailure;
  std::string queryFailureMessage;
  std::exception_ptr mutationFailure;
  std::string mutationFailureMessage;

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions,
      LogicalTime const&,
      OrderType sentOrder,
      OrderType receivedOrder,
      MessageRetractionHandle const*) override {
    ++callbacks;
    timestampedCallback = sentOrder == TIMESTAMP && receivedOrder == TIMESTAMP;
    if (optionalSentRegions == nullptr || optionalSentRegions->size() != 1U) {
      return;
    }

    conveyedRegion = *optionalSentRegions->begin();
    try {
      conveyedDimensions = receiver->getDimensionHandleSet(*conveyedRegion);
      boundsBeforeMutation = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
    } catch (...) {
      queryFailure = std::current_exception();
      try {
        std::rethrow_exception(queryFailure);
      } catch (rti1516_2025::Exception const& error) {
        queryFailureMessage = narrow(error.name()) + ": " + narrow(error.what());
      } catch (std::exception const& error) {
        queryFailureMessage = error.what();
      } catch (...) {
        queryFailureMessage = "non-standard exception during callback region query";
      }
    }
    try {
      publisher->setRangeBounds(
          sourceRegion, publisherDimension, RangeBounds(0UL, 2UL));
      publisher->commitRegionModifications(RegionHandleSet{sourceRegion});
    } catch (...) {
      mutationFailure = std::current_exception();
      try {
        std::rethrow_exception(mutationFailure);
      } catch (rti1516_2025::Exception const& error) {
        mutationFailureMessage = narrow(error.name()) + ": " + narrow(error.what());
      } catch (std::exception const& error) {
        mutationFailureMessage = error.what();
      } catch (...) {
        mutationFailureMessage = "non-standard exception during source region mutation";
      }
    }
    try {
      boundsAfterMutation = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
    } catch (...) {
      queryFailure = std::current_exception();
      try {
        std::rethrow_exception(queryFailure);
      } catch (rti1516_2025::Exception const& error) {
        queryFailureMessage = narrow(error.name()) + ": " + narrow(error.what());
      } catch (std::exception const& error) {
        queryFailureMessage = error.what();
      } catch (...) {
        queryFailureMessage = "non-standard exception during callback region query";
      }
    }
  }
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path regionalFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"ddm-conveyed-region-tso-reflect-snapshot-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
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
    "Embedded 2025 timestamped conveyed region designators remain callback snapshots during Reflect Attribute Values",
    "[integration][development-profile][time-management][ddm]"
    "[ddm-conveyed-region-designator-tso-reflect-snapshot]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.get-range-bounds]"
    "[rti.service.get-dimension-handle-set]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.reflect-attribute-values][callback-evoked]"
    "[rti.service.time-advance-request][2025]") {
  NullFederateAmbassador publisherCallbacks;
  TimestampedReflectionSnapshotReceiver receiverCallbacks;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = regionalFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-tso-reflect-snapshot-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"ddm-tso-reflect-snapshot-receiver", L"receiver", federationName));

  auto const publisherSoda = publisher->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const receiverSoda = receiver->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const publisherFlavor = publisher->getAttributeHandle(
      publisherSoda, L"Flavor");
  auto const receiverFlavor = receiver->getAttributeHandle(
      receiverSoda, L"Flavor");
  auto const publisherDimension = publisher->getDimensionHandle(L"SodaFlavor");
  auto const receiverDimension = receiver->getDimensionHandle(L"SodaFlavor");
  REQUIRE(publisherSoda.isValid());
  REQUIRE(receiverSoda.isValid());
  REQUIRE(publisherFlavor.isValid());
  REQUIRE(receiverFlavor.isValid());
  REQUIRE(publisherDimension.isValid());
  REQUIRE(receiverDimension.isValid());

  AttributeHandleSet const publisherAttributes{publisherFlavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherSoda, publisherAttributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      publisherSoda, publisherAttributes, TIMESTAMP));
  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherDimension});
  auto const subscriptionRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherDimension, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      subscriptionRegion, receiverDimension, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{subscriptionRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      publisherAttributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      AttributeHandleSet{receiverFlavor},
      RegionHandleSet{subscriptionRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      publisherSoda, publisherPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      receiverSoda, receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  drainCallbacks(*receiver);

  receiverCallbacks.receiver = receiver.get();
  receiverCallbacks.publisher = publisher.get();
  receiverCallbacks.sourceRegion = sourceRegion;
  receiverCallbacks.publisherDimension = publisherDimension;
  receiverCallbacks.receiverDimension = receiverDimension;

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(HLAinteger64Interval(0)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  HLAinteger64Time const timestamp(5);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(timestamp));
  drainCallbacks(*receiver);

  unsigned char const valueBytes[]{0x31U, 0x42U};
  AttributeHandleValueMap values;
  values.emplace(
      publisherFlavor,
      VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance, values, VariableLengthData{}, timestamp);
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(timestamp));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);

  REQUIRE(receiverCallbacks.callbacks == 1U);
  REQUIRE(receiverCallbacks.timestampedCallback);
  REQUIRE(receiverCallbacks.conveyedRegion.has_value());
  INFO(receiverCallbacks.queryFailureMessage);
  REQUIRE(receiverCallbacks.queryFailure == nullptr);
  INFO(receiverCallbacks.mutationFailureMessage);
  REQUIRE(receiverCallbacks.mutationFailure == nullptr);
  REQUIRE(receiverCallbacks.conveyedDimensions ==
          DimensionHandleSet{receiverDimension});
  REQUIRE(receiverCallbacks.boundsBeforeMutation.has_value());
  REQUIRE(receiverCallbacks.boundsAfterMutation.has_value());
  REQUIRE(receiverCallbacks.boundsBeforeMutation->getLowerBound() == 1UL);
  REQUIRE(receiverCallbacks.boundsBeforeMutation->getUpperBound() == 3UL);
  REQUIRE(receiverCallbacks.boundsAfterMutation->getLowerBound() == 1UL);
  REQUIRE(receiverCallbacks.boundsAfterMutation->getUpperBound() == 3UL);
  auto const publisherBounds = publisher->getRangeBounds(
      sourceRegion, publisherDimension);
  REQUIRE(publisherBounds.getLowerBound() == 0UL);
  REQUIRE(publisherBounds.getUpperBound() == 2UL);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      receiverSoda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
