#include "tck_update_rate_scenarios.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {

static_assert(std::is_same_v<rti::AttributeHandleSet, std::set<rti::AttributeHandle>>);
static_assert(std::is_same_v<
              rti::AttributeHandleValueMap,
              std::map<rti::AttributeHandle, rti::VariableLengthData>>);
static_assert(std::is_same_v<
              rti::ParameterHandleValueMap,
              std::map<rti::ParameterHandle, rti::VariableLengthData>>);
static_assert(std::is_same_v<
              rti::AttributeHandleSetRegionHandleSetPair,
              std::pair<rti::AttributeHandleSet, rti::RegionHandleSet>>);
static_assert(std::is_same_v<
              rti::AttributeHandleSetRegionHandleSetPairVector,
              std::vector<rti::AttributeHandleSetRegionHandleSetPair>>);
static_assert(std::is_same_v<
              rti::FederateHandleSaveStatusPair,
              std::pair<rti::FederateHandle, rti::SaveStatus>>);
static_assert(std::is_same_v<
              rti::FederateHandleSaveStatusPairVector,
              std::vector<rti::FederateHandleSaveStatusPair>>);

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
  require(
      namedRate == 1.0,
      "named update-rate lookup did not return the High rate defined by the P0 FOM");
  require(
      publisher.rtiAmbassador().getUpdateRateValue(L"High") == namedRate,
      "the same FDD update-rate designator resolved to different values for joined federates");
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
  require(
      publisherAttributes.size() == 2U && receiverAttributes.size() == 2U,
      "C++ AttributeHandleSet did not retain both distinct attribute handles");
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
    require(
        values.size() == 2U,
        "C++ AttributeHandleValueMap did not retain both distinct handle/value pairs");
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

  Session noRateReceiver(options, model, "member");
  noRateReceiver.connect();
  noRateReceiver.join(
      options.memberFederateName + L"-no-rate-observer",
      options.federateType,
      federation);
  auto const regionalObjectClass = publisher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckTransportRegionalObject");
  auto const publisherRegionalAttribute = publisher.rtiAmbassador().getAttributeHandle(
      regionalObjectClass,
      L"RegionalValue");
  auto const receiverRegionalAttribute = receiver.rtiAmbassador().getAttributeHandle(
      regionalObjectClass,
      L"RegionalValue");
  auto const noRateRegionalAttribute = noRateReceiver.rtiAmbassador().getAttributeHandle(
      regionalObjectClass,
      L"RegionalValue");
  require(
      regionalObjectClass.isValid() && publisherRegionalAttribute.isValid() &&
          receiverRegionalAttribute.isValid() && noRateRegionalAttribute.isValid(),
      "regional update-rate object class or attribute lookup returned an invalid handle");
  rti::AttributeHandleSet const regionalPublisherAttributes{publisherRegionalAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      regionalObjectClass,
      regionalPublisherAttributes);

  auto createRateRegion = [&](Session& session) {
    auto& ambassador = session.rtiAmbassador();
    rti::DimensionHandleSet dimensions;
    std::vector<rti::DimensionHandle> handles;
    for (auto const& name : options.ddmDimensionNames) {
      auto const dimension = ambassador.getDimensionHandle(name);
      require(
          dimension.isValid() && ambassador.getDimensionUpperBound(dimension) >= 3UL,
          "regional update-rate FOM dimension is invalid or too small for the test range");
      dimensions.insert(dimension);
      handles.push_back(dimension);
    }
    require(
        handles.size() == 2U,
        "regional update-rate test requires the two dimensions of TckTransportRegionalObject");
    auto const region = ambassador.createRegion(dimensions);
    require(region.isValid(), "regional update-rate region creation failed");
    for (auto const dimension : handles) {
      ambassador.setRangeBounds(region, dimension, rti::RangeBounds(1UL, 3UL));
    }
    ambassador.commitRegionModifications(rti::RegionHandleSet{region});
    return region;
  };
  auto const regionalSourceRegion = createRateRegion(publisher);
  auto const regionalSubscriptionRegion = createRateRegion(receiver);
  auto const regionalNoRateRegion = createRateRegion(noRateReceiver);
  rti::AttributeHandleSetRegionHandleSetPairVector const regionalSourcePairs{{
      rti::AttributeHandleSet{publisherRegionalAttribute},
      rti::RegionHandleSet{regionalSourceRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const regionalSubscriptionPairs{{
      rti::AttributeHandleSet{receiverRegionalAttribute},
      rti::RegionHandleSet{regionalSubscriptionRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const regionalNoRatePairs{{
      rti::AttributeHandleSet{noRateRegionalAttribute},
      rti::RegionHandleSet{regionalNoRateRegion}}};
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      regionalObjectClass,
      regionalSubscriptionPairs,
      true,
      options.rateDesignatorName);
  noRateReceiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      regionalObjectClass,
      regionalNoRatePairs,
      true,
      L"");
  auto const regionalObject = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      regionalObjectClass,
      regionalSourcePairs);
  require(
      regionalObject.isValid(),
      "regional update-rate registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(regionalObject); },
      options,
      "regional update-rate object discovery for rate-limited subscriber");
  waitFor(
      noRateReceiver,
      [&] { return noRateReceiver.recorder().hasDiscovery(regionalObject); },
      options,
      "regional update-rate object discovery for default-rate subscriber");

  auto const regionalNamedRate = receiver.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(
      regionalNamedRate > 0.0 &&
          receiver.rtiAmbassador().getUpdateRateValueForAttribute(
              regionalObject,
              receiverRegionalAttribute) == regionalNamedRate,
      "regional subscription did not apply the specified maximum update rate");
  require(
      noRateReceiver.rtiAmbassador().getUpdateRateValueForAttribute(
          regionalObject,
          noRateRegionalAttribute) == 0.0,
      "regional subscription without a designator did not retain the default rate");

  std::vector<std::uint8_t> const regionalFirstValue{0x31U};
  std::vector<std::uint8_t> const regionalSecondValue{0x32U};
  std::vector<std::uint8_t> const regionalFirstTag{0x73U};
  std::vector<std::uint8_t> const regionalSecondTag{0x74U};
  auto sendRegionalUpdate = [&](std::vector<std::uint8_t> const& value,
                                std::vector<std::uint8_t> const& tag) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherRegionalAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        regionalObject,
        values,
        rti::VariableLengthData(tag.data(), tag.size()));
  };
  auto countRegionalValue = [&](Session& subscriber,
                                rti::AttributeHandle const& attribute,
                                std::vector<std::uint8_t> const& expected,
                                std::vector<std::uint8_t> const& expectedTag) {
    auto const reflections = subscriber.recorder().reflections();
    return static_cast<std::size_t>(std::count_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const found = reflection.values.find(attribute);
          return reflection.present && reflection.object == regionalObject &&
              found != reflection.values.end() &&
              copyBytes(found->second) == expected &&
              reflection.tag == expectedTag &&
              reflection.producer == publisher.federateHandle();
        }));
  };
  sendRegionalUpdate(regionalFirstValue, regionalFirstTag);
  waitFor(
      receiver,
      [&] {
        return countRegionalValue(
            receiver,
            receiverRegionalAttribute,
            regionalFirstValue,
            regionalFirstTag) == 1U;
      },
      options,
      "regional update-rate first callback for rate-limited subscriber");
  waitFor(
      noRateReceiver,
      [&] {
        return countRegionalValue(
            noRateReceiver,
            noRateRegionalAttribute,
            regionalFirstValue,
            regionalFirstTag) == 1U;
      },
      options,
      "regional update-rate first callback for default-rate subscriber");

  sendRegionalUpdate(regionalSecondValue, regionalSecondTag);
  waitFor(
      noRateReceiver,
      [&] {
        return countRegionalValue(
            noRateReceiver,
            noRateRegionalAttribute,
            regionalSecondValue,
            regionalSecondTag) == 1U;
      },
      options,
      "regional update with no maximum-rate reduction");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    receiver.pump();
    noRateReceiver.pump();
  }
  require(
      countRegionalValue(
          receiver,
          receiverRegionalAttribute,
          regionalSecondValue,
          regionalSecondTag) == 0U &&
          countRegionalValue(
              noRateReceiver,
              noRateRegionalAttribute,
              regionalSecondValue,
              regionalSecondTag) == 1U,
      "regional specified-rate subscription did not reduce the best-effort update while the omitted-rate subscriber received it");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      regionalObjectClass,
      regionalSubscriptionPairs);
  noRateReceiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      regionalObjectClass,
      regionalNoRatePairs);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(
      regionalObject,
      regionalSourcePairs);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      regionalObjectClass,
      regionalPublisherAttributes);
  publisher.rtiAmbassador().deleteRegion(regionalSourceRegion);
  receiver.rtiAmbassador().deleteRegion(regionalSubscriptionRegion);
  noRateReceiver.rtiAmbassador().deleteRegion(regionalNoRateRegion);

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  receiver.resign(rti::NO_ACTION);
  noRateReceiver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  noRateReceiver.disconnect();
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
  auto const publisherTargetTwo = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const publisherTargetThree = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const publisherTargetFour = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  auto const publisherTargetFive = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  auto const thirdTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const fourthTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const fifthTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      14U);
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
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetTwo);
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
  publisher.rtiAmbassador().retract(suppressedRetraction);
  receiver.rtiAmbassador().timeAdvanceRequest(*thirdTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetThree);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 3U &&
            receiver.recorder().timeAdvanceGrants().size() >= 3U;
      },
      options,
      "Timestamped update-rate reduction suppressed passel grant");
  require(
      receiver.recorder().timedReflections().size() == 3U,
      "Timestamped update-rate reduction delivered an all-best-effort suppressed passel");
  require(
      receiver.recorder().timeAdvanceGrants().back().encoded == encodeTime(*thirdTime),
      "Timestamped update-rate reduction returned the wrong final receiver grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(suppressedRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "Timestamped update-rate reduction second suppressed-passel retraction");

  auto const reliableTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliableTransportation.isValid(),
      "Timestamped update-rate reduction reliable transport lookup was invalid");
  rti::AttributeHandleSet const changedTransportationAttributes{publisherBestEffort};
  publisher.rtiAmbassador().requestAttributeTransportationTypeChange(
      object,
      changedTransportationAttributes,
      reliableTransportation);
  waitFor(
      publisher,
      [&] {
        return publisher.recorder().attributeTransportationConfirmations().size() >= 1U;
      },
      options,
      "Timestamped update-rate reduction reliable transport-change confirmation");
  auto const transportConfirmations =
      publisher.recorder().attributeTransportationConfirmations();
  require(
      transportConfirmations.size() == 1U &&
          transportConfirmations.front().object == object &&
          transportConfirmations.front().attributes == changedTransportationAttributes &&
          publisher.rtiAmbassador().getTransportationTypeName(
              transportConfirmations.front().transportation) == L"HLAreliable",
      "Timestamped update-rate reduction did not confirm reliable transport for the changed attribute");

  auto const changedTypeFirstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      bestEffortOnly,
      tag,
      *fourthTime);
  require(
      changedTypeFirstRetraction.isValid(),
      "Timestamped update-rate reduction first post-change update returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*fourthTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetFour);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 4U &&
            receiver.recorder().timeAdvanceGrants().size() >= 4U &&
            receiver.recorder().timedReflections().size() >= 4U;
      },
      options,
      "Timestamped update-rate reduction first post-change reflection and grants");
  auto const fourthReflections = receiver.recorder().timedReflections();
  require(
      fourthReflections.size() == 4U && countAttribute(receiverReliable) == 2U &&
          countAttribute(receiverBestEffort) == 2U,
      "Timestamped update-rate reduction did not deliver the first post-change reliable-transport update");
  assertReflection(
      fourthReflections.back(),
      receiverBestEffort,
      bestEffortBytes,
      encodeTime(*fourthTime),
      copyBytes(changedTypeFirstRetraction.encode()),
      L"HLAreliable");

  auto const changedTypeSecondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      bestEffortOnly,
      tag,
      *fifthTime);
  require(
      changedTypeSecondRetraction.isValid(),
      "Timestamped update-rate reduction second post-change update returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*fifthTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetFive);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 5U &&
            receiver.recorder().timeAdvanceGrants().size() >= 5U &&
            receiver.recorder().timedReflections().size() >= 5U;
      },
      options,
      "Timestamped update-rate reduction second post-change reflection and grants");
  auto const fifthReflections = receiver.recorder().timedReflections();
  require(
      fifthReflections.size() == 5U && countAttribute(receiverReliable) == 2U &&
          countAttribute(receiverBestEffort) == 3U,
      "Timestamped update-rate reduction suppressed an update after the attribute became reliable");
  assertReflection(
      fifthReflections.back(),
      receiverBestEffort,
      bestEffortBytes,
      encodeTime(*fifthTime),
      copyBytes(changedTypeSecondRetraction.encode()),
      L"HLAreliable");

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

