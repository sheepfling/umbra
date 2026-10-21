#include "tck_timed_restore_regional_attribute_multi_recipient.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

struct RecipientRegion {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::RegionHandle region;
  rti::AttributeHandleSetRegionHandleSetPairVector pair;
};

rti::RegionHandle createRegion(Session& member, rti::DimensionHandle const& x,
                               rti::DimensionHandle const& y,
                               unsigned long lower, unsigned long upper) {
  auto& ambassador = member.rtiAmbassador();
  auto const region = ambassador.createRegion(rti::DimensionHandleSet{x, y});
  ambassador.setRangeBounds(region, x, rti::RangeBounds(lower, upper));
  ambassador.setRangeBounds(region, y, rti::RangeBounds(lower, upper));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  return region;
}

struct RegionSnapshot {
  bool queried = false;
  std::array<unsigned long, 4> bounds{};
  std::string error;
};

void captureRegionSnapshot(Session& receiver,
                           rti::DimensionHandle const& x,
                           rti::DimensionHandle const& y,
                           RegionSnapshot& observation) {
  receiver.recorder().setTimedReflectionHook(
      [&receiver, &x, &y, &observation](rti::RegionHandleSet const* regions) {
        try {
          require(regions != nullptr && regions->size() == 1U,
                  "Timed reflection did not convey one copied source region");
          auto const region = *regions->begin();
          require(receiver.rtiAmbassador().getDimensionHandleSet(region) ==
                      rti::DimensionHandleSet{x, y},
                  "Copied source region did not retain both DDM dimensions");
          auto const xRange = receiver.rtiAmbassador().getRangeBounds(region, x);
          auto const yRange = receiver.rtiAmbassador().getRangeBounds(region, y);
          observation.bounds = {xRange.getLowerBound(), xRange.getUpperBound(),
                                yRange.getLowerBound(), yRange.getUpperBound()};
          observation.queried = true;
        } catch (std::exception const& error) {
          observation.error = error.what();
        } catch (...) {
          observation.error = "unknown callback region snapshot failure";
        }
      });
}

RecipientRegion subscribe(Session& receiver, Options const& options,
                          unsigned long lower, unsigned long upper) {
  auto& ambassador = receiver.rtiAmbassador();
  auto const objectClass = ambassador.getObjectClassHandle(options.objectClassName);
  auto const attribute = ambassador.getAttributeHandle(objectClass, options.attributeName);
  auto const x = ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const y = ambassador.getDimensionHandle(options.ddmDimensionNames.at(1));
  require(objectClass.isValid() && attribute.isValid() && x.isValid() && y.isValid(),
          "Adapter DDM FOM lacks recipient class, attribute, or dimensions");
  require(ambassador.getDimensionUpperBound(x) >= 9UL &&
              ambassador.getDimensionUpperBound(y) >= 9UL,
          "Adapter DDM FOM dimensions do not support the test ranges");
  auto const region = createRegion(receiver, x, y, lower, upper);
  rti::AttributeHandleSetRegionHandleSetPairVector const pair{{
      rti::AttributeHandleSet{attribute}, rti::RegionHandleSet{region}}};
  ambassador.subscribeObjectClassAttributesWithRegions(objectClass, pair);
  require(!ambassador.getConveyRegionDesignatorSetsSwitch(),
          "Default conveyed-region switch was not false");
  ambassador.setConveyRegionDesignatorSetsSwitch(true);
  return {objectClass, attribute, region, pair};
}

