#include "tck_joined_federate_mom_deleted_object_count_periodic.hpp"
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

void scenarioJoinedFederateMomDeletedObjectCountPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM deleted-object-count testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM deleted-object-count testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM deleted-object-count testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-deleted-object-count-periodic");
  owner.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-mom-deleted-count-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-deleted-count-observer",
      options.federateType,
      federation);
  auto const ownerFederate = owner.federateHandle();

  auto const momClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = owner.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const deletedCountAttribute = owner.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAobjectInstancesDeleted");
  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() &&
          owner.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          federateHandleAttribute.isValid() && deletedCountAttribute.isValid() &&
          owner.rtiAmbassador().getAttributeName(momClass, federateHandleAttribute) ==
              L"HLAfederateHandle" &&
          owner.rtiAmbassador().getAttributeName(momClass, deletedCountAttribute) ==
              L"HLAobjectInstancesDeleted" &&
          reliable.isValid() &&
          owner.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM deleted-object-count lookup did not resolve the standard surface");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      deletedCountAttribute};
  owner.rtiAmbassador().subscribeObjectClassAttributes(momClass, momAttributes, true, L"");

  auto const findOwnerMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = owner.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(ownerFederate.encode());
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
      owner,
      [&] { return findOwnerMomReflection().has_value(); },
      options,
      "joined-federate MOM deleted-object-count self discovery");
  auto const ownerMomReflection = findOwnerMomReflection();
  require(
      ownerMomReflection.has_value() && ownerMomReflection->object.isValid() &&
          owner.rtiAmbassador().getKnownObjectClassHandle(ownerMomReflection->object) ==
              momClass,
      "Joined-federate MOM deleted-object-count discovery returned the wrong standard object");
  auto const ownerMomObject = ownerMomReflection->object;

  auto const findCountReflection = [&](std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = owner.recorder().reflections();
    if (before > reflections.size()) {
      return std::nullopt;
    }
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == ownerMomObject &&
              reflection.values.count(deletedCountAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto requestDeletedCount = [&](std::string const& description) {
    auto const before = owner.recorder().reflections().size();
    owner.rtiAmbassador().requestAttributeValueUpdate(
        ownerMomObject,
        rti::AttributeHandleSet{deletedCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        owner,
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
    decoded.decode(reflection->values.at(deletedCountAttribute));
    require(decoded.get() >= 0, description + " returned a negative count");
    return decoded.get();
  };
  auto const baseline = requestDeletedCount(
      "joined-federate MOM deleted-object-count direct baseline request");
  require(
      baseline <= std::numeric_limits<std::int32_t>::max() - 2,
      "Joined-federate MOM deleted-object-count baseline cannot safely accommodate the tested deletions");

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
      "Joined-federate MOM deleted-object-count adapter application lookup did not round-trip");
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute},
      true,
      L"");

  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      firstObject.isValid() && secondObject.isValid(),
      "Joined-federate MOM deleted-object-count registrations returned invalid identities");
  waitFor(
      observer,
      [&] {
        return observer.recorder().hasDiscovery(firstObject) &&
            observer.recorder().hasDiscovery(secondObject);
      },
      options,
      "joined-federate MOM deleted-object-count application discoveries");
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(firstObject) == observerClass &&
          observer.rtiAmbassador().getKnownObjectClassHandle(secondObject) == observerClass,
      "Joined-federate MOM deleted-object-count discoveries did not establish both application objects");
  require(
      requestDeletedCount(
          "joined-federate MOM deleted-object-count after registration") == baseline,
      "Joined-federate MOM deleted-object-count changed before a Delete Object Instance call");

  auto deleteAndCheck = [&](rti::ObjectInstanceHandle const& object,
                            std::vector<std::uint8_t> const& tagBytes,
                            std::int32_t expected,
                            std::string const& description) {
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    observer.recorder().clearRemovals();
    owner.rtiAmbassador().deleteObjectInstance(object, tag);
    waitFor(
        observer,
        [&] {
          return observer.recorder().hasRemoval(object, tagBytes, ownerFederate);
        },
        options,
        description + " removal callback");
    require(
        requestDeletedCount(description + " direct count request") == expected,
        description + " produced the wrong standard Delete Object Instance count");
  };
  auto const firstName = owner.rtiAmbassador().getObjectInstanceName(firstObject);
  auto const secondName = owner.rtiAmbassador().getObjectInstanceName(secondObject);
  deleteAndCheck(
      firstObject,
      std::vector<std::uint8_t>{0x44U, 0x45U, 0x4CU, 0x31U},
      baseline + 1,
      "joined-federate MOM deleted-object-count first deletion");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getObjectInstanceHandle(firstName)); },
      L"ObjectInstanceNotKnown",
      "object-name lookup after first deleted-object-count removal");
  deleteAndCheck(
      secondObject,
      std::vector<std::uint8_t>{0x44U, 0x45U, 0x4CU, 0x32U},
      baseline + 2,
      "joined-federate MOM deleted-object-count second deletion");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().getObjectInstanceHandle(secondName)); },
      L"ObjectInstanceNotKnown",
      "object-name lookup after second deleted-object-count removal");

  auto const setTiming = owner.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const federateParameter = owner.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriodParameter = owner.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && federateParameter.isValid() &&
          reportPeriodParameter.isValid() &&
          owner.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          owner.rtiAmbassador().getParameterName(setTiming, federateParameter) ==
              L"HLAfederate" &&
          owner.rtiAmbassador().getParameterName(setTiming, reportPeriodParameter) ==
              L"HLAreportPeriod",
      "Joined-federate MOM deleted-object-count HLAsetTiming lookup did not round-trip");
  auto const beforePeriodic = owner.recorder().reflections().size();
  owner.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {federateParameter, ownerFederate.encode()},
          {reportPeriodParameter, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      owner,
      [&] { return findCountReflection(beforePeriodic).has_value(); },
      options,
      "joined-federate MOM deleted-object-count periodic report");
  auto const periodicReflection = findCountReflection(beforePeriodic);
  require(
      periodicReflection.has_value() && periodicReflection->values.size() == 1U &&
          periodicReflection->transportation == reliable &&
          periodicReflection->tag.empty() && !periodicReflection->producer.isValid() &&
          !periodicReflection->regions.has_value(),
      "Joined-federate MOM deleted-object-count periodic reflection metadata was incorrect");
  rti::HLAinteger32BE periodicDeletedCount;
  periodicDeletedCount.decode(periodicReflection->values.at(deletedCountAttribute));
  require(
      periodicDeletedCount.get() == baseline + 2,
      "Joined-federate MOM deleted-object-count periodic value did not preserve the deletion total");
  owner.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {federateParameter, ownerFederate.encode()},
          {reportPeriodParameter, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute});
  owner.rtiAmbassador().unsubscribeObjectClassAttributes(momClass, momAttributes);
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
