#include "tck_joined_federate_mom_reflections_received_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomReflectionsReceivedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM reflections-received testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session publisher(options, model, "mom-reflections-received-publisher");
  Session subject(options, model, "mom-reflections-received-subject");
  Session monitor(options, model, "mom-reflections-received-monitor");
  auto const federation = federationName(
      options,
      "joined-federate-mom-reflections-received-mim-attribute");
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
      options.ownerFederateName + L"-mom-reflections-received-publisher",
      options.federateType,
      federation);
  monitor.join(
      options.memberFederateName + L"-mom-reflections-received-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const reflectionsReceivedAttribute =
      monitor.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAreflectionsReceived");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          reflectionsReceivedAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              reflectionsReceivedAttribute) == L"HLAreflectionsReceived" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM reflections-received lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          reflectionsReceivedAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-mom-reflections-received-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
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
      "Joined-federate MOM reflections-received application lookup did not round-trip");
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
      "Joined-federate MOM reflections-received registration returned invalid or duplicate handles");
  waitFor(
      subject,
      [&] {
        return subject.recorder().hasDiscovery(firstObject) &&
            subject.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM reflections-received application discovery");

  auto findSubjectMomObject = [&]() -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
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
      "joined-federate MOM reflections-received subject discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Joined-federate MOM reflections-received discovery returned the wrong standard object");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *subjectMomObject &&
              reflection.values.count(reflectionsReceivedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(reflectionsReceivedAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(reflectionsReceivedAttribute));
    require(value.get() >= 0, description + " returned a negative reflection count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{reflectionsReceivedAttribute},
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
  auto const baseline = requestCount(
      "joined-federate MOM reflections-received baseline");
  require(
      baseline <= std::numeric_limits<std::int32_t>::max() - 3,
      "Joined-federate MOM reflections-received baseline cannot safely accommodate the tested callbacks");

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
                        std::int32_t expectedDelta,
                        std::string const& description) {
    auto const before = subject.recorder().reflections().size();
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{{
            publisherAttribute,
            rti::HLAbyte{payload}.encode()}},
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
    require(
        requestCount(description + " reflected-count direct request") ==
            baseline + expectedDelta,
        description + " did not increment HLAreflectionsReceived exactly once");
  };

  sendUpdate(
      firstObject,
      static_cast<rti::Octet>(0x61U),
      1,
      "joined-federate MOM first receive-order reflection");
  sendUpdate(
      firstObject,
      static_cast<rti::Octet>(0x62U),
      2,
      "joined-federate MOM repeated reflection of the same object");
  sendUpdate(
      secondObject,
      static_cast<rti::Octet>(0x63U),
      3,
      "joined-federate MOM reflection of a second object");

  auto const setTiming = monitor.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const federateParameter = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriodParameter = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && federateParameter.isValid() &&
          reportPeriodParameter.isValid() &&
          monitor.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          monitor.rtiAmbassador().getParameterName(
              setTiming,
              federateParameter) == L"HLAfederate" &&
          monitor.rtiAmbassador().getParameterName(
              setTiming,
              reportPeriodParameter) == L"HLAreportPeriod",
      "Joined-federate MOM reflections-received HLAsetTiming lookup did not round-trip");
  auto sendTiming = [&](std::int32_t period) {
    monitor.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {federateParameter, subjectFederate.encode()},
            {reportPeriodParameter, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };
  auto const periodicStart = monitor.recorder().reflections().size();
  sendTiming(1);
  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(periodicStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *subjectMomObject ||
          iterator->values.count(reflectionsReceivedAttribute) != 1U) {
        continue;
      }
      if (decodeCount(
              *iterator,
              "Joined-federate MOM periodic reflections-received count") ==
          baseline + 3) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      monitor,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic reflections-received value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(
              *periodicReflection,
              "Joined-federate MOM periodic reflections-received report") ==
              baseline + 3,
      "Joined-federate MOM periodic HLAreflectionsReceived did not preserve the three committed callbacks");
  sendTiming(0);
  require(
      requestCount(
          "joined-federate MOM direct value after periodic reporting was disabled") ==
          baseline + 3,
      "Joined-federate MOM direct HLAreflectionsReceived disagreed after periodic reporting was disabled");

  subject.rtiAmbassador().unsubscribeObjectClassAttributes(
      subjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  monitor.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          reflectionsReceivedAttribute});
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

void scenarioJoinedFederateMomReflectionsReceivedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomReflectionsReceivedAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