void scenarioUpdateRateWallClockSpacing(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Update-rate wall-clock spacing requires an adapter-supplied rich FOM model");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "update-rate-wall-clock-spacing");
  connectAndJoin(publisher, receiver, options, federation, options.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.rateBestEffortAttributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.rateBestEffortAttributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Update-rate wall-clock spacing lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      options.rateDesignatorName);

  auto const maximumRate = receiver.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(
      maximumRate > 0.0,
      "Update-rate wall-clock spacing could not resolve the subscribed maximum rate");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Update-rate wall-clock spacing registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Update-rate wall-clock spacing object discovery");
  require(
      receiver.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          receiverAttribute) == maximumRate,
      "Update-rate wall-clock spacing subscription did not select the named maximum rate");

  auto send = [&](std::uint8_t value) {
    std::vector<std::uint8_t> const bytes{value};
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(bytes.data(), bytes.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
  };

  send(0x11U);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflections().size() >= 1U; },
      options,
      "Update-rate wall-clock spacing first reflection");
  auto const firstReflections = receiver.recorder().reflections();
  require(
      firstReflections.size() == 1U && firstReflections.front().object == object &&
          firstReflections.front().values.size() == 1U &&
          firstReflections.front().values.count(receiverAttribute) == 1U &&
          copyBytes(firstReflections.front().values.at(receiverAttribute)) ==
              std::vector<std::uint8_t>{0x11U} &&
          firstReflections.front().callbackTime != Clock::time_point{},
      "Update-rate wall-clock spacing first callback did not record its public delivery time");

  auto const minimumInterval = std::chrono::duration<double>(1.0 / maximumRate);
  auto const earliestNextDelivery = firstReflections.front().callbackTime +
      std::chrono::duration_cast<Clock::duration>(minimumInterval);
  send(0x12U);

  auto verifyObservedSpacing = [&] {
    auto const reflections = receiver.recorder().reflections();
    for (auto const& reflection : reflections) {
      auto const bytes = reflection.values.count(receiverAttribute) == 1U
          ? copyBytes(reflection.values.at(receiverAttribute))
          : std::vector<std::uint8_t>{};
      require(
          reflection.object == object && reflection.values.size() == 1U &&
              (bytes == std::vector<std::uint8_t>{0x11U} ||
               bytes == std::vector<std::uint8_t>{0x12U}),
          "Update-rate wall-clock spacing received an unexpected object or value");
    }
    for (std::size_t index = 1U; index < reflections.size(); ++index) {
      auto const elapsed = std::chrono::duration<double>(
          reflections[index].callbackTime - reflections[index - 1U].callbackTime);
      require(
          elapsed >= minimumInterval,
          "Update-rate wall-clock spacing delivered a second message before 1/x seconds elapsed");
    }
  };

  while (Clock::now() < earliestNextDelivery) {
    receiver.pump();
    verifyObservedSpacing();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  receiver.pump();
  verifyObservedSpacing();

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioUpdateRateWallClockSpacingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUpdateRateWallClockSpacing(options, model);
}

