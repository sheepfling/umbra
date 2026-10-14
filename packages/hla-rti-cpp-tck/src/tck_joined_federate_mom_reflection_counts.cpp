#include "tck_joined_federate_mom_reflection_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomReflectionCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM reflection-count testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM reflection-count testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM reflection-count testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-reflection-counts");
  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  receiver.join(
      options.memberFederateName + L"-mom-reflection-count-receiver",
      options.federateType,
      federation);
  auto const receiverFederate = receiver.federateHandle();

  auto const momClass = receiver.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const reflectedObjectsAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAobjectInstancesReflected");
  auto const reflectionsReceivedAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAreflectionsReceived");
  auto const reliable = receiver.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          federateHandleAttribute.isValid() && reflectedObjectsAttribute.isValid() &&
          reflectionsReceivedAttribute.isValid() && reliable.isValid() &&
          receiver.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM reflection-count lookup did not resolve the standard surface");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      reflectedObjectsAttribute,
      reflectionsReceivedAttribute};
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  auto const findReceiverMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(receiverFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      receiver,
      [&] { return findReceiverMomReflection().has_value(); },
      options,
      "joined-federate MOM reflection-count self discovery");
  auto const initialMomReflection = findReceiverMomReflection();
  require(
      initialMomReflection.has_value() && initialMomReflection->object.isValid() &&
          receiver.rtiAmbassador().getKnownObjectClassHandle(
              initialMomReflection->object) == momClass,
      "Joined-federate MOM reflection-count discovery returned the wrong standard object");
  auto const receiverMomObject = initialMomReflection->object;

  auto const findCountReflection = [&](std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    if (before > reflections.size()) {
      return std::nullopt;
    }
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == receiverMomObject &&
              reflection.values.count(reflectedObjectsAttribute) == 1U &&
              reflection.values.count(reflectionsReceivedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCounts = [&](ReflectionRecord const& reflection,
                          std::string const& description) {
    require(
        reflection.values.size() == 2U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE reflectedObjects;
    rti::HLAinteger32BE reflectionsReceived;
    reflectedObjects.decode(reflection.values.at(reflectedObjectsAttribute));
    reflectionsReceived.decode(reflection.values.at(reflectionsReceivedAttribute));
    require(
        reflectedObjects.get() >= 0 && reflectionsReceived.get() >= 0,
        description + " returned a negative reflection count");
    return std::pair<std::int32_t, std::int32_t>{
        reflectedObjects.get(),
        reflectionsReceived.get()};
  };
  auto requestCounts = [&](std::string const& description) {
    auto const before = receiver.recorder().reflections().size();
    receiver.rtiAmbassador().requestAttributeValueUpdate(
        receiverMomObject,
        rti::AttributeHandleSet{
            reflectedObjectsAttribute,
            reflectionsReceivedAttribute},
        rti::VariableLengthData{});
    waitFor(
        receiver,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no count reflection");
    return decodeCounts(*reflection, description);
  };

  auto const initialCounts = requestCounts(
      "joined-federate MOM reflection-count initial direct request");
  require(
      initialCounts.first == 0 && initialCounts.second == 0,
      "Joined-federate MOM reflection-count initial values were not zero");

  publisher.join(
      options.ownerFederateName + L"-mom-reflection-count-publisher",
      options.federateType,
      federation);
  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  require(
      publisherClass.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          publisherAttribute.isValid() &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName,
      "Joined-federate MOM reflection-count adapter object lookup did not round-trip");
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      receiverClass.isValid() && receiverAttribute.isValid(),
      "Joined-federate MOM reflection-count receiver object lookup returned invalid handles");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      rti::AttributeHandleSet{publisherAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      rti::AttributeHandleSet{receiverAttribute},
      true,
      L"");

  auto const objectOne = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  auto const objectTwo = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      objectOne.isValid() && objectTwo.isValid(),
      "Joined-federate MOM reflection-count registration returned an invalid handle");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().hasDiscovery(objectOne) &&
            receiver.recorder().hasDiscovery(objectTwo);
      },
      options,
      "joined-federate MOM reflection-count object discovery");

  auto const applicationReflectionCount = [&](rti::ObjectInstanceHandle const& object) {
    auto const reflections = receiver.recorder().reflections();
    auto const timedReflections = receiver.recorder().timedReflections();
    auto const receiveOrderCount = static_cast<std::size_t>(std::count_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == object;
        }));
    auto const timestampedCount = static_cast<std::size_t>(std::count_if(
        timedReflections.begin(),
        timedReflections.end(),
        [&](TimedReflectionRecord const& reflection) {
          return reflection.object == object;
        }));
    return receiveOrderCount + timestampedCount;
  };
  auto sendReceiveOrderUpdate = [&](rti::ObjectInstanceHandle const& object,
                                    std::uint8_t value,
                                    std::size_t expectedReflections,
                                    std::string const& description) {
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{{
            publisherAttribute,
            rti::HLAbyte{static_cast<rti::Octet>(value)}.encode()}},
        rti::VariableLengthData{});
    waitFor(
        receiver,
        [&] { return applicationReflectionCount(object) >= expectedReflections; },
        options,
        description);
  };

  sendReceiveOrderUpdate(
      objectOne,
      0x31U,
      1U,
      "joined-federate MOM reflection-count first object reflection");
  auto const firstCounts = requestCounts(
      "joined-federate MOM reflection-count after first reflection");
  require(
      firstCounts.first == 1 && firstCounts.second == 1,
      "Joined-federate MOM reflection-count first values were incorrect");

  sendReceiveOrderUpdate(
      objectOne,
      0x32U,
      2U,
      "joined-federate MOM reflection-count repeated object reflection");
  auto const secondCounts = requestCounts(
      "joined-federate MOM reflection-count after repeated reflection");
  require(
      secondCounts.first == 1 && secondCounts.second == 2,
      "Joined-federate MOM reflection-count repeated-object values were incorrect");

  sendReceiveOrderUpdate(
      objectTwo,
      0x33U,
      1U,
      "joined-federate MOM reflection-count second object reflection");
  auto const thirdCounts = requestCounts(
      "joined-federate MOM reflection-count after second object");
  require(
      thirdCounts.first == 2 && thirdCounts.second == 3,
      "Joined-federate MOM reflection-count distinct-object values were incorrect");

  publisher.rtiAmbassador().changeAttributeOrderType(
      objectTwo,
      rti::AttributeHandleSet{publisherAttribute},
      rti::TIMESTAMP);
  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Joined-federate MOM reflection-count members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "joined-federate MOM reflection-count time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "joined-federate MOM reflection-count time-regulation callback");
  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      objectTwo,
      rti::AttributeHandleValueMap{{
          publisherAttribute,
          rti::HLAbyte{0x34U}.encode()}},
      rti::VariableLengthData{},
      *timestamp);
  require(
      retraction.isValid(),
      "Joined-federate MOM reflection-count timestamped update returned no retraction");
  auto const receiverTimestamp = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      2U);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTimestamp);
  publisher.rtiAmbassador().timeAdvanceRequest(*timestamp);
  waitFor(
      receiver,
      publisher,
      [&] {
        return receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            publisher.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "joined-federate MOM reflection-count timestamped grants");
  waitFor(
      receiver,
      [&] { return applicationReflectionCount(objectTwo) >= 2U; },
      options,
      "joined-federate MOM reflection-count timestamped reflection");
  auto const afterTimestampedCounts = requestCounts(
      "joined-federate MOM reflection-count after timestamped reflection");
  require(
      afterTimestampedCounts.first == 2 && afterTimestampedCounts.second == 4,
      "Joined-federate MOM reflection-count timestamped values were incorrect");

  auto const setTiming = receiver.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          receiver.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          receiver.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM reflection-count HLAsetTiming lookup did not round-trip");
  auto const beforePeriodic = receiver.recorder().reflections().size();
  receiver.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, receiverFederate.encode()},
          {reportPeriod, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      receiver,
      [&] { return findCountReflection(beforePeriodic).has_value(); },
      options,
      "joined-federate MOM reflection-count periodic reflection");
  auto const periodicReflection = findCountReflection(beforePeriodic);
  require(
      periodicReflection.has_value(),
      "Joined-federate MOM reflection-count periodic report was missing");
  auto const periodicCounts = decodeCounts(
      *periodicReflection,
      "joined-federate MOM reflection-count periodic report");
  require(
      periodicCounts.first == 2 && periodicCounts.second == 4,
      "Joined-federate MOM reflection-count periodic values were incorrect");
  receiver.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, receiverFederate.encode()},
          {reportPeriod, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      rti::AttributeHandleSet{receiverAttribute});
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      rti::AttributeHandleSet{publisherAttribute});
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail

