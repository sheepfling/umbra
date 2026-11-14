#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

struct RegionalSubscriptionTripleHandles {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle firstAttribute;
  rti::AttributeHandle secondAttribute;
  rti::DimensionHandle firstDimension;
  rti::DimensionHandle secondDimension;
};

RegionalSubscriptionTripleHandles resolveHandles(
    Session& session,
    Options const& options) {
  auto& ambassador = session.rtiAmbassador();
  RegionalSubscriptionTripleHandles handles;
  handles.objectClass = ambassador.getObjectClassHandle(
      options.multiAttributeObjectClassName);
  handles.firstAttribute = ambassador.getAttributeHandle(
      handles.objectClass,
      options.multiAttributeFirstName);
  handles.secondAttribute = ambassador.getAttributeHandle(
      handles.objectClass,
      options.multiAttributeSecondName);
  require(handles.objectClass.isValid() &&
              handles.firstAttribute.isValid() &&
              handles.secondAttribute.isValid() &&
              handles.firstAttribute != handles.secondAttribute,
          "regional subscription triple lookup returned invalid or duplicate handles");
  require(options.ddmDimensionNames.size() >= 2U,
          "regional subscription triple isolation requires two DDM dimensions");
  handles.firstDimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  handles.secondDimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(1));
  require(handles.firstDimension.isValid() &&
              handles.secondDimension.isValid() &&
              handles.firstDimension != handles.secondDimension,
          "regional subscription triple lookup returned invalid or duplicate dimensions");
  return handles;
}

rti::RegionHandle createRegion(
    Session& session,
    RegionalSubscriptionTripleHandles const& handles,
    unsigned long lowerBound,
    unsigned long upperBound) {
  auto& ambassador = session.rtiAmbassador();
  auto const region = ambassador.createRegion(
      rti::DimensionHandleSet{
          handles.firstDimension,
          handles.secondDimension});
  require(region.isValid(),
          "regional subscription triple region creation failed");
  ambassador.setRangeBounds(
      region,
      handles.firstDimension,
      rti::RangeBounds(lowerBound, upperBound));
  ambassador.setRangeBounds(
      region,
      handles.secondDimension,
      rti::RangeBounds(lowerBound, upperBound));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  return region;
}

bool hasAttributeValue(
    std::vector<ReflectionRecord> const& reflections,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& attribute,
    std::vector<std::uint8_t> const& expectedValue,
    std::vector<std::uint8_t> const& expectedTag,
    rti::FederateHandle const& expectedProducer) {
  return std::any_of(
      reflections.begin(),
      reflections.end(),
      [&](ReflectionRecord const& reflection) {
        auto const found = reflection.values.find(attribute);
        return reflection.present && reflection.object == object &&
            found != reflection.values.end() &&
            copyBytes(found->second) == expectedValue &&
            reflection.tag == expectedTag &&
            reflection.producer == expectedProducer;
      });
}

std::size_t countAttributeValue(
    std::vector<ReflectionRecord> const& reflections,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& attribute,
    std::vector<std::uint8_t> const& expectedValue,
    std::vector<std::uint8_t> const& expectedTag,
    rti::FederateHandle const& expectedProducer) {
  return static_cast<std::size_t>(std::count_if(
      reflections.begin(),
      reflections.end(),
      [&](ReflectionRecord const& reflection) {
        auto const found = reflection.values.find(attribute);
        return reflection.present && reflection.object == object &&
            found != reflection.values.end() &&
            copyBytes(found->second) == expectedValue &&
            reflection.tag == expectedTag &&
            reflection.producer == expectedProducer;
      }));
}

} // namespace

