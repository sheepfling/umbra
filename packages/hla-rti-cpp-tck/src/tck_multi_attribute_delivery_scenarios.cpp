#include "tck_multi_attribute_delivery_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOrdinaryMultiAttributeSubscriptionProjection(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "ordinary multi-attribute projection requires an adapter-supplied FOM");
  Session publisher(options, model, "ordinary-multi-attribute-publisher");
  Session both(options, model, "ordinary-multi-attribute-both");
  Session firstOnly(options, model, "ordinary-multi-attribute-first");
  Session secondOnly(options, model, "ordinary-multi-attribute-second");
  auto const federation = federationName(
      options,
      "ordinary-multi-attribute-subscription-projection");
  connectAndJoin(publisher, both, options, federation, options.multiAttributeFom);
  firstOnly.connect();
  firstOnly.join(
      options.memberFederateName + L"-first",
      options.federateType,
      federation);
  secondOnly.connect();
  secondOnly.join(
      options.memberFederateName + L"-second",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const bothClass = both.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const firstClass = firstOnly.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const secondClass = secondOnly.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherFirst = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const publisherSecond = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeSecondName);
  auto const bothFirst = both.rtiAmbassador().getAttributeHandle(
      bothClass,
      options.multiAttributeFirstName);
  auto const bothSecond = both.rtiAmbassador().getAttributeHandle(
      bothClass,
      options.multiAttributeSecondName);
  auto const firstOnlyAttribute = firstOnly.rtiAmbassador().getAttributeHandle(
      firstClass,
      options.multiAttributeFirstName);
  auto const secondOnlyAttribute = secondOnly.rtiAmbassador().getAttributeHandle(
      secondClass,
      options.multiAttributeSecondName);
  require(
      publisherClass.isValid() && bothClass.isValid() && firstClass.isValid() &&
          secondClass.isValid() && publisherFirst.isValid() &&
          publisherSecond.isValid() && bothFirst.isValid() &&
          bothSecond.isValid() && firstOnlyAttribute.isValid() &&
          secondOnlyAttribute.isValid(),
      "ordinary multi-attribute projection lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{
      publisherFirst,
      publisherSecond};
  rti::AttributeHandleSet const bothAttributes{bothFirst, bothSecond};
  rti::AttributeHandleSet const firstOnlyAttributes{firstOnlyAttribute};
  rti::AttributeHandleSet const secondOnlyAttributes{secondOnlyAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  both.rtiAmbassador().subscribeObjectClassAttributes(
      bothClass,
      bothAttributes,
      true,
      L"");
  firstOnly.rtiAmbassador().subscribeObjectClassAttributes(
      firstClass,
      firstOnlyAttributes,
      true,
      L"");
  secondOnly.rtiAmbassador().subscribeObjectClassAttributes(
      secondClass,
      secondOnlyAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "ordinary multi-attribute projection registration returned an invalid object");
  waitForSessions(
      {&both, &firstOnly, &secondOnly},
      [&] {
        return both.recorder().hasDiscovery(object) &&
            firstOnly.recorder().hasDiscovery(object) &&
            secondOnly.recorder().hasDiscovery(object);
      },
      options,
      "ordinary multi-attribute projection discovery");
  both.recorder().clearReflection();
  firstOnly.recorder().clearReflection();
  secondOnly.recorder().clearReflection();

  auto update = [&](bool includeFirst,
                    bool includeSecond,
                    std::uint8_t firstValue,
                    std::uint8_t secondValue,
                    std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap values;
    std::vector<std::uint8_t> firstBytes{firstValue};
    std::vector<std::uint8_t> secondBytes{secondValue};
    if (includeFirst) {
      values.emplace(
          publisherFirst,
          rti::VariableLengthData(firstBytes.data(), firstBytes.size()));
    }
    if (includeSecond) {
      values.emplace(
          publisherSecond,
          rti::VariableLengthData(secondBytes.data(), secondBytes.size()));
    }
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };

  std::vector<std::uint8_t> const bothTag{0xB1U, 0x01U};
  std::vector<std::uint8_t> const firstTag{0xF1U, 0x02U};
  std::vector<std::uint8_t> const secondTag{0x51U, 0x03U};
  update(true, true, 0x11U, 0x22U, bothTag);
  waitForSessions(
      {&both, &firstOnly, &secondOnly},
      [&] {
        return both.recorder().reflections().size() >= 1U &&
            firstOnly.recorder().reflections().size() >= 1U &&
            secondOnly.recorder().reflections().size() >= 1U;
      },
      options,
      "ordinary multi-attribute projection first delivery");

  auto assertMetadata = [&](ReflectionRecord const& record,
                            std::vector<std::uint8_t> const& expectedTag,
                            std::string const& description) {
    require(
        record.object == object && record.tag == expectedTag &&
            record.producer == publisher.federateHandle() &&
            record.transportation.isValid(),
        description + " returned the wrong delivery metadata");
  };
  auto const bothFirstDelivery = both.recorder().reflections().at(0);
  assertMetadata(
      bothFirstDelivery,
      bothTag,
      "both-attribute subscriber first delivery");
  require(
      bothFirstDelivery.values.size() == 2U &&
          bothFirstDelivery.values.count(bothFirst) == 1U &&
          bothFirstDelivery.values.count(bothSecond) == 1U &&
          copyBytes(bothFirstDelivery.values.at(bothFirst)) ==
              std::vector<std::uint8_t>{0x11U} &&
          copyBytes(bothFirstDelivery.values.at(bothSecond)) ==
              std::vector<std::uint8_t>{0x22U},
      "both-attribute subscriber did not receive the complete attribute map");

  auto const firstOnlyDelivery = firstOnly.recorder().reflections().at(0);
  assertMetadata(
      firstOnlyDelivery,
      bothTag,
      "first-only subscriber first delivery");
  require(
      firstOnlyDelivery.values.size() == 1U &&
          firstOnlyDelivery.values.count(firstOnlyAttribute) == 1U &&
          copyBytes(firstOnlyDelivery.values.at(firstOnlyAttribute)) ==
              std::vector<std::uint8_t>{0x11U},
      "first-only subscriber received an unrequested attribute");

  auto const secondOnlyDelivery = secondOnly.recorder().reflections().at(0);
  assertMetadata(
      secondOnlyDelivery,
      bothTag,
      "second-only subscriber first delivery");
  require(
      secondOnlyDelivery.values.size() == 1U &&
          secondOnlyDelivery.values.count(secondOnlyAttribute) == 1U &&
          copyBytes(secondOnlyDelivery.values.at(secondOnlyAttribute)) ==
              std::vector<std::uint8_t>{0x22U},
      "second-only subscriber received an unrequested attribute");

  update(true, false, 0x33U, 0x00U, firstTag);
  waitForSessions(
      {&both, &firstOnly},
      [&] {
        return both.recorder().reflections().size() >= 2U &&
            firstOnly.recorder().reflections().size() >= 2U;
      },
      options,
      "ordinary multi-attribute projection first-only delivery");
  for (int pass = 0; pass != 8; ++pass) {
    secondOnly.pump();
  }
  require(
      secondOnly.recorder().reflections().size() == 1U,
      "second-only subscriber received an unrequested first-attribute update");
  auto const bothFirstOnlyDelivery = both.recorder().reflections().at(1);
  require(
      bothFirstOnlyDelivery.values.size() == 1U &&
          bothFirstOnlyDelivery.values.count(bothFirst) == 1U &&
          copyBytes(bothFirstOnlyDelivery.values.at(bothFirst)) ==
              std::vector<std::uint8_t>{0x33U},
      "both-attribute subscriber did not project the first-only update");
  auto const firstOnlySecondDelivery = firstOnly.recorder().reflections().at(1);
  require(
      firstOnlySecondDelivery.values.size() == 1U &&
          firstOnlySecondDelivery.values.count(firstOnlyAttribute) == 1U &&
          copyBytes(firstOnlySecondDelivery.values.at(firstOnlyAttribute)) ==
              std::vector<std::uint8_t>{0x33U},
      "first-only subscriber did not receive the first-only update");
  require(
      bothFirstOnlyDelivery.tag == firstTag &&
          firstOnlySecondDelivery.tag == firstTag,
      "first-only update did not preserve its user tag");

  update(false, true, 0x00U, 0x44U, secondTag);
  waitForSessions(
      {&both, &secondOnly},
      [&] {
        return both.recorder().reflections().size() >= 3U &&
            secondOnly.recorder().reflections().size() >= 2U;
      },
      options,
      "ordinary multi-attribute projection second-only delivery");
  require(
      firstOnly.recorder().reflections().size() == 2U,
      "first-only subscriber received an unrequested second-attribute update");
  auto const bothSecondOnlyDelivery = both.recorder().reflections().at(2);
  require(
      bothSecondOnlyDelivery.values.size() == 1U &&
          bothSecondOnlyDelivery.values.count(bothSecond) == 1U &&
          copyBytes(bothSecondOnlyDelivery.values.at(bothSecond)) ==
              std::vector<std::uint8_t>{0x44U},
      "both-attribute subscriber did not project the second-only update");
  auto const secondOnlySecondDelivery = secondOnly.recorder().reflections().at(1);
  require(
      secondOnlySecondDelivery.values.size() == 1U &&
          secondOnlySecondDelivery.values.count(secondOnlyAttribute) == 1U &&
          copyBytes(secondOnlySecondDelivery.values.at(secondOnlyAttribute)) ==
              std::vector<std::uint8_t>{0x44U},
      "second-only subscriber did not receive the second-only update");
  require(
      bothSecondOnlyDelivery.tag == secondTag &&
          secondOnlySecondDelivery.tag == secondTag,
      "second-only update did not preserve its user tag");
  require(
      publisher.recorder().reflections().empty(),
      "ordinary multi-attribute projection delivered a reflection to its owner");

  both.rtiAmbassador().unsubscribeObjectClassAttributes(
      bothClass,
      bothAttributes);
  firstOnly.rtiAmbassador().unsubscribeObjectClassAttributes(
      firstClass,
      firstOnlyAttributes);
  secondOnly.rtiAmbassador().unsubscribeObjectClassAttributes(
      secondClass,
      secondOnlyAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  secondOnly.resign(rti::NO_ACTION);
  firstOnly.resign(rti::NO_ACTION);
  both.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  secondOnly.disconnect();
  firstOnly.disconnect();
  both.disconnect();
  publisher.disconnect();
}

void scenarioOrdinaryMultiAttributeSubscriptionProjectionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOrdinaryMultiAttributeSubscriptionProjection(options, model);
}

