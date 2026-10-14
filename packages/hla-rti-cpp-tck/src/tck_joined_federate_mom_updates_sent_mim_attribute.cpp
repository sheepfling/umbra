#include "tck_joined_federate_mom_updates_sent_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomUpdatesSentAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM updates-sent testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session publisher(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-updates-sent-mim-attribute");
  publisher.connect();
  observer.connect();
  if (options.logicalTimeImplementationName.empty()) {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  publisher.join(
      options.ownerFederateName + L"-mom-updates-sent-publisher",
      options.federateType,
      federation);

  auto const momClass = publisher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = publisher.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const updatesSentAttribute = publisher.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAupdatesSent");
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          updatesSentAttribute.isValid() && reliable.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              updatesSentAttribute) == L"HLAupdatesSent" &&
          publisher.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM updates-sent lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      updatesSentAttribute};
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  observer.join(
      options.memberFederateName + L"-mom-updates-sent-observer",
      options.federateType,
      federation);
  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      publisherClass.isValid() && observerClass == publisherClass &&
          publisherAttribute.isValid() && observerAttribute == publisherAttribute &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          observer.rtiAmbassador().getObjectClassName(observerClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName &&
          observer.rtiAmbassador().getAttributeName(
              observerClass,
              observerAttribute) == options.attributeName,
      "Joined-federate MOM updates-sent application lookup did not round-trip");
  rti::AttributeHandleSet const applicationAttributes{publisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      applicationAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute},
      true,
      L"");

  auto const firstObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  auto const secondObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      firstObject.isValid() && secondObject.isValid() && firstObject != secondObject,
      "Joined-federate MOM updates-sent registration returned invalid or duplicate handles");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM updates-sent application discovery");

  auto findMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = publisher.recorder().reflections();
    auto const expectedHandle = copyBytes(publisher.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      publisher,
      [&] { return findMomObject().has_value(); },
      options,
      "joined-federate MOM updates-sent self discovery");
  auto const publisherMomObject = findMomObject();
  require(
      publisherMomObject.has_value() &&
          publisher.rtiAmbassador().getKnownObjectClassHandle(
              *publisherMomObject) == momClass,
      "Joined-federate MOM updates-sent discovery returned the wrong object class");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *publisherMomObject &&
              reflection.values.count(updatesSentAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(updatesSentAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(updatesSentAttribute));
    require(value.get() >= 0, description + " returned a negative update count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = publisher.recorder().reflections().size();
    publisher.rtiAmbassador().requestAttributeValueUpdate(
        *publisherMomObject,
        rti::AttributeHandleSet{updatesSentAttribute},
        rti::VariableLengthData{});
    waitFor(
        publisher,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no scalar value");
    return decodeCount(*reflection, description);
  };

  auto const initialCount = requestCount(
      "joined-federate MOM initial updates-sent value");
  require(
      initialCount == 0,
      "Joined-federate MOM updates-sent was nonzero before application updates");

  auto findUpdateReflection = [&](std::size_t firstReflection,
                                  rti::ObjectInstanceHandle const& object,
                                  std::vector<std::uint8_t> const& payload)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const expectedValue = copyBytes(
        rti::HLAbyte{static_cast<rti::Octet>(payload.front())}.encode());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerAttribute);
          return reflection.present && reflection.object == object &&
              value != reflection.values.end() &&
              copyBytes(value->second) == expectedValue;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto countUpdateReflections = [&](std::size_t firstReflection,
                                    rti::ObjectInstanceHandle const& object,
                                    std::vector<std::uint8_t> const& payload) {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const expectedValue = copyBytes(
        rti::HLAbyte{static_cast<rti::Octet>(payload.front())}.encode());
    return static_cast<std::size_t>(std::count_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerAttribute);
          return reflection.present && reflection.object == object &&
              value != reflection.values.end() &&
              copyBytes(value->second) == expectedValue;
        }));
  };
  auto sendUpdate = [&](rti::ObjectInstanceHandle const& object,
                        std::vector<std::uint8_t> const& payload,
                        std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    auto const encoded = rti::HLAbyte{
        static_cast<rti::Octet>(payload.front())}.encode();
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{{publisherAttribute, encoded}},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return findUpdateReflection(before, object, payload).has_value(); },
        options,
        description);
    auto const reflection = findUpdateReflection(before, object, payload);
    require(
        reflection.has_value() && countUpdateReflections(before, object, payload) == 1U &&
            reflection->values.size() == 1U &&
            reflection->transportation == reliable && reflection->tag.empty() &&
            reflection->producer == publisher.federateHandle() &&
            !reflection->regions.has_value(),
        description + " returned missing, duplicate, or non-standard reflection data");
  };

  std::vector<std::uint8_t> const firstPayload{0x61U};
  std::vector<std::uint8_t> const repeatedPayload{0x62U};
  std::vector<std::uint8_t> const secondObjectPayload{0x63U};
  sendUpdate(
      firstObject,
      firstPayload,
      "joined-federate MOM first receive-order attribute update");
  require(
      requestCount("joined-federate MOM updates-sent after first invocation") == 1,
      "Joined-federate MOM updates-sent did not count the first update invocation");
  sendUpdate(
      firstObject,
      repeatedPayload,
      "joined-federate MOM repeated receive-order update to one object");
  require(
      requestCount("joined-federate MOM updates-sent after repeated invocation") == 2,
      "Joined-federate MOM updates-sent did not count a repeated update invocation");
  sendUpdate(
      secondObject,
      secondObjectPayload,
      "joined-federate MOM receive-order update to a second object");
  auto const directBaseline = requestCount(
      "joined-federate MOM updates-sent direct baseline before periodic updates");
  require(
      directBaseline == 3,
      "Joined-federate MOM updates-sent did not count three successful invocations");

  auto const setTiming = publisher.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = publisher.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = publisher.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          publisher.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          publisher.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          publisher.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM updates-sent HLAsetTiming lookup did not round-trip");
  auto sendTiming = [&](std::int32_t period) {
    publisher.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, publisher.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::size_t const periodicReflectionStart =
      publisher.recorder().reflections().size();
  sendTiming(1);
  require(
      requestCount("joined-federate MOM updates-sent after enabling periodic reports") ==
          directBaseline,
      "Joined-federate MOM HLAsetTiming changed the updates-sent count");
  std::vector<std::uint8_t> const periodicFirstPayload{0x64U};
  std::vector<std::uint8_t> const periodicSecondPayload{0x65U};
  sendUpdate(
      firstObject,
      periodicFirstPayload,
      "joined-federate MOM first periodic-phase receive-order update");
  sendUpdate(
      secondObject,
      periodicSecondPayload,
      "joined-federate MOM second periodic-phase receive-order update");
  auto const expectedPeriodicCount = directBaseline + 2;

  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *publisherMomObject ||
          iterator->values.count(updatesSentAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(updatesSentAttribute));
      if (value.get() >= expectedPeriodicCount) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      publisher,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic updates-sent value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(*periodicReflection,
                      "Joined-federate MOM periodic updates-sent reflection") ==
              expectedPeriodicCount,
      "Joined-federate MOM periodic updates-sent did not equal successful update invocations");
  require(
      requestCount("joined-federate MOM direct updates-sent after periodic-phase updates") ==
          expectedPeriodicCount,
      "Joined-federate MOM direct updates-sent value did not include both periodic-phase invocations");

  sendTiming(0);
  require(
      requestCount("joined-federate MOM updates-sent after disabling periodic reports") ==
          expectedPeriodicCount,
      "Joined-federate MOM updates-sent changed after periodic reporting was disabled");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute});
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      applicationAttributes);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  observer.disconnect();
}

void scenarioJoinedFederateMomUpdatesSentAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomUpdatesSentAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
