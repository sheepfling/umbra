#include "tck_joined_federate_mom_object_instances_discovered_mim_attribute.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectInstancesDiscoveredAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Joined-federate MOM object-instances-discovered testing requires adapter-supplied FOM, standard MIM, object class, and attribute names");

  Session owner(options, model, "mom-discovered-owner");
  Session observer(options, model, "mom-discovered-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-object-instances-discovered-mim-attribute");
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
      options.ownerFederateName + L"-mom-discovered-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-discovered-observer",
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
  auto const instancesDiscoveredAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAobjectInstancesDiscovered");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          instancesDiscoveredAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observer.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              momClass,
              instancesDiscoveredAttribute) == L"HLAobjectInstancesDiscovered" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM object-instances-discovered lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      instancesDiscoveredAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momAttributes,
      true,
      L"");

  auto findMomReflection = [&](rti::FederateHandle const& federate)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedHandle = copyBytes(federate.encode());
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
      [&] {
        return findMomReflection(observerFederate).has_value() &&
            findMomReflection(ownerFederate).has_value();
      },
      options,
      "joined-federate MOM object-instances-discovered manager-object discoveries");
  auto const observerMomReflection = findMomReflection(observerFederate);
  require(
      observerMomReflection.has_value() &&
          observerMomReflection->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              observerMomReflection->object) == momClass,
      "Joined-federate MOM object-instances-discovered discovery did not identify the observer's standard MOM object");
  auto const observerMomObject = observerMomReflection->object;

  auto findCountReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == observerMomObject &&
              reflection.values.count(instancesDiscoveredAttribute) == 1U;
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
            reflection.values.count(instancesDiscoveredAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(instancesDiscoveredAttribute));
    require(value.get() >= 0, description + " returned a negative discovery count");
    return value.get();
  };
  auto requestDiscoveredCount = [&](std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        observerMomObject,
        rti::AttributeHandleSet{instancesDiscoveredAttribute},
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

  auto const baseline = requestDiscoveredCount(
      "joined-federate MOM settled object-instances-discovered baseline");
  require(
      baseline <= std::numeric_limits<std::int32_t>::max() - 3,
      "Joined-federate MOM discovery baseline cannot safely accommodate the tested invocations");

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
      "Joined-federate MOM object-instances-discovered application lookup did not round-trip");
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

  auto const discoveriesBeforeApplications =
      observer.recorder().discoveryCount();
  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      firstObject.isValid() && secondObject.isValid() &&
          firstObject != secondObject,
      "Joined-federate MOM object-instances-discovered registration returned invalid or duplicate handles");
  waitFor(
      observer,
      [&] {
        return observer.recorder().discoveryCount() >=
                discoveriesBeforeApplications + 2U &&
            observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM object-instances-discovered application discoveries");
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(firstObject) ==
              observerClass &&
          observer.rtiAmbassador().getKnownObjectClassHandle(secondObject) ==
              observerClass,
      "Joined-federate MOM object-instances-discovered callbacks did not establish both application objects");
  require(
      requestDiscoveredCount(
          "joined-federate MOM count after two application discoveries") ==
          baseline + 2,
      "Joined-federate MOM object-instances-discovered did not count both application discoveries");

  auto const discoveriesBeforeRediscovery =
      observer.recorder().discoveryCount();
  observer.rtiAmbassador().localDeleteObjectInstance(firstObject);
  requireException(
      [&] {
        static_cast<void>(
            observer.rtiAmbassador().getKnownObjectClassHandle(firstObject));
      },
      L"ObjectInstanceNotKnown",
      "known-object lookup after MOM discovered-count local deletion");
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      observerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return observer.recorder().discoveryCount() >
                discoveriesBeforeRediscovery &&
            discovery.object == firstObject;
      },
      options,
      "joined-federate MOM local-delete rediscovery of the same object");
  auto const rediscovery = observer.recorder().discovery();
  require(
      rediscovery.object == firstObject &&
          rediscovery.objectClass == observerClass &&
          rediscovery.producer == ownerFederate &&
          observer.rtiAmbassador().getKnownObjectClassHandle(firstObject) ==
              observerClass,
      "Joined-federate MOM local deletion did not permit eligible rediscovery of the same application object");
  require(
      requestDiscoveredCount(
          "joined-federate MOM count after local-delete rediscovery") ==
          baseline + 3,
      "Joined-federate MOM object-instances-discovered did not count rediscovery after local deletion");

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

  std::size_t const periodicReflectionStart =
      observer.recorder().reflections().size();
  sendTiming(1);
  waitFor(
      observer,
      [&] { return findCountReflection(periodicReflectionStart).has_value(); },
      options,
      "joined-federate MOM periodic object-instances-discovered reflection");
  auto const periodicReflection = findCountReflection(periodicReflectionStart);
  require(
      periodicReflection.has_value() &&
          decodeCount(
              *periodicReflection,
              "Joined-federate MOM periodic object-instances-discovered reflection") ==
              baseline + 3,
      "Joined-federate MOM periodic object-instances-discovered value did not preserve the discovery total");
  require(
      requestDiscoveredCount(
          "joined-federate MOM direct value after periodic discovered-count reflection") ==
          baseline + 3,
      "Joined-federate MOM direct and periodic discovered-object counts disagreed");
  sendTiming(0);
  require(
      requestDiscoveredCount(
          "joined-federate MOM discovered count after disabling periodic reports") ==
          baseline + 3,
      "Joined-federate MOM object-instances-discovered changed after periodic reporting was disabled");

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

void scenarioJoinedFederateMomObjectInstancesDiscoveredAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomObjectInstancesDiscoveredAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
