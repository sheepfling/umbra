#include "tck_main_scenarios.hpp"

#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

struct ReflectionRegionSnapshotObservation final {
  std::mutex mutex;
  std::size_t callbackCount = 0U;
  bool complete = false;
  std::optional<rti::RegionHandle> conveyedRegion;
  std::optional<rti::DimensionHandleSet> dimensions;
  std::optional<rti::RangeBounds> boundsBeforeMutation;
  std::optional<rti::RangeBounds> boundsAfterMutation;
  std::exception_ptr failure;
  std::string failureStage;
};

void rethrowSnapshotFailure(
    std::exception_ptr const& failure,
    std::string const& stage) {
  if (!failure) {
    return;
  }
  try {
    std::rethrow_exception(failure);
  } catch (rti::Exception const& error) {
    throw std::runtime_error(
        "callback-time region operation failed during " + stage + ": " +
        toNarrow(error.name()) + ": " + toNarrow(error.what()));
  } catch (std::exception const& error) {
    throw std::runtime_error(
        "callback-time region operation failed during " + stage + ": " +
        error.what());
  } catch (...) {
    throw std::runtime_error(
        "callback-time region operation failed during " + stage +
        " with a non-standard exception");
  }
}

}  // namespace

void scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshot(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Callback-scoped region mutation is exercised with the adapter-selected HLA_EVOKED model");
  require(
      !options.multiAttributeFom.empty(),
      "Timestamped region-designator snapshot requires an adapter-supplied DDM FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped region-designator snapshot requires an adapter-supplied logical-time implementation");
  require(
      !options.ddmDimensionNames.empty(),
      "Timestamped region-designator snapshot requires an adapter-supplied DDM dimension");

  Session publisher(options, model, "region-snapshot-publisher");
  Session receiver(options, model, "region-snapshot-receiver");
  auto const federation = federationName(
      options,
      "timestamped-attribute-reflect-region-designator-callback-snapshot");
  connectAndJoin(
      publisher,
      receiver,
      options,
      federation,
      options.multiAttributeFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.multiAttributeFirstName);
  auto const publisherDimension = publisher.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(0));
  auto const receiverDimension = receiver.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(0));
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherDimension.isValid() && receiverDimension.isValid(),
      "Adapter DDM FOM did not provide the snapshot class, attribute, and dimension");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const sourceRegion = publisher.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{publisherDimension});
  auto const subscriptionRegion = receiver.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{receiverDimension});
  require(
      sourceRegion.isValid() && subscriptionRegion.isValid(),
      "Region creation returned an invalid handle");
  publisher.rtiAmbassador().setRangeBounds(
      sourceRegion,
      publisherDimension,
      rti::RangeBounds(1UL, 3UL));
  publisher.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{sourceRegion});
  receiver.rtiAmbassador().setRangeBounds(
      subscriptionRegion,
      receiverDimension,
      rti::RangeBounds(1UL, 3UL));
  receiver.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{subscriptionRegion});

  rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      publisherAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      rti::RegionHandleSet{subscriptionRegion},
  }};
  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherClass,
      publisherPair);
  require(
      object.isValid(),
      "Regional object registration returned an invalid handle");
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverClass,
      receiverPair);
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "regional object discovery");

  ReflectionRegionSnapshotObservation observation;
  receiver.recorder().setTimedReflectionHook(
      [&](rti::RegionHandleSet const* conveyedRegions) {
        std::lock_guard<std::mutex> lock(observation.mutex);
        ++observation.callbackCount;
        std::string stage = "validate callback region arguments";
        try {
          require(
              conveyedRegions != nullptr && conveyedRegions->size() == 1U,
              "Timed reflection did not convey exactly one region designator");
          auto const conveyedRegion = *conveyedRegions->begin();
          observation.conveyedRegion = conveyedRegion;
          stage = "query conveyed dimensions";
          observation.dimensions = receiver.rtiAmbassador().getDimensionHandleSet(
              conveyedRegion);
          stage = "query conveyed bounds before source mutation";
          observation.boundsBeforeMutation = receiver.rtiAmbassador().getRangeBounds(
              conveyedRegion,
              receiverDimension);

          stage = "mutate and commit publisher source region";
          publisher.rtiAmbassador().setRangeBounds(
              sourceRegion,
              publisherDimension,
              rti::RangeBounds(0UL, 2UL));
          publisher.rtiAmbassador().commitRegionModifications(
              rti::RegionHandleSet{sourceRegion});

          stage = "query conveyed bounds after source mutation";
          observation.boundsAfterMutation = receiver.rtiAmbassador().getRangeBounds(
              conveyedRegion,
              receiverDimension);
        } catch (...) {
          observation.failure = std::current_exception();
          observation.failureStage = stage;
        }
        observation.complete = true;
      });

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName() &&
          encodeTime(*publisherTime.initial) == encodeTime(*receiverTime.initial),
      "Timestamped region-snapshot members selected different initial time contexts");
  // This scenario isolates callback-scoped region snapshot behavior. Keep a
  // positive, minimal lookahead so enabling time-constrained reception does
  // not also exercise the separate zero-lookahead GALT boundary.
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return !publisher.recorder().timeRegulationEnabled().empty(); },
      options,
      "publisher time-regulation callback");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return !receiver.recorder().timeConstrainedEnabled().empty(); },
      options,
      "receiver time-constrained callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  receiver.rtiAmbassador().timeAdvanceRequest(*timestamp);

  std::vector<std::uint8_t> const valueBytes{0x31U, 0x42U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x53U, 0x4EU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped regional update returned an invalid retraction handle");
  publisher.rtiAmbassador().timeAdvanceRequest(*timestamp);
  waitFor(
      publisher,
      [&] { return !publisher.recorder().timeAdvanceGrants().empty(); },
      options,
      "publisher time advance after timestamped update");
  waitFor(
      receiver,
      [&] {
        std::lock_guard<std::mutex> lock(observation.mutex);
        return observation.complete;
      },
      options,
      "callback-time conveyed-region query and source-region mutation");
  waitFor(
      receiver,
      [&] { return !receiver.recorder().timeAdvanceGrants().empty(); },
      options,
      "receiver grant after timestamped reflection");

  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U,
      "Timestamped regional update did not produce exactly one reflection");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.producer == publisher.federateHandle() &&
          reflection.time == encodeTime(*timestamp) &&
          !reflection.timeText.empty() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()) &&
          reflection.regions.has_value() && reflection.regions->size() == 1U,
      "Timestamped reflection returned incorrect value, time, order, or designator metadata");

  std::optional<rti::RegionHandle> conveyedRegion;
  std::optional<rti::DimensionHandleSet> conveyedDimensions;
  std::optional<rti::RangeBounds> boundsBeforeMutation;
  std::optional<rti::RangeBounds> boundsAfterMutation;
  std::exception_ptr callbackFailure;
  std::string callbackFailureStage;
  std::size_t callbackCount = 0U;
  {
    std::lock_guard<std::mutex> lock(observation.mutex);
    callbackCount = observation.callbackCount;
    conveyedRegion = observation.conveyedRegion;
    conveyedDimensions = observation.dimensions;
    boundsBeforeMutation = observation.boundsBeforeMutation;
    boundsAfterMutation = observation.boundsAfterMutation;
    callbackFailure = observation.failure;
    callbackFailureStage = observation.failureStage;
  }
  rethrowSnapshotFailure(callbackFailure, callbackFailureStage);
  require(
      callbackCount == 1U && conveyedRegion.has_value() &&
          conveyedDimensions.has_value() && boundsBeforeMutation.has_value() &&
          boundsAfterMutation.has_value(),
      "Callback did not complete one queryable conveyed-region snapshot");
  require(
      reflection.regions->contains(*conveyedRegion) &&
          *conveyedDimensions == rti::DimensionHandleSet{receiverDimension},
      "Callback region designator did not resolve to the expected dimension");
  require(
      boundsBeforeMutation->getLowerBound() == 1UL &&
          boundsBeforeMutation->getUpperBound() == 3UL &&
          boundsAfterMutation->getLowerBound() == 1UL &&
          boundsAfterMutation->getUpperBound() == 3UL,
      "Conveyed region designator changed after its source region was modified");
  auto const publisherBounds = publisher.rtiAmbassador().getRangeBounds(
      sourceRegion,
      publisherDimension);
  require(
      publisherBounds.getLowerBound() == 0UL &&
          publisherBounds.getUpperBound() == 2UL,
      "Publisher source region did not retain its committed post-send bounds");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverClass,
      receiverPair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(
      object,
      publisherPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().deleteRegion(subscriptionRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshotContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshot(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
