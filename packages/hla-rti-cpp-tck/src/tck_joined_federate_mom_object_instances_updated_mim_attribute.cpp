#include "tck_joined_federate_mom_object_instances_updated_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesUpdatedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM object-instances-updated testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session publisher(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-updated-mim-attribute");
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
      options.ownerFederateName + L"-mom-object-instances-updated-publisher",
      options.federateType,
      federation);

  auto const momClass = publisher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAfederateHandle");
  auto const instancesUpdatedAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAobjectInstancesUpdated");
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          instancesUpdatedAttribute.isValid() && reliable.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              instancesUpdatedAttribute) == L"HLAobjectInstancesUpdated" &&
          publisher.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM object-instances-updated lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      instancesUpdatedAttribute};
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  observer.join(
      options.memberFederateName + L"-mom-object-instances-updated-observer",
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
      "Joined-federate MOM object-instances-updated application lookup did not round-trip");
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
      firstObject.isValid() && secondObject.isValid() &&
          firstObject != secondObject,
      "Joined-federate MOM object-instances-updated registration returned invalid or duplicate handles");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM object-instances-updated application discovery");

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
      "joined-federate MOM object-instances-updated self discovery");
  auto const publisherMomObject = findMomObject();
  require(
      publisherMomObject.has_value() &&
          publisher.rtiAmbassador().getKnownObjectClassHandle(
              *publisherMomObject) == momClass,
      "Joined-federate MOM object-instances-updated discovery returned the wrong object class");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *publisherMomObject &&
              reflection.values.count(instancesUpdatedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(instancesUpdatedAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(instancesUpdatedAttribute));
    require(value.get() >= 0, description + " returned a negative object count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = publisher.recorder().reflections().size();
    publisher.rtiAmbassador().requestAttributeValueUpdate(
        *publisherMomObject,
        rti::AttributeHandleSet{instancesUpdatedAttribute},
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

  require(
      requestCount("joined-federate MOM initial object-instances-updated value") == 0,
      "Joined-federate MOM object-instances-updated was nonzero before application updates");

  auto findUpdateReflection = [&](std::size_t firstReflection,
                                  rti::ObjectInstanceHandle const& object,
                                  rti::Octet payload)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const expectedValue = copyBytes(rti::HLAbyte{payload}.encode());
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
                                    rti::Octet payload) {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const expectedValue = copyBytes(rti::HLAbyte{payload}.encode());
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
                        rti::Octet payload,
                        std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{
            {publisherAttribute, rti::HLAbyte{payload}.encode()}},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return findUpdateReflection(before, object, payload).has_value(); },
        options,
        description);
    auto const reflection = findUpdateReflection(before, object, payload);
    require(
        reflection.has_value() &&
            countUpdateReflections(before, object, payload) == 1U &&
            reflection->values.size() == 1U &&
            reflection->transportation == reliable && reflection->tag.empty() &&
            reflection->producer == publisher.federateHandle() &&
            !reflection->regions.has_value(),
        description + " returned missing, duplicate, or non-standard reflection data");
  };

  sendUpdate(
      firstObject,
      static_cast<rti::Octet>(0x41U),
      "joined-federate MOM first object update");
  require(
      requestCount("joined-federate MOM count after first object update") == 1,
      "Joined-federate MOM object-instances-updated did not count the first updated object");
  sendUpdate(
      firstObject,
      static_cast<rti::Octet>(0x42U),
      "joined-federate MOM repeated update to first object");
  require(
      requestCount("joined-federate MOM count after repeated first-object update") == 1,
      "Joined-federate MOM object-instances-updated counted repeated updates to one object more than once");
  sendUpdate(
      secondObject,
      static_cast<rti::Octet>(0x43U),
      "joined-federate MOM update to second object");
  auto const directBaseline = requestCount(
      "joined-federate MOM direct object-instances-updated baseline");
  require(
      directBaseline == 2,
      "Joined-federate MOM object-instances-updated did not count two distinct updated objects");

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
      "Joined-federate MOM HLAsetTiming lookup did not round-trip");
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
  auto const thirdObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      thirdObject.isValid() && thirdObject != firstObject &&
          thirdObject != secondObject,
      "Joined-federate MOM object-instances-updated third registration returned an invalid or duplicate handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(thirdObject); },
      options,
      "joined-federate MOM third application object discovery");
  sendUpdate(
      thirdObject,
      static_cast<rti::Octet>(0x44U),
      "joined-federate MOM first periodic-phase update to third object");
  sendUpdate(
      thirdObject,
      static_cast<rti::Octet>(0x45U),
      "joined-federate MOM repeated periodic-phase update to third object");

  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *publisherMomObject ||
          iterator->values.count(instancesUpdatedAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(instancesUpdatedAttribute));
      if (value.get() >= 3) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      publisher,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic object-instances-updated value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(
              *periodicReflection,
              "Joined-federate MOM periodic object-instances-updated reflection") == 3,
      "Joined-federate MOM periodic object-instances-updated did not count three distinct updated objects");
  require(
      requestCount("joined-federate MOM direct value after periodic object updates") == 3,
      "Joined-federate MOM direct object-instances-updated value disagreed with periodic reporting");

  sendTiming(0);
  require(
      requestCount("joined-federate MOM object-instances-updated after disabling periodic reports") == 3,
      "Joined-federate MOM object-instances-updated changed after periodic reporting was disabled");

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

void scenarioJoinedFederateMomObjectInstancesUpdatedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesUpdatedAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