void checkReflected(Session& receiver, RecipientRegion const& handles,
                    rti::ObjectInstanceHandle const& object,
                    rti::FederateHandle const& publisherHandle,
                    rti::MessageRetractionHandle const& saved,
                    RegionSnapshot const& regionSnapshot,
                    rti::LogicalTime const& time8,
                    rti::LogicalTime const& grant7,
                    std::array<std::uint8_t, 5> const& value,
                    std::array<std::uint8_t, 7> const& tag) {
  auto const reflections = receiver.recorder().timedReflections();
  auto const grants = receiver.recorder().flushQueueGrants();
  require(reflections.size() == 1U && grants.size() == 1U,
          "Recipient did not receive exactly one restored reflection and FQG");
  auto const& reflection = reflections.front();
  require(reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(handles.attribute) == 1U &&
              copyBytes(reflection.values.at(handles.attribute)) ==
                  std::vector<std::uint8_t>(value.begin(), value.end()) &&
              reflection.tag == std::vector<std::uint8_t>(tag.begin(), tag.end()),
          "Restored regional reflection changed its object, attribute value, or tag");
  require(reflection.producer == publisherHandle &&
              reflection.time == copyBytes(time8.encode()) &&
              reflection.sentOrder == rti::TIMESTAMP &&
              reflection.receivedOrder == rti::TIMESTAMP,
          "Restored regional reflection changed its producer, timestamp, or order");
  require(reflection.regions.has_value(),
          "Restored regional reflection omitted its region-designator set");
  require(reflection.regions->size() == 1U,
          "Restored regional reflection changed the source-region set size");
  require(regionSnapshot.queried, "Restored source-region snapshot was not queryable in callback: " +
                                      regionSnapshot.error);
  require(regionSnapshot.bounds == std::array<unsigned long, 4>{1UL, 7UL, 1UL, 7UL},
          "Restored region designator did not retain the publisher's saved bounds");
  require(reflection.retractionPresent &&
              reflection.retraction == copyBytes(saved.encode()),
          "Restored regional reflection changed its retraction designator");
  require(grants.front().time == copyBytes(grant7.encode()) &&
              grants.front().optimisticTime == copyBytes(time8.encode()),
          "Restored regional Flush Queue grant changed its time values");
  require(receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "flush-grant"},
          "Restored regional reflection did not precede its Flush Queue grant");
}

