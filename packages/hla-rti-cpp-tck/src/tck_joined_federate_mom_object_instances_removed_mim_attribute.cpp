#include "tck_joined_federate_mom_object_instances_removed_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesRemovedAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM object-instances-removed testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session owner(options, model, "mom-removed-owner");
  Session observer(options, model, "mom-removed-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-removed-mim-attribute");
  owner.connect();
  observer.connect();
  if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  owner.join(
      options.ownerFederateName + L"-mom-removed-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-removed-observer",
      options.federateType,
      federation);
  auto const ownerFederate = owner.federateHandle();
  auto const observerFederate = observer.federateHandle();

  auto const momClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAfederateHandle");
  auto const instancesRemovedAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAobjectInstancesRemoved");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          instancesRemovedAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observer.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              momClass,
              instancesRemovedAttribute) == L"HLAobjectInstancesRemoved" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM object-instances-removed lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      instancesRemovedAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  auto findObserverMomReflection = [&]()
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedHandle = copyBytes(observerFederate.encode());
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
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findObserverMomReflection().has_value(); },
      options,
      "joined-federate MOM object-instances-removed self discovery");
  auto const observerMomReflection = findObserverMomReflection();
  require(
      observerMomReflection.has_value() &&
          observerMomReflection->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              observerMomReflection->object) == momClass,
      "Joined-federate MOM object-instances-removed discovery returned the wrong standard object");
  auto const observerMomObject = observerMomReflection->object;

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && observerClass == ownerClass &&
          ownerAttribute.isValid() && observerAttribute == ownerAttribute &&
          owner.rtiAmbassador().getObjectClassName(ownerClass) ==
              options.objectClassName &&
          observer.rtiAmbassador().getObjectClassName(observerClass) ==
              options.objectClassName &&
          owner.rtiAmbassador().getAttributeName(ownerClass, ownerAttribute) ==
              options.attributeName &&
          observer.rtiAmbassador().getAttributeName(
              observerClass,
              observerAttribute) == options.attributeName,
      "Joined-federate MOM object-instances-removed application lookup did not round-trip");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");

  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const thirdObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      firstObject.isValid() && secondObject.isValid() && thirdObject.isValid() &&
          firstObject != secondObject && firstObject != thirdObject &&
          secondObject != thirdObject,
      "Joined-federate MOM object-instances-removed registration returned invalid or duplicate handles");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject) &&
            observer.recorder().hasDiscovery(thirdObject);
      },
      options,
      "joined-federate MOM object-instances-removed application discovery");

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == observerMomObject &&
              reflection.values.count(instancesRemovedAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCount = [&](ReflectionRecord const& reflection,
                         std::string const& description) {
    require(
        reflection.values.size() == 1U &&
            reflection.values.count(instancesRemovedAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(instancesRemovedAttribute));
    require(value.get() >= 0, description + " returned a negative removal count");
    return value.get();
  };
  auto requestRemovedCount = [&](std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        observerMomObject,
        rti::AttributeHandleSet{instancesRemovedAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no scalar count value");
    return decodeCount(*reflection, description);
  };
  auto const baseline = requestRemovedCount(
      "joined-federate MOM object-instances-removed baseline");
  require(
      baseline <= std::numeric_limits<std::int32_t>::max() - 3,
      "Joined-federate MOM removal baseline cannot safely accommodate the tested callbacks");

  auto removeAndObserve = [&](rti::ObjectInstanceHandle const& object,
                              std::vector<std::uint8_t> const& tagBytes,
                              std::string const& description) {
    auto const before = observer.recorder().removals().size();
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    owner.rtiAmbassador().deleteObjectInstance(object, tag);
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
            iterator->producer == ownerFederate,
        description + " returned incorrect removal metadata");
  };

  removeAndObserve(
      firstObject,
      std::vector<std::uint8_t>{0x71U},
      "joined-federate MOM first receive-order object removal");
  require(
      requestRemovedCount(
          "joined-federate MOM count after first removal") == baseline + 1,
      "Joined-federate MOM object-instances-removed did not count the first committed removal callback");
  removeAndObserve(
      secondObject,
      std::vector<std::uint8_t>{0x72U},
      "joined-federate MOM second receive-order object removal");
  require(
      requestRemovedCount(
          "joined-federate MOM count after second removal") == baseline + 2,
      "Joined-federate MOM object-instances-removed did not count the second committed removal callback");

  auto const setTiming = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const federateParameter = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriodParameter = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && federateParameter.isValid() &&
          reportPeriodParameter.isValid() &&
          observer.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          observer.rtiAmbassador().getParameterName(
              setTiming,
              federateParameter) == L"HLAfederate" &&
          observer.rtiAmbassador().getParameterName(
              setTiming,
              reportPeriodParameter) == L"HLAreportPeriod",
      "Joined-federate MOM HLAsetTiming lookup did not round-trip");
  auto sendTiming = [&](std::int32_t period) {
    observer.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {federateParameter, observerFederate.encode()},
            {reportPeriodParameter, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  auto const periodicStart = observer.recorder().reflections().size();
  sendTiming(1);
  removeAndObserve(
      thirdObject,
      std::vector<std::uint8_t>{0x73U},
      "joined-federate MOM periodic-phase receive-order object removal");
  auto findPeriodicCount = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(periodicStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != observerMomObject ||
          iterator->values.count(instancesRemovedAttribute) != 1U) {
        continue;
      }
      if (decodeCount(*iterator, "Joined-federate MOM periodic removal count") ==
          baseline + 3) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      observer,
      [&] { return findPeriodicCount().has_value(); },
      options,
      "joined-federate MOM periodic object-instances-removed reflection");
  auto const periodicReflection = findPeriodicCount();
  require(
      periodicReflection.has_value() &&
          decodeCount(
              *periodicReflection,
              "Joined-federate MOM periodic object-instances-removed reflection") ==
              baseline + 3,
      "Joined-federate MOM periodic object-instances-removed value did not preserve the committed callback total");
  sendTiming(0);
  require(
      requestRemovedCount(
          "joined-federate MOM direct value after periodic removal reporting was disabled") ==
          baseline + 3,
      "Joined-federate MOM direct value disagreed after periodic removal reporting was disabled");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      observerAttributes);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      momAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomObjectInstancesRemovedAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesRemovedAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
