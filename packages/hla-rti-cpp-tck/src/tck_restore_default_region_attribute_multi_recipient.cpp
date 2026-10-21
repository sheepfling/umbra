#include "tck_restore_default_region_attribute_multi_recipient.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

struct RecipientRegion {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::RegionHandle region;
  rti::AttributeHandleSetRegionHandleSetPairVector pair;
};

RecipientRegion subscribe(Session& receiver, Options const& options) {
  auto& rti = receiver.rtiAmbassador();
  auto const objectClass = rti.getObjectClassHandle(options.objectClassName);
  auto const attribute = rti.getAttributeHandle(objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter DDM FOM lacks recipient object class or attribute");
  require(options.ddmDimensionNames.size() >= 2U,
          "Adapter must provide two dimensions for regional subscriptions");
  auto const first = rti.getDimensionHandle(options.ddmDimensionNames[0]);
  auto const second = rti.getDimensionHandle(options.ddmDimensionNames[1]);
  require(first.isValid() && second.isValid(),
          "Adapter DDM FOM lacks the configured dimensions");
  auto const region = rti.createRegion(rti::DimensionHandleSet{first, second});
  rti.setRangeBounds(region, first, rti::RangeBounds(2UL, 3UL));
  rti.setRangeBounds(region, second, rti::RangeBounds(2UL, 3UL));
  rti.commitRegionModifications(rti::RegionHandleSet{region});
  rti::AttributeHandleSetRegionHandleSetPairVector const pair{{
      rti::AttributeHandleSet{attribute}, rti::RegionHandleSet{region}}};
  rti.subscribeObjectClassAttributesWithRegions(objectClass, pair);
  require(!rti.getConveyRegionDesignatorSetsSwitch(),
          "Default conveyed-region switch was not false");
  rti.setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().enableTimeConstrained();
  receiver.pump();
  return {objectClass, attribute, region, pair};
}

void checkReflected(Session& receiver, RecipientRegion const& handles,
                    rti::ObjectInstanceHandle const& object,
                    rti::FederateHandle const& publisherHandle,
                    rti::MessageRetractionHandle const& saved,
                    rti::LogicalTime const& time7, rti::LogicalTime const& time5,
                    std::array<std::uint8_t, 7> const& valueBytes,
                    std::array<std::uint8_t, 7> const& tagBytes) {
  auto const reflections = receiver.recorder().timedReflections();
  auto const grants = receiver.recorder().flushQueueGrants();
  require(reflections.size() == 1U && grants.size() == 1U,
          "Recipient did not receive exactly one restored default-region reflection and FQG");
  auto const& reflection = reflections.front();
  require(reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(handles.attribute) == 1U &&
              copyBytes(reflection.values.at(handles.attribute)) ==
                  std::vector<std::uint8_t>(valueBytes.begin(), valueBytes.end()) &&
              reflection.tag ==
                  std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
              reflection.producer == publisherHandle &&
              reflection.time == copyBytes(time7.encode()) &&
              reflection.sentOrder == rti::TIMESTAMP &&
              reflection.receivedOrder == rti::TIMESTAMP &&
              reflection.regions.has_value() && reflection.regions->empty() &&
              reflection.retractionPresent &&
              reflection.retraction == copyBytes(saved.encode()) &&
              grants.front().time == copyBytes(time5.encode()) &&
              grants.front().optimisticTime == copyBytes(time7.encode()) &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"reflect", "flush-grant"},
          "Restored default-region reflection, grant, or callback order changed");
}

