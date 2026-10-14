#include "tck_joined_federate_mom_discovered_object_count_periodic.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomDiscoveredObjectCountPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM discovered-object-count testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM discovered-object-count testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM discovered-object-count testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-discovered-object-count-periodic");
  owner.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-mom-discovered-count-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-discovered-count-observer",
      options.federateType,
      federation);
  auto const observerFederate = observer.federateHandle();
  auto const ownerFederate = owner.federateHandle();

  auto const momClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const discoveredCountAttribute = observer.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAobjectInstancesDiscovered");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          federateHandleAttribute.isValid() && discoveredCountAttribute.isValid() &&
          observer.rtiAmbassador().getAttributeName(momClass, federateHandleAttribute) ==
              L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(momClass, discoveredCountAttribute) ==
              L"HLAobjectInstancesDiscovered" &&
          reliable.isValid() &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM discovered-object-count lookup did not resolve the standard surface");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      discoveredCountAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(momClass, momAttributes, true, L"");

  auto const findMomReflection = [&](rti::FederateHandle const& federate)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(federate.encode());
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
      observer,
      [&] {
        return findMomReflection(observerFederate).has_value() &&
            findMomReflection(ownerFederate).has_value();
      },
      options,
      "joined-federate MOM discovered-object-count manager-object discoveries");
  auto const observerMomReflection = findMomReflection(observerFederate);
  require(
      observerMomReflection.has_value() && observerMomReflection->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              observerMomReflection->object) == momClass,
      "Joined-federate MOM discovered-object-count did not identify the observer's standard MOM object");
  auto const observerMomObject = observerMomReflection->object;

  auto const findCountReflection = [&](std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    if (before > reflections.size()) {
      return std::nullopt;
    }
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == observerMomObject &&
              reflection.values.count(discoveredCountAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto requestDiscoveredCount = [&](std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        observerMomObject,
        rti::AttributeHandleSet{discoveredCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no count reflection");
    require(
        reflection->values.size() == 1U && reflection->transportation == reliable &&
            reflection->tag.empty() && !reflection->producer.isValid() &&
            !reflection->regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection->values.at(discoveredCountAttribute));
    require(decoded.get() >= 0, description + " returned a negative count");
    return decoded.get();
  };

  auto const baseline = requestDiscoveredCount(
      "joined-federate MOM discovered-object-count settled direct baseline request");
  require(
      baseline <= std::numeric_limits<std::int32_t>::max() - 3,
      "Joined-federate MOM discovered-object-count baseline cannot safely accommodate the tested discoveries");

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const observerClass =
      observer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && observerClass.isValid() &&
          observerAttribute.isValid() &&
          owner.rtiAmbassador().getObjectClassName(ownerClass) == options.objectClassName &&
          observer.rtiAmbassador().getObjectClassName(observerClass) ==
              options.objectClassName &&
          owner.rtiAmbassador().getAttributeName(ownerClass, ownerAttribute) ==
              options.attributeName &&
          observer.rtiAmbassador().getAttributeName(observerClass, observerAttribute) ==
              options.attributeName,
      "Joined-federate MOM discovered-object-count adapter application lookup did not round-trip");
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute},
      true,
      L"");

  auto const discoveryCountBeforeApplications = observer.recorder().discoveryCount();
  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "Joined-federate MOM discovered-object-count registrations returned invalid identities");
  waitFor(
      observer,
      [&] {
        return observer.recorder().discoveryCount() >=
                discoveryCountBeforeApplications + 2U &&
            observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM discovered-object-count application discoveries");
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(firstObject) == observerClass &&
          observer.rtiAmbassador().getKnownObjectClassHandle(secondObject) == observerClass,
      "Joined-federate MOM discovered-object-count callbacks did not establish both application objects");
  require(
      requestDiscoveredCount(
          "joined-federate MOM discovered-object-count after two application discoveries") ==
          baseline + 2,
      "Joined-federate MOM discovered-object-count did not include two delivered object discoveries");

  auto const beforeRediscovery = observer.recorder().discoveryCount();
  observer.rtiAmbassador().localDeleteObjectInstance(firstObject);
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getKnownObjectClassHandle(firstObject));
      },
      L"ObjectInstanceNotKnown",
      "known-object lookup after discovered-object-count local deletion");
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute});
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute},
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return observer.recorder().discoveryCount() > beforeRediscovery &&
            discovery.object == firstObject;
      },
      options,
      "joined-federate MOM discovered-object-count local-delete rediscovery");
  auto const rediscovery = observer.recorder().discovery();
  require(
      rediscovery.object == firstObject && rediscovery.objectClass == observerClass &&
          rediscovery.producer == ownerFederate &&
          observer.rtiAmbassador().getKnownObjectClassHandle(firstObject) == observerClass,
      "Joined-federate MOM discovered-object-count local deletion did not produce the same object's eligible rediscovery");
  require(
      requestDiscoveredCount(
          "joined-federate MOM discovered-object-count after local-delete rediscovery") ==
          baseline + 3,
      "Joined-federate MOM discovered-object-count did not count rediscovery of the locally deleted object");

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
          observer.rtiAmbassador().getParameterName(setTiming, federateParameter) ==
              L"HLAfederate" &&
          observer.rtiAmbassador().getParameterName(setTiming, reportPeriodParameter) ==
              L"HLAreportPeriod",
      "Joined-federate MOM discovered-object-count HLAsetTiming lookup did not round-trip");
  auto const beforePeriodic = observer.recorder().reflections().size();
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {reportPeriodParameter, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return findCountReflection(beforePeriodic).has_value(); },
      options,
      "joined-federate MOM discovered-object-count periodic report");
  auto const periodicReflection = findCountReflection(beforePeriodic);
  require(
      periodicReflection.has_value() && periodicReflection->values.size() == 1U &&
          periodicReflection->transportation == reliable &&
          periodicReflection->tag.empty() && !periodicReflection->producer.isValid() &&
          !periodicReflection->regions.has_value(),
      "Joined-federate MOM discovered-object-count periodic reflection metadata was incorrect");
  rti::HLAinteger32BE periodicDiscoveredCount;
  periodicDiscoveredCount.decode(periodicReflection->values.at(discoveredCountAttribute));
  require(
      periodicDiscoveredCount.get() == baseline + 3,
      "Joined-federate MOM discovered-object-count periodic value did not preserve the discovery total");
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {federateParameter, observerFederate.encode()},
          {reportPeriodParameter, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute});
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