void scenarioSubscriptionInitialReflectionProjection(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "subscription initial reflection requires an adapter-supplied multi-attribute FOM");
  Session publisher(options, model, "subscription-initial-reflection-publisher");
  Session subscriber(options, model, "subscription-initial-reflection-subscriber");
  auto const federation = federationName(
      options,
      "subscription-initial-reflection-projection");
  connectAndJoin(publisher, subscriber, options, federation, options.multiAttributeFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherFirst = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const publisherSecond = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeSecondName);
  auto const subscriberFirst = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.multiAttributeFirstName);
  auto const subscriberSecond = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.multiAttributeSecondName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherFirst.isValid() && publisherSecond.isValid() &&
          subscriberFirst.isValid() && subscriberSecond.isValid(),
      "subscription initial reflection lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherFirst, publisherSecond};
  rti::AttributeHandleSet const subscribedAttributes{subscriberFirst};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "subscription initial reflection registration returned an invalid handle");

  std::vector<std::uint8_t> const currentFirst{0x21U, 0x22U};
  std::vector<std::uint8_t> const currentSecond{0x31U, 0x32U};
  rti::AttributeHandleValueMap currentValues;
  currentValues.emplace(
      publisherFirst,
      rti::VariableLengthData(currentFirst.data(), currentFirst.size()));
  currentValues.emplace(
      publisherSecond,
      rti::VariableLengthData(currentSecond.data(), currentSecond.size()));
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      currentValues,
      rti::VariableLengthData{});
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(object) &&
          subscriber.recorder().reflections().empty(),
      "unsubscribed member received discovery or reflection before subscribing");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscribedAttributes,
      true,
      L"");
  waitFor(
      subscriber,
      [&] {
        return subscriber.recorder().hasDiscovery(object) &&
            subscriber.recorder().reflections().size() >= 1U;
      },
      options,
      "subscription initial reflection delivery");
  auto const discovery = subscriber.recorder().discovery();
  require(
      subscriber.recorder().discoveryCount() == 1U && discovery.present &&
          discovery.object == object && discovery.objectClass == subscriberClass,
      "subscription of an existing object did not produce exactly one matching discovery");

  auto assertSubscribedProjection = [&](ReflectionRecord const& reflection,
                                        std::vector<std::uint8_t> const& expected,
                                        std::string const& description) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.values.count(subscriberFirst) == 1U &&
            reflection.values.count(subscriberSecond) == 0U &&
            copyBytes(reflection.values.at(subscriberFirst)) == expected,
        description + " returned values outside the active subscription or the wrong value");
  };
  require(
      subscriber.recorder().reflections().size() == 1U,
      "subscription initial reflection delivered a duplicate initial-value callback");
  assertSubscribedProjection(
      subscriber.recorder().reflections().front(),
      currentFirst,
      "subscription initial reflection");

  std::vector<std::uint8_t> const laterFirst{0x41U, 0x42U};
  std::vector<std::uint8_t> const laterSecond{0x51U, 0x52U};
  rti::AttributeHandleValueMap laterValues;
  laterValues.emplace(
      publisherFirst,
      rti::VariableLengthData(laterFirst.data(), laterFirst.size()));
  laterValues.emplace(
      publisherSecond,
      rti::VariableLengthData(laterSecond.data(), laterSecond.size()));
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      laterValues,
      rti::VariableLengthData{});
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflections().size() >= 2U; },
      options,
      "subscription initial reflection later update");
  require(
      subscriber.recorder().reflections().size() == 2U,
      "multi-attribute update delivered a duplicate reflection to the selective subscriber");
  assertSubscribedProjection(
      subscriber.recorder().reflections().at(1),
      laterFirst,
      "subscription initial reflection later update");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscribedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioSubscriptionInitialReflectionProjectionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSubscriptionInitialReflectionProjection(options, model);
}

