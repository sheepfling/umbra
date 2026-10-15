#include "tck_update_rate_scenarios.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioUpdateRateQueries(Options const& options, rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(options, "update-rate-queries");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  rti::ObjectClassHandle publisherClass;
  rti::AttributeHandle publisherAttribute;
  rti::InteractionClassHandle publisherInteraction;
  rti::ParameterHandle publisherParameter;
  handles(
      publisher,
      options,
      publisherClass,
      publisherAttribute,
      publisherInteraction,
      publisherParameter);
  rti::ObjectClassHandle subscriberClass;
  rti::AttributeHandle subscriberAttribute;
  rti::InteractionClassHandle subscriberInteraction;
  rti::ParameterHandle subscriberParameter;
  handles(
      subscriber,
      options,
      subscriberClass,
      subscriberAttribute,
      subscriberInteraction,
      subscriberParameter);
  require(
      publisherClass == subscriberClass && publisherAttribute == subscriberAttribute &&
          publisherInteraction == subscriberInteraction &&
          publisherParameter == subscriberParameter,
      "Update-rate query handles differed between joined federates");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet subscriberAttributes;
  subscriberAttributes.insert(subscriberAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"High");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "Update-rate query registration returned an invalid object handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "update-rate query object discovery");

  auto const namedRate = subscriber.rtiAmbassador().getUpdateRateValue(L"High");
  require(namedRate > 0.0, "named update-rate lookup did not return a positive value");
  require(
      subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          subscriberAttribute) == namedRate,
      "active named subscription did not determine the per-attribute rate");
  require(
      publisher.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          publisherAttribute) == 0.0,
      "publisher publication incorrectly contributed to its per-attribute rate");

  requireException(
      [&] {
        static_cast<void>(subscriber.rtiAmbassador().getUpdateRateValue(
            L"cpp-tck-missing-update-rate"));
      },
      L"InvalidUpdateRateDesignator",
      "unknown update-rate designator");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  require(
      subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          subscriberAttribute) == 0.0,
      "per-attribute rate did not reset after unsubscribe");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      false,
      L"High");
  require(
      subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          subscriberAttribute) == 0.0,
      "passive named subscription incorrectly contributed to the per-attribute rate");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");
  require(
      subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          subscriberAttribute) == 0.0,
      "active default subscription did not use the no-reduction rate boundary");

  requireException(
      [&] {
        static_cast<void>(subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
            rti::ObjectInstanceHandle{},
            subscriberAttribute));
      },
      L"ObjectInstanceNotKnown",
      "unknown object in per-attribute rate query");
  requireException(
      [&] {
        static_cast<void>(subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
            object,
            rti::AttributeHandle{}));
      },
      L"AttributeNotDefined",
      "undefined attribute in per-attribute rate query");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
}

void scenarioUpdateRateQueriesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUpdateRateQueries(options, model);
}

