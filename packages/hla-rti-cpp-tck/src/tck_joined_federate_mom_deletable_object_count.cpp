#include <RTI/encoding/BasicDataElements.h>

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_joined_federate_mom_deletable_object_count.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomDeletableObjectCount(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM deletable-object count testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM deletable-object count testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "mom-deletable-count-owner");
  Session observer(options, model, "mom-deletable-count-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-deletable-object-count");
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

  observer.join(
      options.memberFederateName + L"-mom-deletable-count-observer",
      options.federateType,
      federation);
  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const observerFederateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const observerDeletableCountAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAobjectInstancesThatCanBeDeleted");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      observerFederateClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observerFederateHandleAttribute.isValid() &&
          observerDeletableCountAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerFederateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerDeletableCountAttribute) ==
              L"HLAobjectInstancesThatCanBeDeleted" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM deletable-object count lookup did not round-trip standard handles");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerDeletableCountAttribute},
      true,
      L"");

  owner.join(
      options.ownerFederateName + L"-mom-deletable-count-owner",
      options.federateType,
      federation);
  auto const ownerFederateClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const ownerFederateHandleAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAfederateHandle");
  auto const ownerDeletableCountAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAobjectInstancesThatCanBeDeleted");
  require(
      ownerFederateClass == observerFederateClass &&
          ownerFederateHandleAttribute == observerFederateHandleAttribute &&
          ownerDeletableCountAttribute == observerDeletableCountAttribute,
      "Joined-federate MOM deletable-object count returned different handles to the joined federates");

  auto findOwnerMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(owner.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerFederateHandleAttribute);
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
      [&] { return findOwnerMomReflection().has_value(); },
      options,
      "joined-federate MOM deletable-object owner discovery");
  auto const ownerMomReflection = findOwnerMomReflection();
  require(
      ownerMomReflection.has_value() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              ownerMomReflection->object) == observerFederateClass,
      "Joined-federate MOM deletable-object discovery returned the wrong standard object");
  auto const ownerMomObject = ownerMomReflection->object;

  auto requestDeletableCount = [&](std::int32_t expected,
                                   std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        ownerMomObject,
        rti::AttributeHandleSet{observerDeletableCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return reflections.size() > before &&
              std::any_of(
                  reflections.begin() + static_cast<std::ptrdiff_t>(before),
                  reflections.end(),
                  [&](ReflectionRecord const& reflection) {
                    return reflection.object == ownerMomObject &&
                        reflection.values.count(observerDeletableCountAttribute) == 1U;
                  });
        },
        options,
        description);
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == ownerMomObject &&
              reflection.values.count(observerDeletableCountAttribute) == 1U;
        });
    require(iterator != reflections.end(), description + " produced no value");
    auto const& reflection = *iterator;
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(observerDeletableCountAttribute));
    require(
        decoded.get() == expected,
        description + " returned the wrong deletable-object count");
  };

  requestDeletableCount(
      0,
      "joined-federate MOM initial deletable-object value request");

  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.attributeName);
  require(
      ownerObjectClass.isValid() && ownerAttribute.isValid(),
      "Joined-federate MOM deletable-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      firstObject.isValid(),
      "Joined-federate MOM first deletable-object registration returned an invalid handle");
  requestDeletableCount(
      1,
      "joined-federate MOM one-object value request");
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      secondObject.isValid(),
      "Joined-federate MOM second deletable-object registration returned an invalid handle");
  requestDeletableCount(
      2,
      "joined-federate MOM two-object value request");

  auto const setTiming = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          observer.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          observer.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          observer.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM deletable-object count HLAsetTiming lookup did not round-trip");
  auto const beforePeriodic = observer.recorder().reflections().size();
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, owner.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflections = observer.recorder().reflections();
        auto const begin = reflections.begin() +
            static_cast<std::ptrdiff_t>(std::min(beforePeriodic, reflections.size()));
        return std::any_of(
            begin,
            reflections.end(),
            [&](ReflectionRecord const& reflection) {
              return reflection.present && reflection.object == ownerMomObject &&
                  reflection.values.count(observerDeletableCountAttribute) == 1U;
            });
      },
      options,
      "joined-federate MOM deletable-object periodic reflection");
  auto const periodicReflections = observer.recorder().reflections();
  auto const periodicBegin = periodicReflections.begin() +
      static_cast<std::ptrdiff_t>(std::min(beforePeriodic, periodicReflections.size()));
  auto const periodicReflection = std::find_if(
      periodicBegin,
      periodicReflections.end(),
      [&](ReflectionRecord const& reflection) {
        return reflection.present && reflection.object == ownerMomObject &&
            reflection.values.count(observerDeletableCountAttribute) == 1U;
      });
  require(
      periodicReflection != periodicReflections.end() &&
          periodicReflection->values.size() == 1U &&
          periodicReflection->transportation == reliable &&
          periodicReflection->tag.empty() && !periodicReflection->producer.isValid() &&
          !periodicReflection->regions.has_value(),
      "Joined-federate MOM deletable-object periodic reflection metadata was incorrect");
  rti::HLAinteger32BE periodicDeletableCount;
  periodicDeletableCount.decode(
      periodicReflection->values.at(observerDeletableCountAttribute));
  require(
      periodicDeletableCount.get() == 2,
      "Joined-federate MOM deletable-object periodic report returned the wrong count");
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, owner.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});

  owner.rtiAmbassador().deleteObjectInstance(
      firstObject,
      rti::VariableLengthData{});
  requestDeletableCount(
      1,
      "joined-federate MOM value request after one deletion");
  owner.rtiAmbassador().deleteObjectInstance(
      secondObject,
      rti::VariableLengthData{});
  requestDeletableCount(
      0,
      "joined-federate MOM value request after all deletions");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerDeletableCountAttribute});
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomDeletableObjectCountContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomDeletableObjectCount(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