void runTimedRestoreRegionalAttributeMultiRecipient(Options const& options,
                                                    rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Timed regional restore has native evidence only for evoked callbacks");
  require(!options.ddmFom.empty() && !options.logicalTimeImplementationName.empty() &&
              options.ddmDimensionNames.size() >= 2U,
          "Timed regional restore requires adapter DDM FOM, time implementation, and dimensions");
  Session publisher(options, model, "owner");
  Session first(options, model, "member-a");
  Session second(options, model, "member-b");
  auto const federation = federationName(options, "timed-restore-regional-attribute-multi");
  std::wstring const saveLabel = L"tck-timed-regional-attribute-multi-boundary";
  std::array<std::uint8_t, 5> const valueBytes{'M', 'U', 'L', 'T', 'I'};
  std::array<std::uint8_t, 7> const tagBytes{'R', 'E', 'S', 'T', 'O', 'R', 'E'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  first.connect();
  second.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.ddmFom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  first.join(options.memberFederateName + L"-a", options.federateType + L"-subscriber", federation);
  second.join(options.memberFederateName + L"-b", options.federateType + L"-subscriber", federation);

  auto& ambassador = publisher.rtiAmbassador();
  auto const publisherHandle = publisher.federateHandle();
  auto const objectClass = ambassador.getObjectClassHandle(options.objectClassName);
  auto const attribute = ambassador.getAttributeHandle(objectClass, options.attributeName);
  auto const x = ambassador.getDimensionHandle(options.ddmDimensionNames[0]);
  auto const y = ambassador.getDimensionHandle(options.ddmDimensionNames[1]);
  require(objectClass.isValid() && attribute.isValid() && x.isValid() && y.isValid(),
          "Adapter DDM FOM lacks publisher class, attribute, or dimensions");
  rti::AttributeHandleSet const attributes{attribute};
  ambassador.publishObjectClassAttributes(objectClass, attributes);
  ambassador.changeDefaultAttributeOrderType(objectClass, attributes, rti::TIMESTAMP);
  auto const sourceRegion = createRegion(publisher, x, y, 1UL, 7UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      attributes, rti::RegionHandleSet{sourceRegion}}};
  auto const firstHandles = subscribe(first, options, 3UL, 8UL);
  auto const secondHandles = subscribe(second, options, 5UL, 9UL);
  auto const object = ambassador.registerObjectInstanceWithRegions(objectClass, sourcePair);
  require(object.isValid(), "Timed regional target object was invalid");
  waitForSessions({&first, &second}, [&] {
    return first.recorder().hasDiscovery(object) && second.recorder().hasDiscovery(object);
  }, options, "regional target discovery at both subscribers");

  first.rtiAmbassador().enableTimeConstrained();
  second.rtiAmbassador().enableTimeConstrained();
  first.pump();
  second.pump();
  auto const time = makeTimeContext(publisher);
  auto const time6 = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const time7 = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const time8 = timeAfter(*time.factory, *time.initial, *time.epsilon, 8U);
  auto const time10 = timeAfter(*time.factory, *time.initial, *time.epsilon, 10U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  *lookahead += *time.epsilon;
  ambassador.enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  rti::AttributeHandleValueMap values;
  values.emplace(attribute, rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const saved = ambassador.updateAttributeValues(object, values, tag, *time8);
  require(saved.isValid() && first.recorder().timedReflections().empty() &&
              second.recorder().timedReflections().empty(),
          "Timestamp-eight regional update was not queued for both recipients");
  ambassador.requestFederationSave(saveLabel, *time6);
  first.rtiAmbassador().timeAdvanceRequest(*time6);
  second.rtiAmbassador().timeAdvanceRequest(*time6);
  ambassador.timeAdvanceRequest(*time6);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           publisher.recorder().timeAdvanceGrants().size() == 1U &&
           first.recorder().timeAdvanceGrants().size() == 1U &&
           second.recorder().timeAdvanceGrants().size() == 1U;
  }, options, "timed save initiation and time-six grants");
  for (Session* member : {&publisher, &first, &second}) {
    require(member->recorder().timeAdvanceGrants().front().encoded == copyBytes(time6->encode()),
            "Federate did not reach the timed save boundary");
    member->rtiAmbassador().federateSaveBegun();
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationSavedCount() == 1U && first.federationSavedCount() == 1U &&
           second.federationSavedCount() == 1U;
  }, options, "timed regional federation saved");
  require(first.recorder().timedReflections().empty() &&
              second.recorder().timedReflections().empty(),
          "Queued regional update arrived at the save boundary");

  ambassador.retract(saved);
  first.pump();
  second.pump();
  requireException([&] { ambassador.retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save regional update handle");
  ambassador.unassociateRegionsForUpdates(object, sourcePair);
  ambassador.requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           first.federateRestoreInitiations().size() == 1U &&
           second.federateRestoreInitiations().size() == 1U;
  }, options, "timed regional restore initiation");
  for (Session* member : {&publisher, &first, &second}) {
    member->rtiAmbassador().federateRestoreComplete();
  }
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationRestoredCount() == 1U && first.federationRestoredCount() == 1U &&
           second.federationRestoredCount() == 1U;
  }, options, "timed regional federation restored");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  RegionSnapshot firstRegionSnapshot;
  RegionSnapshot secondRegionSnapshot;
  captureRegionSnapshot(first, x, y, firstRegionSnapshot);
  captureRegionSnapshot(second, x, y, secondRegionSnapshot);
  first.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&first}, [&] {
    return first.recorder().timedReflections().size() == 1U &&
           first.recorder().flushQueueGrants().size() == 1U;
  }, options, "first restored regional reflection and FQG");
  require(second.recorder().timedReflections().empty() &&
              second.recorder().flushQueueGrants().empty(),
          "First FQR prematurely released the second regional reflection");
  second.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&second}, [&] {
    return second.recorder().timedReflections().size() == 1U &&
           second.recorder().flushQueueGrants().size() == 1U;
  }, options, "second restored regional reflection and FQG");
  checkReflected(first, firstHandles, object, publisherHandle, saved, firstRegionSnapshot,
                 *time8, *time7, valueBytes, tagBytes);
  checkReflected(second, secondHandles, object, publisherHandle, saved, secondRegionSnapshot,
                 *time8, *time7, valueBytes, tagBytes);
  first.recorder().setTimedReflectionHook({});
  second.recorder().setTimedReflectionHook({});

  ambassador.retract(saved);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().retractions().size() == 1U &&
           second.recorder().retractions().size() == 1U;
  }, options, "restored regional request-retraction callbacks");
  for (Session* member : {&first, &second}) {
    auto const& retraction = member->recorder().retractions().front();
    require(retraction.valid && retraction.encoded == copyBytes(saved.encode()),
            "Restored regional retraction returned a different designator");
  }
  requireException([&] { ambassador.retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored regional update handle");
  ambassador.unassociateRegionsForUpdates(object, sourcePair);

  first.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      firstHandles.objectClass, firstHandles.pair);
  second.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      secondHandles.objectClass, secondHandles.pair);
  first.rtiAmbassador().deleteRegion(firstHandles.region);
  second.rtiAmbassador().deleteRegion(secondHandles.region);
  ambassador.deleteRegion(sourceRegion);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioTimedRestoreRegionalAttributeMultiRecipient(Options const& options,
                                                         rti::CallbackModel model) {
  runTimedRestoreRegionalAttributeMultiRecipient(options, model);
}

void scenarioTimedRestoreRegionalAttributeMultiRecipientContract(
    Options const& options, rti::CallbackModel model) {
  runTimedRestoreRegionalAttributeMultiRecipient(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
