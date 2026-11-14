#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void verifyMomOwnershipTransferProhibition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "MOM ownership-transfer coverage requires an application FOM, the standard MIM, and logical-time configuration");

  Session subject(options, model, "mom-ownership-subject");
  Session observer(options, model, "mom-ownership-observer");
  auto const federation = federationName(options, "mom-ownership-transfer-prohibition");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);

  observer.join(
      options.memberFederateName + L"-mom-ownership-observer",
      options.federateType,
      federation);
  auto& observerRti = observer.rtiAmbassador();
  auto const momClass = observerRti.getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = observerRti.getAttributeHandle(
      momClass, L"HLAfederateHandle");
  auto const federateNameAttribute = observerRti.getAttributeHandle(
      momClass, L"HLAfederateName");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          federateNameAttribute.isValid() &&
          observerRti.getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          observerRti.getAttributeName(momClass, federateHandleAttribute) ==
              L"HLAfederateHandle" &&
          observerRti.getAttributeName(momClass, federateNameAttribute) ==
              L"HLAfederateName",
      "The standard MIM does not expose the expected joined-federate MOM handles");
  observerRti.subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          federateNameAttribute});

  subject.join(
      options.ownerFederateName + L"-mom-ownership-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const found = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (found == reflections.end()) {
      return std::nullopt;
    }
    return *found;
  };
  waitFor(
      observer,
      [&] { return findSubjectMomReflection().has_value(); },
      options,
      "joined-federate MOM object discovery and initial reflection");
  auto const subjectMomReflection = findSubjectMomReflection();
  require(
      subjectMomReflection.has_value() &&
          observerRti.getKnownObjectClassHandle(subjectMomReflection->object) ==
              momClass &&
          subjectMomReflection->values.count(federateNameAttribute) == 1U,
      "The public MOM reflection did not identify the subject federate object");
  auto const subjectMomObject = subjectMomReflection->object;

  // The API does not prescribe which service response applies to a federate's
  // request against an RTI-owned MOM attribute. The portable oracle is limited
  // to the clause's prohibition: the RTI must not transfer ownership or ask
  // the joined federate to release the MOM attribute.
  observerRti.publishObjectClassAttributes(
      momClass, rti::AttributeHandleSet{federateNameAttribute});
  auto verifyRtiOwnership = [&](std::string const& phase) {
    auto const ownedByRtiBefore =
        observer.recorder().ownershipOwnedByRtiEvents().size();
    auto const notOwnedBefore =
        observer.recorder().ownershipNotOwnedEvents().size();
    observerRti.queryAttributeOwnership(
        subjectMomObject,
        rti::AttributeHandleSet{federateNameAttribute});
    waitFor(
        observer,
        [&] {
          return observer.recorder().ownershipOwnedByRtiEvents().size() >
                  ownedByRtiBefore ||
              observer.recorder().ownershipNotOwnedEvents().size() >
                  notOwnedBefore;
        },
        options,
        phase + " ownership query for the predefined HLAfederateName attribute");
    auto const ownedByRti = observer.recorder().ownershipOwnedByRtiEvents();
    auto const notOwned = observer.recorder().ownershipNotOwnedEvents();
    require(
        ownedByRti.size() == ownedByRtiBefore + 1U &&
            ownedByRti.back().object == subjectMomObject &&
            ownedByRti.back().attributes ==
                rti::AttributeHandleSet{federateNameAttribute} &&
            notOwned.size() == notOwnedBefore,
        "The RTI no longer owned the predefined HLAfederateName MOM attribute " +
            phase);
  };
  verifyRtiOwnership("before ownership acquisition");
  auto const acquisitionEventsBefore =
      observer.recorder().ownershipAcquisitionEvents().size();
  auto const releaseRequestBefore = subject.recorder().ownershipReleaseRequest();
  require(
      !releaseRequestBefore.has_value(),
      "The subject already had an unrelated ownership-release request before MOM coverage");
  auto attemptAcquisition = [&](bool ifAvailable) {
    try {
      if (ifAvailable) {
        observerRti.attributeOwnershipAcquisitionIfAvailable(
            subjectMomObject,
            rti::AttributeHandleSet{federateNameAttribute},
            rti::VariableLengthData{});
      } else {
        observerRti.attributeOwnershipAcquisition(
            subjectMomObject,
            rti::AttributeHandleSet{federateNameAttribute},
            rti::VariableLengthData{});
      }
    } catch (rti::Exception const&) {
      // §11.4.1 prohibits the transfer, not a specific exception response.
    }
    auto const deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(250);
    while (std::chrono::steady_clock::now() < deadline) {
      observer.pump();
      subject.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto const acquisitionEvents =
        observer.recorder().ownershipAcquisitionEvents();
    require(
        acquisitionEvents.size() == acquisitionEventsBefore,
        ifAvailable
            ? "The RTI transferred a MOM attribute after acquisition-if-available"
            : "The RTI transferred a MOM attribute after negotiated acquisition");
    require(
        !subject.recorder().ownershipReleaseRequest().has_value(),
        ifAvailable
            ? "The RTI requested release of a MOM attribute during acquisition-if-available"
            : "The RTI requested release of a MOM attribute during negotiated acquisition");
  };

  attemptAcquisition(true);
  verifyRtiOwnership("after acquisition-if-available");
  attemptAcquisition(false);
  verifyRtiOwnership("after negotiated acquisition");

  observerRti.unpublishObjectClassAttributes(
      momClass, rti::AttributeHandleSet{federateNameAttribute});
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

}  // namespace

void scenarioMomOwnershipTransferProhibition(
    Options const& options,
    rti::CallbackModel model) {
  verifyMomOwnershipTransferProhibition(options, model);
}

void scenarioMomOwnershipTransferProhibitionContract(
    Options const& options,
    rti::CallbackModel model) {
  verifyMomOwnershipTransferProhibition(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