void scenarioSynchronousUpdateRateMessageDrop(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Synchronous update-rate message dropping requires the evoked callback model");
  require(
      !options.modelFom.empty(),
      "Synchronous update-rate message dropping requires an adapter-supplied rich FOM model");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "synchronous-update-rate-message-drop");
  connectAndJoin(publisher, receiver, options, federation, options.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.rateBestEffortAttributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.rateBestEffortAttributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Synchronous update-rate message dropping lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      options.rateDesignatorName);

  auto const subscribedRate = receiver.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(
      subscribedRate >= 0.25,
      "Synchronous update-rate message dropping needs an adapter rate high enough for bounded rate measurement");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Synchronous update-rate message dropping registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Synchronous update-rate message dropping object discovery");
  require(
      receiver.rtiAmbassador().getUpdateRateValueForAttribute(
          object,
          receiverAttribute) == subscribedRate,
      "Synchronous update-rate message dropping subscription did not select the named rate");

  auto send = [&](std::uint32_t sequence) {
    std::vector<std::uint8_t> const bytes{
        static_cast<std::uint8_t>((sequence >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((sequence >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((sequence >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(sequence & 0xFFU)};
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(bytes.data(), bytes.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
  };

  auto const publisherInterval = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(1.0 / (3.0 * subscribedRate)));
  auto const evokeInterval = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(1.0 / (4.0 * subscribedRate)));
  require(
      publisherInterval > Clock::duration::zero() &&
          evokeInterval > Clock::duration::zero(),
      "Synchronous update-rate message dropping adapter rate exceeds clock pacing resolution");

  auto const windowSeconds = std::max(2.0, 10.0 / subscribedRate);
  auto const window = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(windowSeconds));
  auto const measurementStart = Clock::now();
  auto const measurementEnd = measurementStart + window;
  auto nextUpdate = measurementStart;
  auto nextEvoke = measurementStart;
  std::uint32_t sentCount = 0U;
  std::size_t evokeCount = 0U;
  while (Clock::now() < measurementEnd) {
    auto const now = Clock::now();
    if (now >= nextEvoke) {
      static_cast<void>(receiver.rtiAmbassador().evokeCallback(0.0));
      ++evokeCount;
      nextEvoke += evokeInterval;
      if (nextEvoke <= now) nextEvoke = now + evokeInterval;
    }
    if (now >= nextUpdate) {
      send(sentCount);
      ++sentCount;
      nextUpdate += publisherInterval;
      if (nextUpdate <= now) nextUpdate = now + publisherInterval;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const elapsed = std::chrono::duration<double>(measurementEnd - measurementStart);
  require(
      sentCount > 2U && evokeCount > 2U,
      "Synchronous update-rate message dropping did not produce enough paced service calls");
  auto const publisherRate = static_cast<double>(sentCount - 1U) / elapsed.count();
  auto const evokeRate = static_cast<double>(evokeCount - 1U) / elapsed.count();
  require(
      publisherRate > subscribedRate && evokeRate > subscribedRate,
      "Synchronous update-rate message dropping did not establish Rp > Rs and Re > Rs");

  auto const drainDeadline = Clock::now() +
      std::chrono::duration_cast<Clock::duration>(
          std::chrono::duration<double>(1.0 / subscribedRate));
  while (Clock::now() < drainDeadline) {
    static_cast<void>(receiver.rtiAmbassador().evokeCallback(0.0));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto const reflections = receiver.recorder().reflections();
  std::vector<bool> seen(sentCount, false);
  std::size_t deliveredDuringWindow = 0U;
  for (auto const& reflection : reflections) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U,
        "Synchronous update-rate message dropping received an unexpected reflection");
    auto const bytes = copyBytes(reflection.values.at(receiverAttribute));
    require(
        bytes.size() == 4U && reflection.callbackTime != Clock::time_point{},
        "Synchronous update-rate message dropping received malformed sequence evidence");
    auto const sequence =
        (static_cast<std::uint32_t>(bytes[0]) << 24U) |
        (static_cast<std::uint32_t>(bytes[1]) << 16U) |
        (static_cast<std::uint32_t>(bytes[2]) << 8U) |
        static_cast<std::uint32_t>(bytes[3]);
    require(
        sequence < sentCount && !seen[sequence],
        "Synchronous update-rate message dropping duplicated or invented an update");
    seen[sequence] = true;
    if (reflection.callbackTime <= measurementEnd) ++deliveredDuringWindow;
  }
  require(
      deliveredDuringWindow <= sentCount,
      "Synchronous update-rate message dropping delivered more updates than were sent");

  // publisherRate uses N-1 send intervals, whereas the observed drop count
  // includes all N sent updates. Add the initial send boundary so the
  // predicted rate difference and observed count use the same event window.
  auto const expectedDropCount =
      elapsed.count() * (publisherRate - subscribedRate) + 1.0;
  auto const observedDropCount = static_cast<double>(sentCount - deliveredDuringWindow);
  auto const dropCountDifference = std::abs(observedDropCount - expectedDropCount);
  auto const failureDetail =
      std::string{"Synchronous update-rate message dropping did not match the required "
                  "Rp-Rs drop rate within two boundary updates: expected="} +
      std::to_string(expectedDropCount) + ", observed=" +
      std::to_string(observedDropCount) + ", difference=" +
      std::to_string(dropCountDifference) + ", sent=" +
      std::to_string(sentCount) + ", delivered_during_window=" +
      std::to_string(deliveredDuringWindow) + ", elapsed_seconds=" +
      std::to_string(elapsed.count()) + ", publisher_rate=" +
      std::to_string(publisherRate) + ", subscribed_rate=" +
      std::to_string(subscribedRate) + ", evoke_rate=" +
      std::to_string(evokeRate);
  require(
      dropCountDifference <= 2.0,
      failureDetail);

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

void scenarioSynchronousUpdateRateMessageDropContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSynchronousUpdateRateMessageDrop(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
