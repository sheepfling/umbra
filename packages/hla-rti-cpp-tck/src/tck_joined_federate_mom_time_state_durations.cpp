#include "tck_joined_federate_mom_time_state_durations.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomTimeStateDurations(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM time-duration testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM time-duration testing requires an adapter-supplied logical-time implementation");

  Session subject(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation = federationName(options, "joined-federate-mom-time-state-durations");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(
      options.memberFederateName + L"-mom-duration-observer",
      options.federateType,
      federation);

  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(
      observerFederateClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate",
      "Joined-federate MOM time-duration lookup did not round-trip the standard class");
  auto const federateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const timeGrantedAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAtimeGrantedTime");
  auto const timeAdvancingAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAtimeAdvancingTime");
  require(
      federateHandleAttribute.isValid() && timeGrantedAttribute.isValid() &&
          timeAdvancingAttribute.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              timeGrantedAttribute) == L"HLAtimeGrantedTime" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              timeAdvancingAttribute) == L"HLAtimeAdvancingTime",
      "Joined-federate MOM time-duration lookup returned inconsistent standard attributes");

  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliable.isValid() &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM time-duration lookup could not resolve HLAreliable");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      timeGrantedAttribute,
      timeAdvancingAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      momAttributes,
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-mom-duration-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
  auto const expectedFederateHandle = copyBytes(subjectFederate.encode());

  auto findSubjectMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
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
      [&] { return findSubjectMomReflection().has_value(); },
      options,
      "joined-federate MOM time-duration subject discovery");
  auto const initial = findSubjectMomReflection();
  require(
      initial.has_value() && initial->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(initial->object) ==
              observerFederateClass,
      "Joined-federate MOM time-duration discovery returned the wrong standard object");
  auto const subjectMomObject = initial->object;
  require(
      initial->values.count(timeGrantedAttribute) == 0U &&
          initial->values.count(timeAdvancingAttribute) == 0U,
      "Joined-federate MOM time-duration initial reflection was not conditional");

  auto const findDurationReflection = [&](std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = reflections.begin() + static_cast<std::ptrdiff_t>(before);
    auto const iterator = std::find_if(
        begin,
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == subjectMomObject &&
              reflection.values.count(timeGrantedAttribute) == 1U &&
              reflection.values.count(timeAdvancingAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeDuration = [&](ReflectionRecord const& reflection,
                            rti::AttributeHandle const& attribute,
                            std::string const& description) {
    require(
        reflection.values.at(attribute).size() == 4U,
        description + " did not use the official four-octet HLAinteger32BE encoding");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(attribute));
    require(decoded.get() >= 0, description + " returned a negative duration");
    return decoded.get();
  };
  auto requireDurationReflectionMetadata = [&](ReflectionRecord const& reflection,
                                               std::string const& description) {
    require(
        reflection.values.size() == 2U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    static_cast<void>(decodeDuration(
        reflection,
        timeGrantedAttribute,
        description + " HLAtimeGrantedTime"));
    static_cast<void>(decodeDuration(
        reflection,
        timeAdvancingAttribute,
        description + " HLAtimeAdvancingTime"));
  };
  auto waitForDurationReflection = [&](std::size_t before,
                                       std::string const& description) {
    waitFor(
        observer,
        [&] { return findDurationReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findDurationReflection(before);
    require(reflection.has_value(), description + " produced no duration reflection");
    requireDurationReflectionMetadata(*reflection, description);
    return *reflection;
  };

  // Request Attribute Value Update is a direct current-value path even though
  // these MIM attributes are declared Periodic.  The values use the official
  // HLAmsec/HLAinteger32BE representation and are non-negative durations.
  auto const beforeDirect = observer.recorder().reflections().size();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      subjectMomObject,
      rti::AttributeHandleSet{timeGrantedAttribute, timeAdvancingAttribute},
      rti::VariableLengthData{});
  auto const direct = waitForDurationReflection(
      beforeDirect,
      "joined-federate MOM time-duration direct value request");

  auto const setTiming = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const federateParameter = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const periodParameter = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && federateParameter.isValid() && periodParameter.isValid(),
      "Joined-federate MOM time-duration HLAsetTiming lookup returned an invalid handle");
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {federateParameter, subjectFederate.encode()},
          {periodParameter, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  auto const periodic = waitForDurationReflection(
      observer.recorder().reflections().size(),
      "joined-federate MOM time-duration periodic reflection");
  static_cast<void>(direct);
  static_cast<void>(periodic);

  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {federateParameter, subjectFederate.encode()},
          {periodParameter, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      momAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
