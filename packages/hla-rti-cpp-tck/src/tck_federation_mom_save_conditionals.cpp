#include "tck_federation_mom_save_conditionals.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederationMomSaveConditionals(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Federation MOM save-conditionals testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Federation MOM save-conditionals testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "mom-save-owner");
  Session observer(options, model, "mom-save-observer");
  Session constrained(options, model, "mom-save-constrained");
  auto const federation = federationName(options, "federation-mom-save-conditionals");
  owner.connect();
  observer.connect();
  constrained.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-mom-save-owner",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-save-observer",
      options.federateType,
      federation);
  constrained.join(
      options.memberFederateName + L"-mom-save-constrained",
      options.federateType,
      federation);

  auto const ownerFederationClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const observerFederationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  require(
      ownerFederationClass.isValid() && observerFederationClass.isValid() &&
          ownerFederationClass == observerFederationClass &&
          observer.rtiAmbassador().getObjectClassName(observerFederationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation",
      "Federation MOM save-conditionals class lookup did not round-trip the standard class");

  auto const ownerNextSaveName = owner.rtiAmbassador().getAttributeHandle(
      ownerFederationClass, L"HLAnextSaveName");
  auto const ownerNextSaveTime = owner.rtiAmbassador().getAttributeHandle(
      ownerFederationClass, L"HLAnextSaveTime");
  auto const ownerLastSaveName = owner.rtiAmbassador().getAttributeHandle(
      ownerFederationClass, L"HLAlastSaveName");
  auto const ownerLastSaveTime = owner.rtiAmbassador().getAttributeHandle(
      ownerFederationClass, L"HLAlastSaveTime");
  auto const observerNextSaveName = observer.rtiAmbassador().getAttributeHandle(
      observerFederationClass, L"HLAnextSaveName");
  auto const observerNextSaveTime = observer.rtiAmbassador().getAttributeHandle(
      observerFederationClass, L"HLAnextSaveTime");
  auto const observerLastSaveName = observer.rtiAmbassador().getAttributeHandle(
      observerFederationClass, L"HLAlastSaveName");
  auto const observerLastSaveTime = observer.rtiAmbassador().getAttributeHandle(
      observerFederationClass, L"HLAlastSaveTime");
  require(
      ownerNextSaveName.isValid() && ownerNextSaveTime.isValid() &&
          ownerLastSaveName.isValid() && ownerLastSaveTime.isValid() &&
          observerNextSaveName.isValid() && observerNextSaveTime.isValid() &&
          observerLastSaveName.isValid() && observerLastSaveTime.isValid() &&
          ownerNextSaveName == observerNextSaveName &&
          ownerNextSaveTime == observerNextSaveTime &&
          ownerLastSaveName == observerLastSaveName &&
          ownerLastSaveTime == observerLastSaveTime,
      "Federation MOM save-conditionals lookup returned inconsistent standard attribute handles");
  require(
      observer.rtiAmbassador().getAttributeName(
          observerFederationClass, observerNextSaveName) == L"HLAnextSaveName" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederationClass, observerNextSaveTime) == L"HLAnextSaveTime" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederationClass, observerLastSaveName) == L"HLAlastSaveName" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederationClass, observerLastSaveTime) == L"HLAlastSaveTime",
      "Federation MOM save-conditionals attribute names did not round-trip");

  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliable.isValid() &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Federation MOM save-conditionals could not resolve HLAreliable");
  rti::AttributeHandleSet const conditionalAttributes{
      observerNextSaveName,
      observerNextSaveTime,
      observerLastSaveName,
      observerLastSaveTime};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederationClass,
      conditionalAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] { return observer.recorder().discoveryCount() >= 1U; },
      options,
      "Federation MOM save-conditionals object discovery");
  auto const discovered = observer.recorder().discovery();
  require(
      discovered.objectClass == observerFederationClass &&
          discovered.name == L"HLAfederation" && discovered.object.isValid(),
      "Federation MOM save-conditionals discovered the wrong standard object");
  auto const federationObject = discovered.object;
  auto const observerTime = makeTimeContext(observer);

  auto decodeLogicalTimeValue = [&](rti::VariableLengthData const& encoded,
                                    std::string const& description) {
    if (encoded.size() == 0U) {
      return std::vector<std::uint8_t>{};
    }
    auto raw = copyBytes(encoded);
    if (!raw.empty()) {
      auto decoded = decodeTime(*observerTime.factory, raw);
      require(decoded != nullptr, description + " returned an undecodable logical time");
    }
    return raw;
  };

  struct SaveConditionals final {
    std::wstring nextName;
    std::vector<std::uint8_t> nextTime;
    std::wstring lastName;
    std::vector<std::uint8_t> lastTime;
  };

  auto decodeConditionals = [&](ReflectionRecord const& reflection,
                                std::string const& description) {
    require(
        reflection.present && reflection.object == federationObject &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    SaveConditionals result;
    auto decodePair = [&](rti::AttributeHandle const& nameHandle,
                          rti::AttributeHandle const& timeHandle,
                          std::wstring& name,
                          std::vector<std::uint8_t>& time,
                          std::string const& pairDescription) {
      bool const hasName = reflection.values.count(nameHandle) == 1U;
      bool const hasTime = reflection.values.count(timeHandle) == 1U;
      require(
          hasName == hasTime,
          pairDescription + " did not reflect its name/time pair together");
      if (hasName) {
        rti::HLAunicodeString decodedName;
        decodedName.decode(reflection.values.at(nameHandle));
        name = decodedName.get();
        time = decodeLogicalTimeValue(
            reflection.values.at(timeHandle), pairDescription + " time");
      }
    };
    decodePair(
        observerNextSaveName,
        observerNextSaveTime,
        result.nextName,
        result.nextTime,
        description + " next-save");
    decodePair(
        observerLastSaveName,
        observerLastSaveTime,
        result.lastName,
        result.lastTime,
        description + " last-save");
    return result;
  };

  auto collectConditionals = [&](std::size_t before,
                                 std::vector<rti::AttributeHandle> const& required,
                                 std::string const& description) {
    auto mergeReflections = [&](std::vector<ReflectionRecord> const& reflections) {
      ReflectionRecord merged;
      for (std::size_t index = before; index < reflections.size(); ++index) {
        auto const& candidate = reflections[index];
        if (!candidate.present || candidate.object != federationObject) {
          continue;
        }
        require(
            candidate.transportation == reliable && candidate.tag.empty() &&
                !candidate.producer.isValid() && !candidate.regions.has_value(),
            description + " returned non-standard MOM reflection metadata");
        if (!merged.present) {
          merged = candidate;
        }
        for (auto const& value : candidate.values) {
          merged.values[value.first] = value.second;
        }
      }
      return merged;
    };
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          if (reflections.size() <= before) {
            return false;
          }
          auto const merged = mergeReflections(reflections);
          return merged.present && std::all_of(
              required.begin(),
              required.end(),
              [&](auto const& handle) { return merged.values.count(handle) == 1U; });
        },
        options,
        description);
    auto const merged = mergeReflections(observer.recorder().reflections());
    require(merged.present, description + " produced no reflection");
    return merged;
  };

  auto requestConditionals = [&] {
    observer.recorder().clearReflection();
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        federationObject,
        conditionalAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] { return observer.recorder().reflectionCount() >= 1U; },
        options,
        "Federation MOM save-conditionals direct value request");
    auto const reflection = collectConditionals(
        before,
        {observerNextSaveName, observerNextSaveTime, observerLastSaveName, observerLastSaveTime},
        "Federation MOM save-conditionals direct value request");
    require(
        reflection.values.size() == 4U,
        "Federation MOM save-conditionals direct value request omitted a conditional");
    return decodeConditionals(reflection, "Federation MOM save-conditionals direct value request");
  };

  auto const initial = requestConditionals();
  require(
      initial.nextName.empty() && initial.nextTime.empty() && initial.lastName.empty() &&
          initial.lastTime.empty(),
      "Federation MOM save-conditionals initial values were not empty");

  auto regulatorTime = makeTimeContext(owner);
  auto constrainedTime = makeTimeContext(constrained);
  enableTimestampedRoles(
      owner,
      constrained,
      regulatorTime,
      constrainedTime,
      options,
      "Federation MOM save-conditionals");

  std::wstring const saveLabel = L"tck-federation-mom-save-conditionals";
  auto const saveTime = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      7U);
  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  auto const pending = requestConditionals();
  require(
      pending.nextName == saveLabel && pending.nextTime == encodeTime(*saveTime) &&
          pending.lastName.empty() && pending.lastTime.empty(),
      "Federation MOM save-conditionals pending values did not expose the requested save");

  auto const admittedReflectionBefore = observer.recorder().reflections().size();
  constrained.rtiAmbassador().timeAdvanceRequest(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);
  waitForSessions(
      {&owner, &constrained, &observer},
      [&] {
        return owner.federateSaveInitiations().size() >= 1U &&
            constrained.federateSaveInitiations().size() >= 1U &&
            observer.federateSaveInitiations().size() >= 1U;
      },
      options,
      "Federation MOM save-conditionals admission callbacks");
  auto const admittedReflection = collectConditionals(
      admittedReflectionBefore,
      {observerNextSaveName, observerNextSaveTime},
      "Federation MOM save-conditionals admission reflection");
  auto const admitted = decodeConditionals(
      admittedReflection,
      "Federation MOM save-conditionals admission reflection");
  require(
      admitted.nextName.empty() && admitted.nextTime.empty() && admitted.lastName.empty() &&
          admitted.lastTime.empty(),
      "Federation MOM save-conditionals admission did not clear the pending values");
  require(
      owner.federateSaveInitiations().front() == saveLabel &&
          constrained.federateSaveInitiations().front() == saveLabel &&
          observer.federateSaveInitiations().front() == saveLabel,
      "Federation MOM save-conditionals admission returned the wrong save label");
  require(
      owner.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
          constrained.federateSaveTimes().front().encoded == encodeTime(*saveTime),
      "Federation MOM save-conditionals admission returned the wrong save timestamp");

  owner.rtiAmbassador().federateSaveBegun();
  constrained.rtiAmbassador().federateSaveBegun();
  observer.rtiAmbassador().federateSaveBegun();
  auto const completedReflectionBefore = observer.recorder().reflections().size();
  owner.rtiAmbassador().federateSaveComplete();
  constrained.rtiAmbassador().federateSaveComplete();
  observer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &constrained, &observer},
      [&] {
        return owner.federationSavedCount() >= 1U &&
            constrained.federationSavedCount() >= 1U &&
            observer.federationSavedCount() >= 1U;
      },
      options,
      "Federation MOM save-conditionals completion callbacks");
  auto const completedReflection = collectConditionals(
      completedReflectionBefore,
      {observerLastSaveName, observerLastSaveTime},
      "Federation MOM save-conditionals completion reflection");
  auto const completed = decodeConditionals(
      completedReflection,
      "Federation MOM save-conditionals completion reflection");
  require(
      completed.nextName.empty() && completed.nextTime.empty() &&
          completed.lastName == saveLabel && completed.lastTime == encodeTime(*saveTime),
      "Federation MOM save-conditionals completion did not update the last values");
  require(
      owner.federationNotSavedReasons().empty() &&
          constrained.federationNotSavedReasons().empty() &&
          observer.federationNotSavedReasons().empty(),
      "Federation MOM save-conditionals reported a failed save");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederationClass,
      conditionalAttributes);
  constrained.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  observer.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
