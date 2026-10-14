#include "tck_joined_federate_mom_object_instances_reflected_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesReflectedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM object-instances-reflected testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session publisher(options, model, "mom-reflected-publisher");
  Session subject(options, model, "mom-reflected-subject");
  Session monitor(options, model, "mom-reflected-monitor");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-reflected-mim-attribute");
  publisher.connect();
  subject.connect();
  monitor.connect();
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
      options.ownerFederateName + L"-mom-reflected-publisher",
      options.federateType,
      federation);
  monitor.join(
      options.memberFederateName + L"-mom-reflected-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const instancesReflectedAttribute =
      monitor.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAobjectInstancesReflected");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          instancesReflectedAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              instancesReflectedAttribute) == L"HLAobjectInstancesReflected" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM object-instances-reflected lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      instancesReflectedAttribute};
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-mom-reflected-subject",
      options.federateType,
      federation);
  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subjectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const subjectAttribute = subject.rtiAmbassador().getAttributeHandle(
      subjectClass,
      options.attributeName);
  require(
      publisherClass.isValid() && subjectClass == publisherClass &&
          publisherAttribute.isValid() && subjectAttribute == publisherAttribute &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          subject.rtiAmbassador().getObjectClassName(subjectClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName &&
          subject.rtiAmbassador().getAttributeName(
              subjectClass,
              subjectAttribute) == options.attributeName,
      "Joined-federate MOM object-instances-reflected application lookup did not round-trip");
  rti::AttributeHandleSet const applicationAttributes{publisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      applicationAttributes);
  subject.rtiAmbassador().subscribeObjectClassAttributes(
      subjectClass,
      rti::AttributeHandleSet{subjectAttribute},
      true,
      L"");

  auto const firstObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  auto const secondObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      firstObject.isValid() && secondObject.isValid() &&
          firstObject != secondObject,
      "Joined-federate MOM object-instances-reflected registration returned invalid or duplicate handles");
  waitFor(
      subject,
      [&] {
        return subject.recorder().hasDiscovery(firstObject) &&
            subject.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM object-instances-reflected application discovery");

  auto findSubjectMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subject.federateHandle().encode());
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
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "joined-federate MOM object-instances-reflected subject discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(
              *subjectMomObject) == momClass,
      "Joined-federate MOM object-instances-reflected discovery returned the wrong standard object");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *subjectMomObject &&
              reflection.values.count(instancesReflectedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(instancesReflectedAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(instancesReflectedAttribute));
    require(value.get() >= 0, description + " returned a negative object count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{instancesReflectedAttribute},
        rti::VariableLengthData{});
    waitFor(
        monitor,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no scalar value");
    return decodeCount(*reflection, description);
  };

  require(
      requestCount("joined-federate MOM initial object-instances-reflected value") == 0,
      "Joined-federate MOM object-instances-reflected was nonzero before application reflections");

  auto findApplicationReflection = [&](std::size_t firstReflection,
                                       rti::ObjectInstanceHandle const& object,
                                       rti::Octet payload)
      -> std::optional<ReflectionRecord> {
    auto const reflections = subject.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const expectedValue = copyBytes(rti::HLAbyte{payload}.encode());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(subjectAttribute);
          return reflection.present && reflection.object == object &&
              value != reflection.values.end() &&
              copyBytes(value->second) == expectedValue;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto countApplicationReflections = [&](std::size_t firstReflection,
                                         rti::ObjectInstanceHandle const& object,
                                         rti::Octet payload) {
    auto const reflections = subject.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const expectedValue = copyBytes(rti::HLAbyte{payload}.encode());
    return static_cast<std::size_t>(std::count_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(subjectAttribute);
          return reflection.present && reflection.object == object &&
              value != reflection.values.end() &&
              copyBytes(value->second) == expectedValue;
        }));
  };
  auto sendUpdate = [&](rti::ObjectInstanceHandle const& object,
                        rti::Octet payload,
                        std::string const& description) {
    auto const before = subject.recorder().reflections().size();
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{
            {publisherAttribute, rti::HLAbyte{payload}.encode()}},
        rti::VariableLengthData{});
    waitFor(
        subject,
        [&] {
          return findApplicationReflection(before, object, payload).has_value();
        },
        options,
        description);
    auto const reflection = findApplicationReflection(before, object, payload);
    require(
        reflection.has_value() &&
            countApplicationReflections(before, object, payload) == 1U &&
            reflection->values.size() == 1U &&
            reflection->transportation == reliable && reflection->tag.empty() &&
            reflection->producer == publisher.federateHandle() &&
            !reflection->regions.has_value(),
        description + " returned missing, duplicate, or non-standard reflection data");
  };

  sendUpdate(
      firstObject,
      static_cast<rti::Octet>(0x51U),
      "joined-federate MOM first object reflection");
  require(
      requestCount("joined-federate MOM count after first object reflection") == 1,
      "Joined-federate MOM object-instances-reflected did not count the first reflected object");
  sendUpdate(
      firstObject,
      static_cast<rti::Octet>(0x52U),
      "joined-federate MOM repeated reflection for first object");
  require(
      requestCount("joined-federate MOM count after repeated first-object reflection") == 1,
      "Joined-federate MOM object-instances-reflected counted repeated reflections of one object more than once");
  sendUpdate(
      secondObject,
      static_cast<rti::Octet>(0x53U),
      "joined-federate MOM reflection for second object");
  auto const directBaseline = requestCount(
      "joined-federate MOM direct object-instances-reflected baseline");
  require(
      directBaseline == 2,
      "Joined-federate MOM object-instances-reflected did not count two distinct reflected objects");

  auto const setTiming = monitor.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          monitor.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          monitor.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          monitor.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM HLAsetTiming lookup did not round-trip");
  auto sendTiming = [&](std::int32_t period) {
    monitor.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, subject.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::size_t const periodicReflectionStart =
      monitor.recorder().reflections().size();
  sendTiming(1);
  auto const thirdObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      thirdObject.isValid() && thirdObject != firstObject &&
          thirdObject != secondObject,
      "Joined-federate MOM object-instances-reflected third registration returned an invalid or duplicate handle");
  waitFor(
      subject,
      [&] { return subject.recorder().hasDiscovery(thirdObject); },
      options,
      "joined-federate MOM third application object discovery");
  sendUpdate(
      thirdObject,
      static_cast<rti::Octet>(0x54U),
      "joined-federate MOM first periodic-phase reflection for third object");
  sendUpdate(
      thirdObject,
      static_cast<rti::Octet>(0x55U),
      "joined-federate MOM repeated periodic-phase reflection for third object");

  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *subjectMomObject ||
          iterator->values.count(instancesReflectedAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(instancesReflectedAttribute));
      if (value.get() >= 3) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      monitor,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic object-instances-reflected value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(
              *periodicReflection,
              "Joined-federate MOM periodic object-instances-reflected report") == 3,
      "Joined-federate MOM periodic object-instances-reflected did not count three distinct reflected objects");
  require(
      requestCount("joined-federate MOM direct value after periodic reflections") == 3,
      "Joined-federate MOM direct object-instances-reflected value disagreed with periodic reporting");

  sendTiming(0);
  require(
      requestCount("joined-federate MOM object-instances-reflected after disabling periodic reports") == 3,
      "Joined-federate MOM object-instances-reflected changed after periodic reporting was disabled");

  subject.rtiAmbassador().unsubscribeObjectClassAttributes(
      subjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  monitor.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      applicationAttributes);
  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  subject.disconnect();
  monitor.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesReflectedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesReflectedAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
