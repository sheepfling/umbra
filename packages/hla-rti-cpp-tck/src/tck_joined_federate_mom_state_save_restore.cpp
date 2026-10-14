#include "tck_joined_federate_mom_state_save_restore.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomFederateStateSaveRestore(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM HLAfederateState testing requires an adapter-supplied standard MIM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-federate MOM HLAfederateState testing requires an adapter-supplied logical-time implementation");

  Session subject(options, model, "mom-state-subject");
  Session observer(options, model, "mom-state-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-federate-state-save-restore");
  auto const subjectName = options.ownerFederateName + L"-mom-state-subject";
  auto const observerName = options.memberFederateName + L"-mom-state-observer";
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(observerName, options.federateType, federation);

  auto const observerMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const observerFederateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerMomClass,
      L"HLAfederateHandle");
  auto const observerFederateStateAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerMomClass,
      L"HLAfederateState");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      observerMomClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerMomClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observerFederateHandleAttribute.isValid() &&
          observerFederateStateAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerMomClass,
              observerFederateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              observerMomClass,
              observerFederateStateAttribute) == L"HLAfederateState" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM HLAfederateState lookup did not resolve the standard surface");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerMomClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerFederateStateAttribute},
      true,
      L"");

  subject.join(subjectName, options.federateType, federation);
  auto const subjectMomClass = subject.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const subjectFederateHandleAttribute = subject.rtiAmbassador().getAttributeHandle(
      subjectMomClass,
      L"HLAfederateHandle");
  auto const subjectFederateStateAttribute = subject.rtiAmbassador().getAttributeHandle(
      subjectMomClass,
      L"HLAfederateState");
  require(
      subjectMomClass == observerMomClass &&
          subjectFederateHandleAttribute == observerFederateHandleAttribute &&
          subjectFederateStateAttribute == observerFederateStateAttribute,
      "Joined-federate MOM HLAfederateState members did not agree on standard handles");
  subject.rtiAmbassador().subscribeObjectClassAttributes(
      subjectMomClass,
      rti::AttributeHandleSet{
          subjectFederateHandleAttribute,
          subjectFederateStateAttribute},
      true,
      L"");

  auto const expectedSubjectFederateHandle = copyBytes(subject.federateHandle().encode());
  auto findMomReflection = [&](Session& session,
                               rti::AttributeHandle const& handleAttribute)
      -> std::optional<ReflectionRecord> {
    auto const reflections = session.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(handleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedSubjectFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitForSessions(
      {&subject, &observer},
      [&] {
        return findMomReflection(observer, observerFederateHandleAttribute).has_value() &&
            findMomReflection(subject, subjectFederateHandleAttribute).has_value();
      },
      options,
      "joined-federate MOM HLAfederateState object discovery");
  auto const observerInitial =
      findMomReflection(observer, observerFederateHandleAttribute);
  auto const subjectInitial =
      findMomReflection(subject, subjectFederateHandleAttribute);
  require(
      observerInitial.has_value() && observerInitial->object.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(observerInitial->object) ==
              observerMomClass &&
          subjectInitial.has_value() && subjectInitial->object.isValid() &&
          subject.rtiAmbassador().getKnownObjectClassHandle(subjectInitial->object) ==
              subjectMomClass,
      "Joined-federate MOM HLAfederateState discovery returned the wrong standard object");
  require(
      observerInitial->values.count(observerFederateStateAttribute) == 0U &&
          subjectInitial->values.count(subjectFederateStateAttribute) == 0U,
      "Joined-federate MOM HLAfederateState initial reflection was not conditional");
  auto const observerMomObject = observerInitial->object;
  auto const subjectMomObject = subjectInitial->object;

  auto findStateReflection = [&](Session& session,
                                 rti::ObjectInstanceHandle const& object,
                                 rti::AttributeHandle const& stateAttribute,
                                 std::size_t before)
      -> std::optional<ReflectionRecord> {
    auto const reflections = session.recorder().reflections();
    if (before > reflections.size()) {
      return std::nullopt;
    }
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == object &&
              reflection.values.count(stateAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto findExpectedStateReflection = [&](Session& session,
                                          rti::ObjectInstanceHandle const& object,
                                          rti::AttributeHandle const& stateAttribute,
                                          std::size_t before,
                                          std::int32_t expected)
      -> std::optional<ReflectionRecord> {
    auto const reflections = session.recorder().reflections();
    if (before > reflections.size()) {
      return std::nullopt;
    }
    for (auto iterator = reflections.begin() + static_cast<std::ptrdiff_t>(before);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != object ||
          iterator->values.count(stateAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE state;
      state.decode(iterator->values.at(stateAttribute));
      if (state.get() == expected) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  auto requireStateReflection = [&](Session& session,
                                    rti::ObjectInstanceHandle const& object,
                                    rti::AttributeHandle const& stateAttribute,
                                    std::size_t before,
                                    std::int32_t expected,
                                    std::string const& description) {
    waitForSessions(
        {&subject, &observer},
        [&] {
          return findExpectedStateReflection(
                     session,
                     object,
                     stateAttribute,
                     before,
                     expected)
              .has_value();
        },
        options,
        description);
    auto const reflection = findExpectedStateReflection(
        session,
        object,
        stateAttribute,
        before,
        expected);
    require(
        reflection.has_value() && reflection->values.size() == 1U &&
            reflection->transportation == reliable && reflection->tag.empty() &&
            !reflection->producer.isValid() && !reflection->regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE state;
    state.decode(reflection->values.at(stateAttribute));
    require(
        state.get() == expected,
        description + " returned the wrong HLAfederateState value");
  };

  auto const beforeInitialObserver = observer.recorder().reflections().size();
  auto const beforeInitialSubject = subject.recorder().reflections().size();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      observerMomObject,
      rti::AttributeHandleSet{observerFederateStateAttribute},
      rti::VariableLengthData{});
  subject.rtiAmbassador().requestAttributeValueUpdate(
      subjectMomObject,
      rti::AttributeHandleSet{subjectFederateStateAttribute},
      rti::VariableLengthData{});
  requireStateReflection(
      observer,
      observerMomObject,
      observerFederateStateAttribute,
      beforeInitialObserver,
      1,
      "joined-federate MOM HLAfederateState initial observer request");
  requireStateReflection(
      subject,
      subjectMomObject,
      subjectFederateStateAttribute,
      beforeInitialSubject,
      1,
      "joined-federate MOM HLAfederateState initial self request");

  std::wstring const saveLabel = L"mom-state-save";
  auto const beforeSaveObserver = observer.recorder().reflections().size();
  auto const beforeSaveSubject = subject.recorder().reflections().size();
  subject.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federateSaveInitiations().size() >= 1U &&
            observer.federateSaveInitiations().size() >= 1U &&
            findStateReflection(
                observer,
                observerMomObject,
                observerFederateStateAttribute,
                beforeSaveObserver)
                .has_value();
      },
      options,
      "joined-federate MOM HLAfederateState save initiation");
  require(
      subject.federateSaveInitiations().front() == saveLabel &&
          observer.federateSaveInitiations().front() == saveLabel,
      "Joined-federate MOM HLAfederateState save initiation returned the wrong label");
  requireStateReflection(
      observer,
      observerMomObject,
      observerFederateStateAttribute,
      beforeSaveObserver,
      3,
      "joined-federate MOM HLAfederateState save initiation reflection");
  for (int pass = 0; pass != 8; ++pass) {
    subject.pump();
    observer.pump();
  }
  auto const subjectSaveReflections = subject.recorder().reflections();
  auto const subjectSaveStateReflection = std::find_if(
      subjectSaveReflections.begin() + static_cast<std::ptrdiff_t>(beforeSaveSubject),
      subjectSaveReflections.end(),
      [&](ReflectionRecord const& reflection) {
        return reflection.object == subjectMomObject &&
            reflection.values.count(subjectFederateStateAttribute) == 1U;
      });
  require(
      subjectSaveStateReflection == subjectSaveReflections.end(),
      "Joined-federate MOM HLAfederateState exposed the saving federate's own state transition");

  subject.rtiAmbassador().federateSaveBegun();
  observer.rtiAmbassador().federateSaveBegun();
  subject.rtiAmbassador().federateSaveComplete();
  auto const beforeSaveCompletion = observer.recorder().reflections().size();
  observer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationSavedCount() >= 1U &&
            observer.federationSavedCount() >= 1U &&
            findStateReflection(
                observer,
                observerMomObject,
                observerFederateStateAttribute,
                beforeSaveCompletion)
                .has_value();
      },
      options,
      "joined-federate MOM HLAfederateState save completion");
  requireStateReflection(
      observer,
      observerMomObject,
      observerFederateStateAttribute,
      beforeSaveCompletion,
      1,
      "joined-federate MOM HLAfederateState save completion reflection");
  require(
      subject.federationNotSavedReasons().empty() &&
          observer.federationNotSavedReasons().empty(),
      "Joined-federate MOM HLAfederateState save reported a failure");

  auto const beforeRestore = observer.recorder().reflections().size();
  subject.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationRestoreRequestsSucceeded().size() >= 1U &&
            subject.federationRestoreBegunCount() >= 1U &&
            observer.federationRestoreBegunCount() >= 1U &&
            subject.federateRestoreInitiations().size() >= 1U &&
            observer.federateRestoreInitiations().size() >= 1U &&
            findStateReflection(
                observer,
                observerMomObject,
                observerFederateStateAttribute,
                beforeRestore)
                .has_value();
      },
      options,
      "joined-federate MOM HLAfederateState restore initiation");
  require(
      subject.federationRestoreRequestsSucceeded().back() == saveLabel &&
          subject.federationRestoreRequestsFailed().empty() &&
          observer.federationRestoreRequestsFailed().empty(),
      "Joined-federate MOM HLAfederateState restore request returned an unexpected result");
  auto const subjectRestore = subject.federateRestoreInitiations().back();
  auto const observerRestore = observer.federateRestoreInitiations().back();
  require(
      subjectRestore.label == saveLabel && observerRestore.label == saveLabel &&
          subjectRestore.federateName == subjectName &&
          observerRestore.federateName == observerName &&
          subjectRestore.postRestoreFederateHandle.isValid() &&
          observerRestore.postRestoreFederateHandle.isValid(),
      "Joined-federate MOM HLAfederateState restore initiation returned incorrect identity data");
  requireStateReflection(
      observer,
      observerMomObject,
      observerFederateStateAttribute,
      beforeRestore,
      5,
      "joined-federate MOM HLAfederateState restore initiation reflection");

  subject.rtiAmbassador().federateRestoreComplete();
  auto const beforeRestoreCompletion = observer.recorder().reflections().size();
  observer.rtiAmbassador().federateRestoreComplete();
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationRestoredCount() >= 1U &&
            observer.federationRestoredCount() >= 1U &&
            findExpectedStateReflection(
                observer,
                observerMomObject,
                observerFederateStateAttribute,
                beforeRestoreCompletion,
                1)
                .has_value();
      },
      options,
      "joined-federate MOM HLAfederateState restore completion");
  requireStateReflection(
      observer,
      observerMomObject,
      observerFederateStateAttribute,
      beforeRestoreCompletion,
      1,
      "joined-federate MOM HLAfederateState restore completion reflection");
  require(
      subject.federationNotRestoredReasons().empty() &&
          observer.federationNotRestoredReasons().empty(),
      "Joined-federate MOM HLAfederateState restore reported a failure");

  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