void runRestoreDefaultRegionAttributeMultiRecipient(Options const& options,
                                                    rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Default-region attribute restore has native evidence only for evoked callbacks");
  require(!options.ddmFom.empty() && !options.logicalTimeImplementationName.empty(),
          "Default-region restore requires adapter DDM FOM and time implementation");
  Session publisher(options, model, "owner");
  Session first(options, model, "member-a");
  Session second(options, model, "member-b");
  auto const federation = federationName(options, "restore-default-region-attribute-multi");
  std::wstring const saveLabel = L"tck-default-region-attribute-multi-baseline";
  std::array<std::uint8_t, 7> const valueBytes{'D', 'E', 'F', '-', 'M', 'U', 'L'};
  std::array<std::uint8_t, 7> const tagBytes{'D', 'E', 'F', '-', 'T', 'S', 'O'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  first.connect();
  second.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.ddmFom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  first.join(options.memberFederateName + L"-a", options.federateType + L"-subscriber",
             federation);
  second.join(options.memberFederateName + L"-b", options.federateType + L"-subscriber",
              federation);
  auto const publisherHandle = publisher.federateHandle();
  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter DDM FOM lacks publisher object class or attribute");
  rti::AttributeHandleSet const attributes{attribute};
  rti::AttributeHandleValueMap values;
  values.emplace(attribute,
                 rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  publisher.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      objectClass, attributes, rti::TIMESTAMP);
  auto const firstHandles = subscribe(first, options);
  auto const secondHandles = subscribe(second, options);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Default-region attribute target was invalid");
  waitForSessions({&first, &second}, [&] {
    return first.recorder().hasDiscovery(object) && second.recorder().hasDiscovery(object);
  }, options, "both default-region target discoveries");
  auto const time = makeTimeContext(publisher);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time5 = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const time7 = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const time10 = timeAfter(*time.factory, *time.initial, *time.epsilon, 10U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  auto const saved = publisher.rtiAmbassador().updateAttributeValues(
      object, values, tag, *time7);
  require(saved.isValid() && first.recorder().timedReflections().empty() &&
              second.recorder().timedReflections().empty(),
          "Default-source timestamp-seven update was not queued for both recipients");
  first.rtiAmbassador().timeAdvanceRequest(*time1);
  second.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "three-member default-region save initiation");
  publisher.rtiAmbassador().federateSaveBegun();
  first.rtiAmbassador().federateSaveBegun();
  second.rtiAmbassador().federateSaveBegun();
  publisher.rtiAmbassador().federateSaveComplete();
  first.rtiAmbassador().federateSaveComplete();
  second.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationSavedCount() == 1U &&
           first.federationSavedCount() == 1U && second.federationSavedCount() == 1U;
  }, options, "three-member default-region federation saved");

  publisher.rtiAmbassador().retract(saved);
  first.pump();
  second.pump();
  require(first.recorder().timedReflections().empty() &&
              second.recorder().timedReflections().empty(),
          "Post-save retract leaked a queued default-region reflection");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save terminal update handle");
  publisher.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           first.federateRestoreInitiations().size() == 1U &&
           second.federateRestoreInitiations().size() == 1U;
  }, options, "three-member default-region restore initiation");
  publisher.rtiAmbassador().federateRestoreComplete();
  first.rtiAmbassador().federateRestoreComplete();
  second.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           first.federationRestoredCount() == 1U &&
           second.federationRestoredCount() == 1U;
  }, options, "three-member default-region federation restored");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&first}, [&] {
    return first.recorder().timedReflections().size() == 1U &&
           first.recorder().flushQueueGrants().size() == 1U;
  }, options, "first restored default-region reflection and FQG");
  require(second.recorder().timedReflections().empty() &&
              second.recorder().flushQueueGrants().empty() &&
              second.recorder().callbackOrder().empty(),
          "First FQR prematurely released the second default-region reflection");
  second.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&second}, [&] {
    return second.recorder().timedReflections().size() == 1U &&
           second.recorder().flushQueueGrants().size() == 1U;
  }, options, "second restored default-region reflection and FQG");
  checkReflected(first, firstHandles, object, publisherHandle, saved,
                 *time7, *time5, valueBytes, tagBytes);
  checkReflected(second, secondHandles, object, publisherHandle, saved,
                 *time7, *time5, valueBytes, tagBytes);

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().retract(saved);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().retractions().size() == 1U &&
           second.recorder().retractions().size() == 1U;
  }, options, "both restored default-region request-retraction callbacks");
  for (Session* receiver : {&first, &second}) {
    auto const retraction = receiver->recorder().retractions().front();
    require(retraction.valid && retraction.encoded == copyBytes(saved.encode()) &&
                receiver->recorder().callbackOrder() ==
                    std::vector<std::string>{"request-retraction"},
            "Restored default-region retraction callback differed between recipients");
  }
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal update handle");

  first.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      firstHandles.objectClass, firstHandles.pair);
  second.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      secondHandles.objectClass, secondHandles.pair);
  first.rtiAmbassador().deleteRegion(firstHandles.region);
  second.rtiAmbassador().deleteRegion(secondHandles.region);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreDefaultRegionAttributeMultiRecipient(Options const& options,
                                                         rti::CallbackModel model) {
  runRestoreDefaultRegionAttributeMultiRecipient(options, model);
}

void scenarioRestoreDefaultRegionAttributeMultiRecipientContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreDefaultRegionAttributeMultiRecipient(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
