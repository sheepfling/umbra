#include "tck_joined_federate_mom_tso_length_periodic.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomTsoLengthPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM TSO-length testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM TSO-length testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM TSO-length testing requires an adapter-supplied logical-time implementation");

  Session target(options, model, "mom-tso-length-target");
  Session publisher(options, model, "mom-tso-length-publisher");
  Session observer(options, model, "mom-tso-length-observer");
  auto const federation = federationName(options, "joined-federate-mom-tso-length-periodic");
  target.connect();
  publisher.connect();
  observer.connect();
  target.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(
      options.memberFederateName + L"-mom-tso-length-observer",
      options.federateType,
      federation);

  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const tsoLengthAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLATSOlength");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      observerFederateClass.isValid() && federateHandleAttribute.isValid() &&
          tsoLengthAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              tsoLengthAttribute) == L"HLATSOlength",
      "Joined-federate MOM TSO-length lookup returned inconsistent standard handles");
  rti::AttributeHandleSet const momAttributes{
      federateHandleAttribute,
      tsoLengthAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      momAttributes,
      true,
      L"");

  target.join(
      options.ownerFederateName + L"-mom-tso-length-target",
      options.federateType,
      federation);
  publisher.join(
      options.ownerFederateName + L"-mom-tso-length-publisher",
      options.federateType,
      federation);
  auto const expectedTargetFederateHandle = copyBytes(target.federateHandle().encode());
  auto findTargetMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedTargetFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findTargetMomReflection().has_value(); },
      options,
      "joined-federate MOM TSO-length target discovery");
  auto const targetMomReflection = findTargetMomReflection();
  require(
      targetMomReflection.has_value() && targetMomReflection->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              targetMomReflection->object) == observerFederateClass,
      "Joined-federate MOM TSO-length discovery returned the wrong standard object");
  auto const targetMomObject = targetMomReflection->object;

  auto findTsoLengthReflection = [&](std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const begin = reflections.begin() + static_cast<std::vector<ReflectionRecord>::difference_type>(before);
    auto const iterator = std::find_if(
        begin,
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == targetMomObject &&
              reflection.values.count(tsoLengthAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto verifyTsoLengthReflection = [&](ReflectionRecord const& reflection,
                                       std::int32_t expected,
                                       std::string const& description) {
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(tsoLengthAttribute));
    require(decoded.get() == expected, description + " returned the wrong HLATSOlength value");
  };
  auto requestTsoLength = [&](std::int32_t expected, std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        targetMomObject,
        rti::AttributeHandleSet{tsoLengthAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return findTsoLengthReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findTsoLengthReflection(before);
    require(reflection.has_value(), description + " produced no MOM reflection");
    verifyTsoLengthReflection(*reflection, expected, description);
  };

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const targetInteraction = target.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const targetParameter = target.rtiAmbassador().getParameterHandle(
      targetInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && targetInteraction.isValid() &&
          publisherParameter.isValid() && targetParameter.isValid(),
      "Joined-federate MOM TSO-length application lookup returned an invalid adapter-supplied handle");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  target.rtiAmbassador().subscribeInteractionClass(targetInteraction);

  auto targetTime = makeTimeContext(target);
  auto publisherTime = makeTimeContext(publisher);
  require(
      targetTime.factory->getName() == publisherTime.factory->getName(),
      "Joined-federate MOM TSO-length members selected different logical-time factories");
  target.rtiAmbassador().enableTimeConstrained();
  waitFor(
      target,
      [&] { return target.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "joined-federate MOM TSO-length time-constrained callback");
  auto publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "Joined-federate MOM TSO-length could not allocate a lookahead interval");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "joined-federate MOM TSO-length time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> const parameterBytes{0x54U, 0x53U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      rti::VariableLengthData{},
      *messageTime);
  require(
      retraction.isValid(),
      "Joined-federate MOM TSO-length send returned an invalid retraction handle");
  require(
      target.recorder().timedInteractions().empty(),
      "Joined-federate MOM TSO-length delivered the queued interaction before its grant");
  requestTsoLength(1, "joined-federate MOM TSO-length direct value request");

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
      "Joined-federate MOM TSO-length HLAsetTiming lookup returned an invalid handle");
  auto const beforePeriodic = observer.recorder().reflections().size();
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, target.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return findTsoLengthReflection(beforePeriodic).has_value(); },
      options,
      "joined-federate MOM TSO-length periodic reflection");
  auto const periodicReflection = findTsoLengthReflection(beforePeriodic);
  require(periodicReflection.has_value(),
          "Joined-federate MOM TSO-length periodic report was missing");
  verifyTsoLengthReflection(*periodicReflection, 1,
                            "joined-federate MOM TSO-length periodic report");

  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, target.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});
  auto const targetMessageTime = timeAfter(
      *targetTime.factory,
      *targetTime.initial,
      *targetTime.epsilon,
      2U);
  target.rtiAmbassador().timeAdvanceRequest(*targetMessageTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&target, &publisher},
      [&] {
        return target.recorder().timeAdvanceGrants().size() >= 1U &&
            publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            target.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "joined-federate MOM TSO-length delivery and grants");
  auto const timedInteractions = target.recorder().timedInteractions();
  require(
      timedInteractions.size() == 1U,
      "Joined-federate MOM TSO-length returned the wrong timestamped interaction count");
  auto const& timedInteraction = timedInteractions.front();
  require(
      timedInteraction.interaction == targetInteraction &&
          timedInteraction.parameters.size() == 1U &&
          timedInteraction.parameters.count(targetParameter) == 1U &&
          copyBytes(timedInteraction.parameters.at(targetParameter)) == parameterBytes &&
          timedInteraction.time == encodeTime(*targetMessageTime) &&
          timedInteraction.sentOrder == rti::TIMESTAMP &&
          timedInteraction.receivedOrder == rti::TIMESTAMP &&
          timedInteraction.retractionPresent,
      "Joined-federate MOM TSO-length returned the wrong timestamped interaction metadata");
  requestTsoLength(0, "joined-federate MOM TSO-length post-grant value request");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      momAttributes);
  target.rtiAmbassador().unsubscribeInteractionClass(targetInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  target.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  target.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  target.rtiAmbassador().destroyFederationExecution(federation);
  target.disconnect();
  publisher.disconnect();
  observer.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
