#include "tck_joined_federate_mom_updated_object_count_periodic.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomUpdatedObjectCountPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM updated-object-count testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM updated-object-count testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM updated-object-count testing requires an adapter-supplied logical-time implementation");

  Session subject(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-updated-object-count-periodic");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-mom-updated-count-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-updated-count-observer",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();

  auto const momClass = subject.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = subject.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const registeredInstancesAttribute = subject.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAobjectInstancesRegistered");
  auto const updatedInstancesAttribute = subject.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAobjectInstancesUpdated");
  auto const updatesSentAttribute = subject.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAupdatesSent");
  auto const reliable = subject.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() &&
          subject.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          federateHandleAttribute.isValid() && registeredInstancesAttribute.isValid() &&
          updatedInstancesAttribute.isValid() && updatesSentAttribute.isValid() &&
          reliable.isValid() &&
          subject.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM updated-object-count lookup did not resolve the standard surface");
  rti::AttributeHandleSet const momCountAttributes{
      registeredInstancesAttribute,
      updatedInstancesAttribute,
      updatesSentAttribute};
  rti::AttributeHandleSet const momSubscriptionAttributes{
      federateHandleAttribute,
      registeredInstancesAttribute,
      updatedInstancesAttribute,
      updatesSentAttribute};
  subject.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      momSubscriptionAttributes,
      true,
      L"");

  auto const findSubjectMomDiscovery = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = subject.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(subjectFederate.encode());
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
      subject,
      [&] { return findSubjectMomDiscovery().has_value(); },
      options,
      "joined-federate MOM updated-object-count self discovery");
  auto const subjectMomDiscovery = findSubjectMomDiscovery();
  require(
      subjectMomDiscovery.has_value() && subjectMomDiscovery->object.isValid() &&
          subject.rtiAmbassador().getKnownObjectClassHandle(
              subjectMomDiscovery->object) == momClass,
      "Joined-federate MOM updated-object-count discovery returned the wrong standard object");
  auto const subjectMomObject = subjectMomDiscovery->object;

  auto const findCountReflection = [&](std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = subject.recorder().reflections();
    if (before > reflections.size()) {
      return std::nullopt;
    }
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == subjectMomObject &&
              reflection.values.count(registeredInstancesAttribute) == 1U &&
              reflection.values.count(updatedInstancesAttribute) == 1U &&
              reflection.values.count(updatesSentAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeCounts = [&](ReflectionRecord const& reflection,
                          std::string const& description) {
    require(
        reflection.values.size() == 3U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE registeredInstances;
    rti::HLAinteger32BE updatedInstances;
    rti::HLAinteger32BE updatesSent;
    registeredInstances.decode(reflection.values.at(registeredInstancesAttribute));
    updatedInstances.decode(reflection.values.at(updatedInstancesAttribute));
    updatesSent.decode(reflection.values.at(updatesSentAttribute));
    require(
        registeredInstances.get() >= 0 && updatedInstances.get() >= 0 &&
            updatesSent.get() >= 0,
        description + " returned a negative count");
    struct Counts final {
      std::int32_t registered;
      std::int32_t updated;
      std::int32_t sent;
    };
    return Counts{
        registeredInstances.get(),
        updatedInstances.get(),
        updatesSent.get()};
  };
  auto requestCounts = [&](std::string const& description) {
    auto const before = subject.recorder().reflections().size();
    subject.rtiAmbassador().requestAttributeValueUpdate(
        subjectMomObject,
        momCountAttributes,
        rti::VariableLengthData{});
    waitFor(
        subject,
        [&] { return findCountReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findCountReflection(before);
    require(reflection.has_value(), description + " produced no count reflection");
    return decodeCounts(*reflection, description);
  };

  auto const initialCounts = requestCounts(
      "joined-federate MOM updated-object-count initial direct request");
  require(
      initialCounts.registered == 0 && initialCounts.updated == 0 &&
          initialCounts.sent == 0,
      "Joined-federate MOM updated-object-count initial values were not zero");

  auto const subjectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subjectAttribute = subject.rtiAmbassador().getAttributeHandle(
      subjectClass,
      options.attributeName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      subjectClass.isValid() && subjectAttribute.isValid() && observerClass.isValid() &&
          observerAttribute.isValid(),
      "Joined-federate MOM updated-object-count application lookup returned an invalid handle");
  subject.rtiAmbassador().publishObjectClassAttributes(
      subjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute},
      true,
      L"");
  auto const objectOne = subject.rtiAmbassador().registerObjectInstance(subjectClass);
  require(
      objectOne.isValid(),
      "Joined-federate MOM updated-object-count first registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(objectOne); },
      options,
      "joined-federate MOM updated-object-count first application discovery");
  auto const firstRegistrationCounts = requestCounts(
      "joined-federate MOM updated-object-count after first registration");
  require(
      firstRegistrationCounts.registered == 1 &&
          firstRegistrationCounts.updated == 0 && firstRegistrationCounts.sent == 0,
      "Joined-federate MOM updated-object-count first registration values were incorrect");

  auto sendUpdate = [&](rti::ObjectInstanceHandle const& object,
                        std::uint8_t value,
                        std::size_t expectedReflections,
                        std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    subject.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{{
            subjectAttribute,
            rti::HLAbyte{static_cast<rti::Octet>(value)}.encode()}},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          auto const begin = reflections.begin() +
              static_cast<std::ptrdiff_t>(std::min(before, reflections.size()));
          auto const expectedValue = copyBytes(
              rti::HLAbyte{static_cast<rti::Octet>(value)}.encode());
          return static_cast<std::size_t>(std::count_if(
                     begin,
                     reflections.end(),
                     [&](ReflectionRecord const& reflection) {
                       auto const reflectedValue =
                           reflection.values.find(observerAttribute);
                       return reflection.present && reflection.object == object &&
                           reflectedValue != reflection.values.end() &&
                           copyBytes(reflectedValue->second) == expectedValue;
                     })) >= expectedReflections;
        },
        options,
        description);
  };

  sendUpdate(
      objectOne,
      0x51U,
      1U,
      "joined-federate MOM updated-object-count first object update");
  auto const firstCounts = requestCounts(
      "joined-federate MOM updated-object-count after first update");
  require(
      firstCounts.registered == 1 && firstCounts.updated == 1 &&
          firstCounts.sent == 1,
      "Joined-federate MOM updated-object-count first values were incorrect");

  sendUpdate(
      objectOne,
      0x52U,
      1U,
      "joined-federate MOM updated-object-count repeated object update");
  auto const repeatedCounts = requestCounts(
      "joined-federate MOM updated-object-count after repeated update");
  require(
      repeatedCounts.registered == 1 && repeatedCounts.updated == 1 &&
          repeatedCounts.sent == 2,
      "Joined-federate MOM updated-object-count repeated-object values were incorrect");

  auto const objectTwo = subject.rtiAmbassador().registerObjectInstance(subjectClass);
  require(
      objectTwo.isValid(),
      "Joined-federate MOM updated-object-count second registration returned an invalid handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(objectTwo); },
      options,
      "joined-federate MOM updated-object-count second application discovery");
  auto const secondRegistrationCounts = requestCounts(
      "joined-federate MOM updated-object-count after second registration");
  require(
      secondRegistrationCounts.registered == 2 &&
          secondRegistrationCounts.updated == 1 && secondRegistrationCounts.sent == 2,
      "Joined-federate MOM updated-object-count second registration values were incorrect");

  sendUpdate(
      objectTwo,
      0x53U,
      1U,
      "joined-federate MOM updated-object-count second object update");
  auto const secondObjectCounts = requestCounts(
      "joined-federate MOM updated-object-count after second object");
  require(
      secondObjectCounts.registered == 2 && secondObjectCounts.updated == 2 &&
          secondObjectCounts.sent == 3,
      "Joined-federate MOM updated-object-count distinct-object values were incorrect");

  auto const setTiming = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = subject.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = subject.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          subject.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          subject.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          subject.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM updated-object-count HLAsetTiming lookup did not round-trip");
  auto sendTiming = [&](std::int32_t period) {
    subject.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, subjectFederate.encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };
  auto const beforePeriodic = subject.recorder().reflections().size();
  sendTiming(1);
  waitFor(
      subject,
      [&] { return findCountReflection(beforePeriodic).has_value(); },
      options,
      "joined-federate MOM updated-object-count periodic reflection");
  auto const periodicReflection = findCountReflection(beforePeriodic);
  require(
      periodicReflection.has_value(),
      "Joined-federate MOM updated-object-count periodic report was missing");
  auto const periodicCounts = decodeCounts(
      *periodicReflection,
      "joined-federate MOM updated-object-count periodic report");
  require(
      periodicCounts.registered == 2 && periodicCounts.updated == 2 &&
          periodicCounts.sent == 3,
      "Joined-federate MOM updated-object-count periodic values were incorrect");
  sendTiming(0);

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      rti::AttributeHandleSet{observerAttribute});
  subject.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      momSubscriptionAttributes);
  subject.rtiAmbassador().unpublishObjectClassAttributes(
      subjectClass,
      rti::AttributeHandleSet{subjectAttribute});
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
