#include "tck_joined_federate_mom_object_instances_deleted_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesDeletedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM object-instances-deleted testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session publisher(options, model, "mom-deleted-publisher");
  Session observer(options, model, "mom-deleted-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-deleted-mim-attribute");
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
      options.ownerFederateName + L"-mom-deleted-publisher",
      options.federateType,
      federation);

  auto const momClass = publisher.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAfederateHandle");
  auto const instancesDeletedAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAobjectInstancesDeleted");
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          instancesDeletedAttribute.isValid() && reliable.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          publisher.rtiAmbassador().getAttributeName(
              momClass,
              instancesDeletedAttribute) == L"HLAobjectInstancesDeleted" &&
          publisher.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM object-instances-deleted lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      instancesDeletedAttribute};
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  observer.join(
      options.memberFederateName + L"-mom-deleted-observer",
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
      "Joined-federate MOM object-instances-deleted application lookup did not round-trip");
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
  auto const thirdObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      firstObject.isValid() && secondObject.isValid() && thirdObject.isValid() &&
          firstObject != secondObject && firstObject != thirdObject &&
          secondObject != thirdObject,
      "Joined-federate MOM object-instances-deleted registration returned invalid or duplicate handles");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject) &&
            observer.recorder().hasDiscovery(thirdObject);
      },
      options,
      "joined-federate MOM object-instances-deleted application discovery");

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
      "joined-federate MOM object-instances-deleted self discovery");
  auto const publisherMomObject = findMomObject();
  require(
      publisherMomObject.has_value() &&
          publisher.rtiAmbassador().getKnownObjectClassHandle(
              *publisherMomObject) == momClass,
      "Joined-federate MOM object-instances-deleted discovery returned the wrong standard object");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *publisherMomObject &&
              reflection.values.count(instancesDeletedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.count(instancesDeletedAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(instancesDeletedAttribute));
    require(value.get() >= 0, description + " returned a negative deletion count");
    return value.get();
  };
  auto requestCount = [&](std::string const& description) {
    auto const before = publisher.recorder().reflections().size();
    publisher.rtiAmbassador().requestAttributeValueUpdate(
        *publisherMomObject,
        rti::AttributeHandleSet{instancesDeletedAttribute},
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
      requestCount("joined-federate MOM initial object-instances-deleted value") == 0,
      "Joined-federate MOM object-instances-deleted was nonzero before object deletion");

  auto deleteAndObserve = [&](rti::ObjectInstanceHandle const& object,
                              std::vector<std::uint8_t> const& tagBytes,
                              std::string const& description) {
    auto const before = observer.recorder().removals().size();
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().deleteObjectInstance(object, tag);
    if (model == rti::HLA_EVOKED) {
      require(
          observer.recorder().removals().size() == before,
          description + " delivered before callback servicing");
    }
    waitFor(
        observer,
        [&] {
          auto const removals = observer.recorder().removals();
          return std::any_of(
              removals.begin() + static_cast<std::ptrdiff_t>(
                                     std::min(before, removals.size())),
              removals.end(),
              [&](RemovalRecord const& removal) {
                return removal.object == object;
              });
        },
        options,
        description);
    auto const removals = observer.recorder().removals();
    auto const begin = removals.begin() + static_cast<std::ptrdiff_t>(
        std::min(before, removals.size()));
    auto const iterator = std::find_if(
        begin,
        removals.end(),
        [&](RemovalRecord const& removal) { return removal.object == object; });
    require(
        std::count_if(
            begin,
            removals.end(),
            [&](RemovalRecord const& removal) { return removal.object == object; }) ==
            1,
        description + " produced a missing or duplicate removal callback");
    require(
        iterator != removals.end() && iterator->tag == tagBytes &&
            iterator->producer == publisher.federateHandle(),
        description + " returned incorrect removal metadata");
  };

  deleteAndObserve(
      firstObject,
      std::vector<std::uint8_t>{0x61U},
      "joined-federate MOM first receive-order object deletion");
  require(
      requestCount("joined-federate MOM count after first deletion") == 1,
      "Joined-federate MOM object-instances-deleted did not count the first delete invocation");
  deleteAndObserve(
      secondObject,
      std::vector<std::uint8_t>{0x62U},
      "joined-federate MOM second receive-order object deletion");
  auto const directBaseline = requestCount(
      "joined-federate MOM direct object-instances-deleted baseline");
  require(
      directBaseline == 2,
      "Joined-federate MOM object-instances-deleted did not count two successful delete invocations");

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
  deleteAndObserve(
      thirdObject,
      std::vector<std::uint8_t>{0x63U},
      "joined-federate MOM periodic-phase receive-order object deletion");
  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = publisher.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *publisherMomObject ||
          iterator->values.count(instancesDeletedAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(instancesDeletedAttribute));
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
      "joined-federate MOM periodic object-instances-deleted value");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(
              *periodicReflection,
              "Joined-federate MOM periodic object-instances-deleted reflection") == 3,
      "Joined-federate MOM periodic object-instances-deleted did not equal successful deletion invocations");
  require(
      requestCount("joined-federate MOM direct value after periodic deletion") == 3,
      "Joined-federate MOM direct object-instances-deleted value disagreed with periodic reporting");

  sendTiming(0);
  require(
      requestCount("joined-federate MOM object-instances-deleted after disabling periodic reports") == 3,
      "Joined-federate MOM object-instances-deleted changed after periodic reporting was disabled");

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

void scenarioJoinedFederateMomObjectInstancesDeletedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesDeletedAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
