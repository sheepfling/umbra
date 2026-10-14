#include "tck_joined_federate_mom_galt_lits_periodic.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomGaltLitsPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM GALT/LITS testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM GALT/LITS testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM GALT/LITS testing requires an adapter-supplied logical-time implementation");

  Session subject(options, model, "mom-galt-lits-subject");
  Session regulator(options, model, "mom-galt-lits-regulator");
  Session observer(options, model, "mom-galt-lits-observer");
  auto const federation = federationName(options, "joined-federate-mom-galt-lits-periodic");
  subject.connect();
  regulator.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(
      options.memberFederateName + L"-mom-galt-lits-observer",
      options.federateType,
      federation);

  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(
      observerFederateClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate",
      "Joined-federate MOM GALT/LITS lookup did not round-trip the standard class");
  auto const federateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const galtAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAGALT");
  auto const litsAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLALITS");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federateHandleAttribute.isValid() && galtAttribute.isValid() &&
          litsAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              galtAttribute) == L"HLAGALT" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              litsAttribute) == L"HLALITS" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM GALT/LITS lookup returned inconsistent standard handles");
  rti::AttributeHandleSet const requestedAttributes{
      galtAttribute,
      litsAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      federateHandleAttribute,
      galtAttribute,
      litsAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      subscribedAttributes,
      true,
      L"");

  regulator.join(
      options.ownerFederateName + L"-mom-galt-lits-regulator",
      options.federateType,
      federation);
  auto regulatorTime = makeTimeContext(regulator);
  auto observerTime = makeTimeContext(observer);
  require(
      regulatorTime.factory->getName() == observerTime.factory->getName(),
      "Joined-federate MOM GALT/LITS members selected different logical-time factories");
  auto regulatorLookaheadTime = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      2U);
  auto regulatorLookahead = regulatorTime.factory->makeZero();
  require(
      regulatorLookahead != nullptr,
      "Joined-federate MOM GALT/LITS could not allocate a lookahead interval");
  regulatorLookahead->setToDifference(
      *regulatorLookaheadTime,
      *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorLookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "joined-federate MOM GALT/LITS time-regulation callback");

  subject.join(
      options.ownerFederateName + L"-mom-galt-lits-subject",
      options.federateType,
      federation);
  auto const expectedSubjectFederateHandle = copyBytes(subject.federateHandle().encode());
  auto findSubjectMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedSubjectFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findSubjectMomReflection().has_value(); },
      options,
      "joined-federate MOM GALT/LITS subject discovery");
  auto const subjectMomReflection = findSubjectMomReflection();
  require(
      subjectMomReflection.has_value() && subjectMomReflection->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              subjectMomReflection->object) == observerFederateClass,
      "Joined-federate MOM GALT/LITS discovery returned the wrong standard object");
  auto const subjectMomObject = subjectMomReflection->object;

  auto findBoundsReflection = [&](std::size_t before,
                                  bool undefined) -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = reflections.begin() +
        static_cast<std::vector<ReflectionRecord>::difference_type>(before);
    auto const iterator = std::find_if(
        begin,
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          if (!reflection.present || reflection.object != subjectMomObject ||
              reflection.values.count(galtAttribute) != 1U ||
              reflection.values.count(litsAttribute) != 1U) {
            return false;
          }
          auto const galt = copyBytes(reflection.values.at(galtAttribute));
          auto const lits = copyBytes(reflection.values.at(litsAttribute));
          return undefined ? galt.empty() && lits.empty()
                           : !galt.empty() && !lits.empty();
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto verifyBoundsReflection = [&](ReflectionRecord const& reflection,
                                    bool undefined,
                                    std::string const& description) {
    require(
        reflection.values.size() == 2U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    if (undefined) {
      require(
          reflection.values.at(galtAttribute).size() == 0U &&
              reflection.values.at(litsAttribute).size() == 0U,
          description + " did not use empty encodings for undefined bounds");
      return;
    }
    auto const galt = decodeTime(
        *observerTime.factory,
        copyBytes(reflection.values.at(galtAttribute)));
    auto const lits = decodeTime(
        *observerTime.factory,
        copyBytes(reflection.values.at(litsAttribute)));
    auto const expected = timeAfter(
        *observerTime.factory,
        *observerTime.initial,
        *observerTime.epsilon,
        2U);
    requireTimeEquals(*galt, *expected, description + " HLAGALT");
    requireTimeEquals(*lits, *expected, description + " HLALITS");
  };
  auto requestBounds = [&](bool undefined, std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        subjectMomObject,
        requestedAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return findBoundsReflection(before, undefined).has_value(); },
        options,
        description);
    auto const reflection = findBoundsReflection(before, undefined);
    require(reflection.has_value(), description + " produced no MOM reflection");
    verifyBoundsReflection(*reflection, undefined, description);
  };

  auto galt = observerTime.factory->makeInitial();
  auto lits = observerTime.factory->makeInitial();
  require(galt != nullptr && lits != nullptr, "Joined-federate MOM GALT/LITS could not allocate query values");
  require(observer.rtiAmbassador().queryGALT(*galt),
          "Joined-federate MOM queryGALT remained undefined while a regulator was active");
  require(observer.rtiAmbassador().queryLITS(*lits),
          "Joined-federate MOM queryLITS remained undefined while a regulator was active");
  auto const expected = timeAfter(
      *observerTime.factory,
      *observerTime.initial,
      *observerTime.epsilon,
      2U);
  requireTimeEquals(*galt, *expected, "Joined-federate MOM queryGALT");
  requireTimeEquals(*lits, *expected, "Joined-federate MOM queryLITS");
  requestBounds(false, "joined-federate MOM GALT/LITS direct value request");

  auto const setTiming = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid(),
      "Joined-federate MOM GALT/LITS HLAsetTiming lookup returned an invalid handle");
  auto const beforePeriodic = observer.recorder().reflections().size();
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, subject.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return findBoundsReflection(beforePeriodic, false).has_value(); },
      options,
      "joined-federate MOM GALT/LITS periodic reflection");
  auto const periodicReflection = findBoundsReflection(beforePeriodic, false);
  require(periodicReflection.has_value(),
          "Joined-federate MOM GALT/LITS periodic report was missing");
  verifyBoundsReflection(*periodicReflection, false,
                         "joined-federate MOM GALT/LITS periodic report");

  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, subject.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});
  regulator.rtiAmbassador().disableTimeRegulation();
  require(!observer.rtiAmbassador().queryGALT(*galt),
          "Joined-federate MOM queryGALT remained defined after regulation was disabled");
  require(!observer.rtiAmbassador().queryLITS(*lits),
          "Joined-federate MOM queryLITS remained defined after regulation was disabled");
  requestBounds(true, "joined-federate MOM GALT/LITS undefined value request");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      subscribedAttributes);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  regulator.disconnect();
  observer.disconnect();
}


}  // namespace hla_rti_cpp_tck::detail