void scenarioRegionalSubscriptionTripleIsolation(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Regional subscription triple isolation requires a multi-attribute DDM FOM");

  Session publisher(options, model, "regional-triple-publisher");
  Session subscriber(options, model, "regional-triple-subscriber");
  Session regionFilteredSubscriber(options, model, "regional-triple-filtered-subscriber");
  auto const federation = federationName(
      options,
      "regional-subscription-triple-isolation");
  connectAndJoin(
      publisher,
      subscriber,
      options,
      federation,
      options.multiAttributeFom);
  regionFilteredSubscriber.connect();
  regionFilteredSubscriber.join(
      options.memberFederateName + L"-region-filtered",
      options.federateType,
      federation);

  auto const publisherHandles = resolveHandles(publisher, options);
  auto const subscriberHandles = resolveHandles(subscriber, options);
  auto const filteredHandles = resolveHandles(regionFilteredSubscriber, options);
  require(publisherHandles.objectClass == subscriberHandles.objectClass &&
              publisherHandles.firstAttribute == subscriberHandles.firstAttribute &&
              publisherHandles.secondAttribute == subscriberHandles.secondAttribute &&
              publisherHandles.firstDimension == subscriberHandles.firstDimension &&
              publisherHandles.secondDimension == subscriberHandles.secondDimension &&
              publisherHandles.objectClass == filteredHandles.objectClass &&
              publisherHandles.firstAttribute == filteredHandles.firstAttribute &&
              publisherHandles.secondAttribute == filteredHandles.secondAttribute &&
              publisherHandles.firstDimension == filteredHandles.firstDimension &&
              publisherHandles.secondDimension == filteredHandles.secondDimension,
          "regional subscription triple members resolved inconsistent handles");

  rti::AttributeHandleSet const publisherAttributes{
      publisherHandles.firstAttribute,
      publisherHandles.secondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);

  auto const sourceRegion = createRegion(publisher, publisherHandles, 1UL, 3UL);
  auto const secondSourceRegion =
      createRegion(publisher, publisherHandles, 5UL, 7UL);
  auto const subscriptionRegion =
      createRegion(subscriber, subscriberHandles, 1UL, 3UL);
  auto const secondSubscriptionRegion =
      createRegion(subscriber, subscriberHandles, 5UL, 7UL);
  auto const filteredFirstSubscriptionRegion =
      createRegion(regionFilteredSubscriber, filteredHandles, 1UL, 3UL);
  auto const filteredSecondSubscriptionRegion =
      createRegion(regionFilteredSubscriber, filteredHandles, 5UL, 7UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{
      {rti::AttributeHandleSet{publisherHandles.firstAttribute},
       rti::RegionHandleSet{sourceRegion}},
      {rti::AttributeHandleSet{publisherHandles.secondAttribute},
       rti::RegionHandleSet{sourceRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{
      {rti::AttributeHandleSet{subscriberHandles.firstAttribute},
       rti::RegionHandleSet{subscriptionRegion}},
      {rti::AttributeHandleSet{subscriberHandles.secondAttribute},
       rti::RegionHandleSet{subscriptionRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const secondSourcePair{
      {rti::AttributeHandleSet{publisherHandles.firstAttribute},
       rti::RegionHandleSet{secondSourceRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const secondSubscriptionPair{
      {rti::AttributeHandleSet{subscriberHandles.firstAttribute},
       rti::RegionHandleSet{secondSubscriptionRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const filteredSubscriptionPair{
      {rti::AttributeHandleSet{filteredHandles.firstAttribute},
       rti::RegionHandleSet{filteredFirstSubscriptionRegion}},
      {rti::AttributeHandleSet{filteredHandles.secondAttribute},
       rti::RegionHandleSet{filteredSecondSubscriptionRegion}}};

  rti::AttributeHandleSetRegionHandleSetPairVector const invalidRegionPair{{
      rti::AttributeHandleSet{subscriberHandles.firstAttribute},
      rti::RegionHandleSet{rti::RegionHandle{}}}};
  requireException(
      [&] {
        subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
            subscriberHandles.objectClass,
            invalidRegionPair);
      },
      L"InvalidRegion",
      "regional subscription with a non-region designator");

  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriptionPair);
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriptionPair);
  rti::AttributeHandleSetRegionHandleSetPairVector const emptyUnsubscribePair{{
      rti::AttributeHandleSet{subscriberHandles.firstAttribute},
      rti::RegionHandleSet{}}};
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      emptyUnsubscribePair);
  requireException(
      [&] {
        subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
            subscriberHandles.objectClass,
            invalidRegionPair);
      },
      L"InvalidRegion",
      "regional unsubscription with a non-region designator");
  regionFilteredSubscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      filteredHandles.objectClass,
      filteredSubscriptionPair);
  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      sourcePair);
  require(object.isValid(),
          "regional subscription triple registration returned an invalid object");
  waitForSessions(
      {&subscriber, &regionFilteredSubscriber},
      [&] {
        return subscriber.recorder().hasDiscovery(object) &&
            regionFilteredSubscriber.recorder().hasDiscovery(object);
      },
      options,
      "regional subscription triple object discovery");

  auto sendBothAttributes = [&](std::vector<std::uint8_t> const& firstValue,
                                std::vector<std::uint8_t> const& secondValue,
                                std::vector<std::uint8_t> const& tag) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherHandles.firstAttribute,
        rti::VariableLengthData(firstValue.data(), firstValue.size()));
    values.emplace(
        publisherHandles.secondAttribute,
        rti::VariableLengthData(secondValue.data(), secondValue.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData(tag.data(), tag.size()));
  };
  auto sendFirstAttribute = [&](rti::ObjectInstanceHandle const& targetObject,
                                std::vector<std::uint8_t> const& value,
                                std::vector<std::uint8_t> const& tag) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherHandles.firstAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        targetObject,
        values,
        rti::VariableLengthData(tag.data(), tag.size()));
  };

  rti::FederateHandle const producer = publisher.federateHandle();
  std::vector<std::uint8_t> const firstInitial{0x51U, 0x01U};
  std::vector<std::uint8_t> const secondInitial{0x52U, 0x01U};
  std::vector<std::uint8_t> const initialTag{0x61U, 0x01U};
  sendBothAttributes(firstInitial, secondInitial, initialTag);
  waitForSessions(
      {&subscriber, &regionFilteredSubscriber},
      [&] {
        auto const reflections = subscriber.recorder().reflections();
        return hasAttributeValue(
                   reflections,
                   object,
                   subscriberHandles.firstAttribute,
                   firstInitial,
                   initialTag,
                   producer) &&
            hasAttributeValue(
                reflections,
                object,
                subscriberHandles.secondAttribute,
                secondInitial,
                initialTag,
                producer) &&
            hasAttributeValue(
                regionFilteredSubscriber.recorder().reflections(),
                object,
                filteredHandles.firstAttribute,
                firstInitial,
                initialTag,
                producer);
      },
      options,
      "both active regional attribute triples before selective unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
    regionFilteredSubscriber.pump();
  }
  auto const filteredPasselReflections =
      regionFilteredSubscriber.recorder().reflections();
  require(
      filteredPasselReflections.size() == 1U &&
          filteredPasselReflections.front().object == object &&
          filteredPasselReflections.front().tag == initialTag &&
          filteredPasselReflections.front().producer == producer &&
          filteredPasselReflections.front().values.size() == 1U &&
          filteredPasselReflections.front().values.count(
              filteredHandles.firstAttribute) == 1U &&
          filteredPasselReflections.front().values.count(
              filteredHandles.secondAttribute) == 0U &&
          copyBytes(filteredPasselReflections.front().values.at(
              filteredHandles.firstAttribute)) == firstInitial,
      "regional subscription did not project the same-region two-attribute passel to the matching attribute only");
  auto const initialReflections = subscriber.recorder().reflections();
  require(
      initialReflections.size() == 1U &&
          initialReflections.front().values.size() == 2U &&
          initialReflections.front().values.count(
              subscriberHandles.firstAttribute) == 1U &&
          initialReflections.front().values.count(
              subscriberHandles.secondAttribute) == 1U &&
          countAttributeValue(
              initialReflections,
              object,
              subscriberHandles.firstAttribute,
              firstInitial,
              initialTag,
              producer) == 1U &&
          countAttributeValue(
              initialReflections,
              object,
              subscriberHandles.secondAttribute,
              secondInitial,
              initialTag,
              producer) == 1U,
      "repeating an existing regional subscription changed callback delivery");

  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      secondSubscriptionPair);
  auto const secondObject =
      publisher.rtiAmbassador().registerObjectInstanceWithRegions(
          publisherHandles.objectClass,
          secondSourcePair);
  require(secondObject.isValid() && secondObject != object,
          "second accumulated-region object registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(secondObject); },
      options,
      "object discovery through a region added by a later subscription invocation");

  subscriber.recorder().clearReflection();
  std::vector<std::uint8_t> const secondRegionValue{0x53U, 0x01U};
  std::vector<std::uint8_t> const secondRegionTag{0x63U, 0x01U};
  sendFirstAttribute(secondObject, secondRegionValue, secondRegionTag);
  waitFor(
      subscriber,
      [&] {
        return hasAttributeValue(
            subscriber.recorder().reflections(),
            secondObject,
            subscriberHandles.firstAttribute,
            secondRegionValue,
            secondRegionTag,
            producer);
      },
      options,
      "first-attribute delivery through the later-added subscription region");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
  }
  auto const secondRegionReflections = subscriber.recorder().reflections();
  require(
      countAttributeValue(
          secondRegionReflections,
          secondObject,
          subscriberHandles.firstAttribute,
          secondRegionValue,
          secondRegionTag,
          producer) == 1U &&
          secondRegionReflections.size() == 1U,
      "later-added regional subscription did not deliver its value exactly once");

  subscriber.recorder().clearReflection();
  std::vector<std::uint8_t> const firstRegionAfterAddition{0x51U, 0x03U};
  std::vector<std::uint8_t> const secondAttributeAfterAddition{0x52U, 0x03U};
  std::vector<std::uint8_t> const afterAdditionTag{0x61U, 0x03U};
  sendBothAttributes(
      firstRegionAfterAddition,
      secondAttributeAfterAddition,
      afterAdditionTag);
  waitFor(
      subscriber,
      [&] {
        auto const reflections = subscriber.recorder().reflections();
        return hasAttributeValue(
                   reflections,
                   object,
                   subscriberHandles.firstAttribute,
                   firstRegionAfterAddition,
                   afterAdditionTag,
                   producer) &&
            hasAttributeValue(
                reflections,
                object,
                subscriberHandles.secondAttribute,
                secondAttributeAfterAddition,
                afterAdditionTag,
                producer);
      },
      options,
      "original subscription region remains active after a later region is added");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
  }
  auto const retainedFirstRegionReflections = subscriber.recorder().reflections();
  require(
      countAttributeValue(
          retainedFirstRegionReflections,
          object,
          subscriberHandles.firstAttribute,
          firstRegionAfterAddition,
          afterAdditionTag,
          producer) == 1U,
      "adding a second subscription region removed first-attribute delivery through the first region");
  require(
      countAttributeValue(
          retainedFirstRegionReflections,
          object,
          subscriberHandles.secondAttribute,
          secondAttributeAfterAddition,
          afterAdditionTag,
          producer) == 1U,
      "adding a second subscription region removed second-attribute delivery through the first region");
  require(
      !retainedFirstRegionReflections.empty() &&
          std::all_of(
              retainedFirstRegionReflections.begin(),
              retainedFirstRegionReflections.end(),
              [&](ReflectionRecord const& reflection) {
                return reflection.object == object &&
                    reflection.tag == afterAdditionTag;
              }),
      "adding a second subscription region produced an unrelated first-region reflection");

  subscriber.recorder().clearReflection();
  rti::AttributeHandleSetRegionHandleSetPairVector const firstTriple{
      {rti::AttributeHandleSet{subscriberHandles.firstAttribute},
       rti::RegionHandleSet{subscriptionRegion}}};
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      firstTriple);
  // The pair is now absent. Repeating this unsubscribe must leave the other
  // class/region/attribute subscription triples unchanged.
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      firstTriple);

  std::vector<std::uint8_t> const firstAfterUnsubscribe{0x51U, 0x02U};
  std::vector<std::uint8_t> const secondAfterUnsubscribe{0x52U, 0x02U};
  std::vector<std::uint8_t> const postUnsubscribeTag{0x61U, 0x02U};
  sendBothAttributes(
      firstAfterUnsubscribe,
      secondAfterUnsubscribe,
      postUnsubscribeTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflectionCount() >= 1U; },
      options,
      "remaining regional attribute triple after selective unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
  }

  auto const remainingReflections = subscriber.recorder().reflections();
  require(remainingReflections.size() == 1U,
          "selective regional unsubscribe produced an absent or duplicate reflection");
  auto const& remaining = remainingReflections.front();
  auto const remainingValue =
      remaining.values.find(subscriberHandles.secondAttribute);
  require(remaining.present && remaining.object == object &&
              remaining.values.size() == 1U &&
              remainingValue != remaining.values.end() &&
              copyBytes(remainingValue->second) == secondAfterUnsubscribe &&
              remaining.tag == postUnsubscribeTag &&
              remaining.producer == producer &&
              remaining.values.count(subscriberHandles.firstAttribute) == 0U,
          "unsubscribing one regional triple changed the distinct attribute triple");

  subscriber.recorder().clearReflection();
  std::vector<std::uint8_t> const secondRegionAfterUnsubscribe{0x53U, 0x02U};
  std::vector<std::uint8_t> const secondRegionAfterUnsubscribeTag{0x63U, 0x02U};
  sendFirstAttribute(
      secondObject,
      secondRegionAfterUnsubscribe,
      secondRegionAfterUnsubscribeTag);
  waitFor(
      subscriber,
      [&] {
        return hasAttributeValue(
            subscriber.recorder().reflections(),
            secondObject,
            subscriberHandles.firstAttribute,
            secondRegionAfterUnsubscribe,
            secondRegionAfterUnsubscribeTag,
            producer);
      },
      options,
      "later-added region remains subscribed after first-region unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
  }
  auto const secondRegionAfterUnsubscribeReflections =
      subscriber.recorder().reflections();
  require(
      secondRegionAfterUnsubscribeReflections.size() == 1U &&
          countAttributeValue(
              secondRegionAfterUnsubscribeReflections,
              secondObject,
              subscriberHandles.firstAttribute,
              secondRegionAfterUnsubscribe,
              secondRegionAfterUnsubscribeTag,
              producer) == 1U,
      "unsubscribing the first region removed the later-added second region");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      secondSubscriptionPair);

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      rti::AttributeHandleSetRegionHandleSetPairVector{{
          rti::AttributeHandleSet{subscriberHandles.secondAttribute},
          rti::RegionHandleSet{subscriptionRegion}}});
  regionFilteredSubscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      filteredHandles.objectClass,
      filteredSubscriptionPair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(
      secondObject,
      secondSourcePair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  publisher.rtiAmbassador().deleteRegion(secondSourceRegion);
  subscriber.rtiAmbassador().deleteRegion(subscriptionRegion);
  subscriber.rtiAmbassador().deleteRegion(secondSubscriptionRegion);
  regionFilteredSubscriber.rtiAmbassador().deleteRegion(
      filteredFirstSubscriptionRegion);
  regionFilteredSubscriber.rtiAmbassador().deleteRegion(
      filteredSecondSubscriptionRegion);
  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
  regionFilteredSubscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioRegionalSubscriptionTripleIsolationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalSubscriptionTripleIsolation(options, model);
}

} // namespace hla_rti_cpp_tck::detail