void scenarioOrdinaryMultiAttributeValueUpdateRequestResponse(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "ordinary multi-attribute value-request response requires an adapter-supplied FOM");
  Session owner(options, model, "ordinary-multi-attribute-request-owner");
  Session requester(options, model, "ordinary-multi-attribute-requester");
  auto const federation = federationName(
      options,
      "ordinary-multi-attribute-value-update-request-response");
  connectAndJoin(owner, requester, options, federation, options.multiAttributeFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.multiAttributeSecondName);
  require(
      ownerClass.isValid() && requesterClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && requesterFirst.isValid() && requesterSecond.isValid(),
      "ordinary multi-attribute value-request response lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterAttributes{requesterFirst, requesterSecond};
  rti::AttributeHandleSet const ownerFirstOnly{ownerFirst};
  rti::AttributeHandleSet const requesterFirstOnly{requesterFirst};
  rti::AttributeHandleSet const ownerSecondOnly{ownerSecond};
  rti::AttributeHandleSet const requesterSecondOnly{requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "ordinary multi-attribute value-request response registration returned an invalid object");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "ordinary multi-attribute value-request response discovery");
  auto const knownObject = requester.recorder().discovery().object;
  require(
      knownObject == object,
      "ordinary multi-attribute value-request response discovery returned the wrong object");

  auto const reliable = requester.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliable.isValid(),
      "ordinary multi-attribute value-request response could not resolve HLAreliable");

  auto requestAndReflect = [&](rti::AttributeHandleSet const& requested,
                               rti::AttributeHandleSet const& expectedOwner,
                               std::vector<std::uint8_t> const& requestTagBytes,
                               rti::AttributeHandleValueMap const& responseValues,
                               std::vector<std::pair<rti::AttributeHandle, std::vector<std::uint8_t>>> const& expectedValues,
                               std::vector<std::uint8_t> const& responseTagBytes,
                               std::string const& description) {
    owner.recorder().clearProvidedUpdates();
    requester.recorder().clearReflection();
    rti::VariableLengthData const requestTag(
        requestTagBytes.data(),
        requestTagBytes.size());
    requester.rtiAmbassador().requestAttributeValueUpdate(
        knownObject,
        requested,
        requestTag);
    waitFor(
        owner,
        [&] {
          auto const requests = owner.recorder().providedUpdates();
          return requests.size() >= 1U && requests.back().object == object &&
              requests.back().attributes == expectedOwner &&
              requests.back().tag == requestTagBytes;
        },
        options,
        description + " provider callback");
    auto const requests = owner.recorder().providedUpdates();
    require(
        requests.size() == 1U && requests.front().object == object &&
            requests.front().attributes == expectedOwner &&
            requests.front().tag == requestTagBytes,
        description + " provider callback returned the wrong object, attributes, or tag");
    require(
        requester.recorder().reflectionCount() == 0U,
        description + " was reflected before the provider answered");

    rti::VariableLengthData const responseTag(
        responseTagBytes.data(),
        responseTagBytes.size());
    owner.rtiAmbassador().updateAttributeValues(object, responseValues, responseTag);
    waitFor(
        requester,
        [&] { return requester.recorder().reflection().present; },
        options,
        description + " reflection");
    auto const reflection = requester.recorder().reflection();
    require(
        reflection.object == knownObject &&
            reflection.values.size() == expectedValues.size() &&
            reflection.tag == responseTagBytes && reflection.transportation == reliable &&
            reflection.producer == owner.federateHandle() && !reflection.regions.has_value(),
        description + " reflection returned the wrong object, metadata, or attribute count");
    for (auto const& [attribute, expected] : expectedValues) {
      require(
          reflection.values.count(attribute) == 1U &&
              copyBytes(reflection.values.at(attribute)) == expected,
          description + " reflection returned the wrong attribute value");
    }
  };

  rti::AttributeHandleValueMap bothResponse;
  std::vector<std::uint8_t> const firstValue{0x11U};
  std::vector<std::uint8_t> const secondValue{0x22U};
  bothResponse.emplace(
      ownerFirst,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  bothResponse.emplace(
      ownerSecond,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  requestAndReflect(
      requesterAttributes,
      ownerAttributes,
      {0x52U, 0x31U},
      bothResponse,
      {{requesterFirst, firstValue}, {requesterSecond, secondValue}},
      {0x52U, 0x41U},
      "ordinary multi-attribute full value request");

  rti::AttributeHandleValueMap firstResponse;
  std::vector<std::uint8_t> const firstReplacement{0x33U};
  firstResponse.emplace(
      ownerFirst,
      rti::VariableLengthData(firstReplacement.data(), firstReplacement.size()));
  requestAndReflect(
      requesterFirstOnly,
      ownerFirstOnly,
      {0x52U, 0x32U},
      firstResponse,
      {{requesterFirst, firstReplacement}},
      {0x52U, 0x42U},
      "ordinary multi-attribute first-only value request");

  rti::AttributeHandleValueMap secondResponse;
  std::vector<std::uint8_t> const secondReplacement{0x44U};
  secondResponse.emplace(
      ownerSecond,
      rti::VariableLengthData(secondReplacement.data(), secondReplacement.size()));
  requestAndReflect(
      requesterSecondOnly,
      ownerSecondOnly,
      {0x52U, 0x33U},
      secondResponse,
      {{requesterSecond, secondReplacement}},
      {0x52U, 0x43U},
      "ordinary multi-attribute second-only value request");

  require(
      owner.recorder().reflectionCount() == 0U,
      "ordinary multi-attribute value-request response looped back to the provider");
  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOrdinaryMultiAttributeValueUpdateRequestResponseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOrdinaryMultiAttributeValueUpdateRequestResponse(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