void scenarioMixedUpdateRateSubscriptions(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Mixed update-rate subscriptions require an adapter-supplied rich FOM model");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "mixed-update-rate-subscriptions");
  connectAndJoin(publisher, receiver, options, federation, options.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const publisherReliable = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.rateReliableAttributeName);
  auto const publisherBestEffort = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.rateBestEffortAttributeName);
  auto const receiverReliable = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.rateReliableAttributeName);
  auto const receiverBestEffort = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.rateBestEffortAttributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid(),
      "Mixed update-rate subscription object-class lookup returned an invalid handle");
  require(
      publisherReliable.isValid() && publisherBestEffort.isValid() &&
          receiverReliable.isValid() && receiverBestEffort.isValid(),
      "Mixed update-rate subscription attribute lookup returned an invalid handle");
  require(
      publisherReliable != publisherBestEffort && receiverReliable != receiverBestEffort,
      "Mixed update-rate subscription adapter supplied duplicate attribute names");

  rti::AttributeHandleSet const publisherAttributes{
      publisherReliable,
      publisherBestEffort};
  rti::AttributeHandleSet const receiverAttributes{
      receiverReliable,
      receiverBestEffort};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      rti::AttributeHandleSet{receiverReliable},
      true,
      L"");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      rti::AttributeHandleSet{receiverBestEffort},
      true,
      options.rateDesignatorName);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Mixed update-rate subscription registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "mixed update-rate subscription object discovery");

  auto const namedRate = receiver.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(
      namedRate > 0.0,
      "mixed update-rate subscription named designator did not resolve to a positive rate");
  require(
      receiver.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          receiverBestEffort) == namedRate,
      "mixed update-rate subscription named attribute did not select its rate");
  require(
      receiver.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          receiverReliable) == 0.0,
      "mixed update-rate subscription default attribute unexpectedly selected a rate");

  std::vector<std::uint8_t> const firstReliableBytes{0x31U, 0x32U};
  std::vector<std::uint8_t> const firstBestEffortBytes{0x41U, 0x42U};
  std::vector<std::uint8_t> const secondReliableBytes{0x51U, 0x52U};
  std::vector<std::uint8_t> const secondBestEffortBytes{0x61U, 0x62U};
  std::vector<std::uint8_t> const firstTagBytes{0x71U};
  std::vector<std::uint8_t> const secondTagBytes{0x72U};

  auto sendUpdate = [&](std::vector<std::uint8_t> const& reliableBytes,
                        std::vector<std::uint8_t> const& bestEffortBytes,
                        std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherReliable,
        rti::VariableLengthData(reliableBytes.data(), reliableBytes.size()));
    values.emplace(
        publisherBestEffort,
        rti::VariableLengthData(bestEffortBytes.data(), bestEffortBytes.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };

  auto countAttribute = [&](rti::AttributeHandle const& handle) {
    auto const reflections = receiver.recorder().reflections();
    return static_cast<std::size_t>(std::count_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.values.count(handle) == 1U;
        }));
  };
  auto assertPassel = [&](std::size_t begin,
                          std::vector<std::uint8_t> const& expectedReliable,
                          std::vector<std::uint8_t> const& expectedBestEffort,
                          std::vector<std::uint8_t> const& expectedTag,
                          bool expectBestEffort,
                          std::string const& description) {
    auto const reflections = receiver.recorder().reflections();
    require(
        begin <= reflections.size(),
        description + " began after the recorded reflection sequence");
    std::size_t reliableCount = 0U;
    std::size_t bestEffortCount = 0U;
    for (auto index = begin; index < reflections.size(); ++index) {
      auto const& reflection = reflections[index];
      require(
          reflection.present && reflection.object == object &&
              reflection.tag == expectedTag &&
              reflection.producer == publisher.federateHandle() &&
              !reflection.regions.has_value() && reflection.transportation.isValid(),
          description + " returned non-standard reflection metadata");
      auto const transportation = receiver.rtiAmbassador().getTransportationTypeName(
          reflection.transportation);
      for (auto const& [attribute, value] : reflection.values) {
        if (attribute == receiverReliable) {
          ++reliableCount;
          require(
              value.size() == expectedReliable.size() &&
                  copyBytes(value) == expectedReliable &&
                  transportation == L"HLAreliable",
              description + " returned the wrong reliable attribute value or transport");
        } else if (attribute == receiverBestEffort) {
          ++bestEffortCount;
          require(
              expectBestEffort && value.size() == expectedBestEffort.size() &&
                  copyBytes(value) == expectedBestEffort &&
                  transportation == L"HLAbestEffort",
              description + " returned the wrong best-effort attribute value or transport");
        } else {
          throw std::runtime_error(
              description + " returned an unexpected attribute handle");
        }
      }
    }
    require(
        reliableCount == 1U && bestEffortCount == (expectBestEffort ? 1U : 0U),
        description + " did not deliver the expected attributes exactly once");
  };

  auto const firstReflectionBegin = receiver.recorder().reflections().size();
  sendUpdate(firstReliableBytes, firstBestEffortBytes, firstTagBytes);
  waitFor(
      receiver,
      [&] {
        return countAttribute(receiverReliable) >= 1U &&
            countAttribute(receiverBestEffort) >= 1U;
      },
      options,
      "mixed update-rate subscription first reflection passel");
  auto const firstReflections = receiver.recorder().reflections();
  require(
      firstReflections.size() > firstReflectionBegin,
      "mixed update-rate subscription first update produced no reflection callback");
  assertPassel(
      firstReflectionBegin,
      firstReliableBytes,
      firstBestEffortBytes,
      firstTagBytes,
      true,
      "mixed update-rate subscription first passel");

  auto const secondReflectionBegin = receiver.recorder().reflections().size();
  sendUpdate(secondReliableBytes, secondBestEffortBytes, secondTagBytes);
  waitFor(
      receiver,
      [&] { return countAttribute(receiverReliable) >= 2U; },
      options,
      "mixed update-rate subscription second reliable reflection");
  auto const secondReflections = receiver.recorder().reflections();
  require(
      secondReflections.size() > secondReflectionBegin,
      "mixed update-rate subscription second update produced no reflection callback");
  require(
      countAttribute(receiverBestEffort) == 1U,
      "mixed update-rate subscription delivered a named-rate best-effort attribute twice");
  assertPassel(
      secondReflectionBegin,
      secondReliableBytes,
      secondBestEffortBytes,
      secondTagBytes,
      false,
      "mixed update-rate subscription second passel");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  receiver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeUpdateRateReduction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Timestamped update-rate reduction requires an adapter-supplied rich FOM model");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped update-rate reduction requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-rate-reduction");
  connectAndJoin(publisher, receiver, options, federation, options.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const publisherReliable = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.rateReliableAttributeName);
  auto const publisherBestEffort = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.rateBestEffortAttributeName);
  auto const receiverReliable = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.rateReliableAttributeName);
  auto const receiverBestEffort = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.rateBestEffortAttributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid(),
      "Timestamped update-rate reduction object-class lookup returned an invalid handle");
  require(
      publisherReliable.isValid() && publisherBestEffort.isValid() &&
          receiverReliable.isValid() && receiverBestEffort.isValid(),
      "Timestamped update-rate reduction attribute lookup returned an invalid handle");
  require(
      publisherReliable != publisherBestEffort && receiverReliable != receiverBestEffort,
      "Timestamped update-rate reduction adapter supplied duplicate attribute names");

  rti::AttributeHandleSet const publisherAttributes{
      publisherReliable,
      publisherBestEffort};
  rti::AttributeHandleSet const receiverAttributes{
      receiverReliable,
      receiverBestEffort};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      rti::AttributeHandleSet{receiverReliable},
      true,
      L"");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      rti::AttributeHandleSet{receiverBestEffort},
      true,
      options.rateDesignatorName);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped update-rate reduction registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Timestamped update-rate reduction object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped update-rate reduction members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped update-rate reduction could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped update-rate reduction time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped update-rate reduction time-regulation callback");

  auto const namedRate = receiver.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(
      namedRate > 0.0,
      "Timestamped update-rate reduction named designator did not resolve to a positive rate");
  require(
      receiver.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          receiverBestEffort) == namedRate,
      "Timestamped update-rate reduction active named subscription did not select its rate");
  require(
      receiver.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          receiverReliable) == 0.0,
      "Timestamped update-rate reduction default subscription unexpectedly selected a rate");

  std::vector<std::uint8_t> const reliableBytes{0x71U, 0x72U};
  std::vector<std::uint8_t> const bestEffortBytes{0x81U, 0x82U};
  std::vector<std::uint8_t> const tagBytes{0x91U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherReliable,
      rti::VariableLengthData(reliableBytes.data(), reliableBytes.size()));
  values.emplace(
      publisherBestEffort,
      rti::VariableLengthData(bestEffortBytes.data(), bestEffortBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  auto const publisherTargetOne = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const thirdTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "Timestamped update-rate reduction first update returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*firstTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetOne);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 2U;
      },
      options,
      "Timestamped update-rate reduction first reflections and grants");

  auto countAttribute = [&](rti::AttributeHandle const& handle) {
    std::size_t count = 0U;
    for (auto const& reflection : receiver.recorder().timedReflections()) {
      if (reflection.values.count(handle) == 1U) {
        ++count;
      }
    }
    return count;
  };
  auto assertReflection = [&](TimedReflectionRecord const& reflection,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTime,
                              std::vector<std::uint8_t> const& expectedRetraction,
                              std::wstring const& expectedTransportation) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.values.count(expectedAttribute) == 1U &&
            copyBytes(reflection.values.at(expectedAttribute)) == expectedValue,
        "Timestamped update-rate reduction reflection returned the wrong object, attribute, or value");
    require(
        reflection.tag == tagBytes && reflection.producer == publisher.federateHandle() &&
            reflection.time == expectedTime && !reflection.timeText.empty(),
        "Timestamped update-rate reduction reflection changed standard delivery metadata");
    require(
        reflection.transportation.isValid() &&
            receiver.rtiAmbassador().getTransportationTypeName(reflection.transportation) ==
                expectedTransportation,
        "Timestamped update-rate reduction reflection returned the wrong transportation type");
    require(
        reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.retractionPresent && reflection.retraction == expectedRetraction,
        "Timestamped update-rate reduction reflection returned the wrong order or retraction metadata");
  };
  auto const firstReflections = receiver.recorder().timedReflections();
  require(
      firstReflections.size() == 2U && countAttribute(receiverReliable) == 1U &&
          countAttribute(receiverBestEffort) == 1U,
      "Timestamped update-rate reduction first passel did not deliver both attributes once");
  for (auto const& reflection : firstReflections) {
    if (reflection.values.count(receiverReliable) == 1U) {
      assertReflection(
          reflection,
          receiverReliable,
          reliableBytes,
          encodeTime(*firstTime),
          copyBytes(firstRetraction.encode()),
          L"HLAreliable");
    } else {
      assertReflection(
          reflection,
          receiverBestEffort,
          bestEffortBytes,
          encodeTime(*firstTime),
          copyBytes(firstRetraction.encode()),
          L"HLAbestEffort");
    }
  }

  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "Timestamped update-rate reduction second update returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*secondTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetOne);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            receiver.recorder().timeAdvanceGrants().size() >= 2U &&
            receiver.recorder().timedReflections().size() >= 3U;
      },
      options,
      "Timestamped update-rate reduction second reflections and grants");
  auto const secondReflections = receiver.recorder().timedReflections();
  require(
      secondReflections.size() == 3U && countAttribute(receiverReliable) == 2U &&
          countAttribute(receiverBestEffort) == 1U,
      "Timestamped update-rate reduction suppressed the wrong second passel");
  auto const& secondReliableReflection = secondReflections.back();
  require(
      secondReliableReflection.values.count(receiverReliable) == 1U,
      "Timestamped update-rate reduction second delivery was not the reliable attribute");
  assertReflection(
      secondReliableReflection,
      receiverReliable,
      reliableBytes,
      encodeTime(*secondTime),
      copyBytes(secondRetraction.encode()),
      L"HLAreliable");

  rti::AttributeHandleValueMap bestEffortOnly;
  bestEffortOnly.emplace(
      publisherBestEffort,
      rti::VariableLengthData(bestEffortBytes.data(), bestEffortBytes.size()));
  auto const suppressedRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      bestEffortOnly,
      tag,
      *thirdTime);
  require(
      suppressedRetraction.isValid(),
      "Timestamped update-rate reduction suppressed update returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*thirdTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetOne);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 3U;
      },
      options,
      "Timestamped update-rate reduction suppressed passel grant");
  require(
      receiver.recorder().timedReflections().size() == 3U,
      "Timestamped update-rate reduction delivered an all-best-effort suppressed passel");
  publisher.rtiAmbassador().retract(suppressedRetraction);
  requireException(
      [&] { publisher.rtiAmbassador().retract(suppressedRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "Timestamped update-rate reduction second suppressed-passel retraction");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioMixedUpdateRateSubscriptionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMixedUpdateRateSubscriptions(options, model);
}

void scenarioTimestampedAttributeUpdateRateReductionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateRateReduction(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
