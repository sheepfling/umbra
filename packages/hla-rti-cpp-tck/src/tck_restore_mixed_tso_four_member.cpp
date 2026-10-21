#include "tck_restore_mixed_tso_four_member.hpp"

#include <algorithm>
#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreMixedTsoFourMember(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Four-member mixed TSO restore has native evidence only for HLA_EVOKED");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Mixed TSO restore requires adapter-supplied FOM and time implementation");
  Session owner(options, model, "owner");
  Session mirror(options, model, "member");
  Session sender(options, model, "member");
  Session observer(options, model, "member");
  auto const federation = federationName(options, "restore-mixed-tso-four-member");
  auto const objectName = federation + L"-object";
  std::wstring const saveLabel = L"tck-mixed-tso-four-member-save";
  std::array<std::uint8_t, 5> const attributePayload{'M', 'I', 'X', '-', 'A'};
  std::array<std::uint8_t, 5> const interactionPayload{'M', 'I', 'X', '-', 'I'};
  std::array<std::uint8_t, 5> const commonTagBytes{'M', 'I', 'X', '-', 'T'};
  std::array<std::uint8_t, 5> const deletionTagBytes{'M', 'I', 'X', '-', 'D'};
  rti::VariableLengthData const commonTag(commonTagBytes.data(), commonTagBytes.size());
  rti::VariableLengthData const deletionTag(deletionTagBytes.data(), deletionTagBytes.size());
  auto const expectedAttribute = std::vector<std::uint8_t>(
      attributePayload.begin(), attributePayload.end());
  auto const expectedInteraction = std::vector<std::uint8_t>(
      interactionPayload.begin(), interactionPayload.end());
  auto const expectedCommonTag = std::vector<std::uint8_t>(
      commonTagBytes.begin(), commonTagBytes.end());
  auto const expectedDeletionTag = std::vector<std::uint8_t>(
      deletionTagBytes.begin(), deletionTagBytes.end());

  owner.connect();
  mirror.connect();
  sender.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  mirror.join(L"tck-mixed-mirror", options.federateType, federation);
  sender.join(L"tck-mixed-sender", options.federateType, federation);
  observer.join(L"tck-mixed-observer", options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const mirrorClass = mirror.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const mirrorAttribute = mirror.rtiAmbassador().getAttributeHandle(
      mirrorClass, options.attributeName);
  auto const deletePrivilege = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, L"HLAprivilegeToDeleteObject");
  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const observerInteraction = observer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction, options.parameterName);
  auto const observerParameter = observer.rtiAmbassador().getParameterHandle(
      observerInteraction, options.parameterName);
  require(ownerClass.isValid() && mirrorClass.isValid() &&
              ownerAttribute.isValid() && mirrorAttribute.isValid() &&
              deletePrivilege.isValid() && senderInteraction.isValid() &&
              observerInteraction.isValid() && senderParameter.isValid() &&
              observerParameter.isValid(),
          "Adapter FOM lacks mixed TSO object, attribute, interaction, or parameter");
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass, rti::AttributeHandleSet{ownerAttribute, deletePrivilege});
  mirror.rtiAmbassador().subscribeObjectClassAttributes(
      mirrorClass, rti::AttributeHandleSet{mirrorAttribute});
  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  observer.rtiAmbassador().subscribeInteractionClass(observerInteraction);
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass, rti::AttributeHandleSet{ownerAttribute}, rti::TIMESTAMP);
  sender.rtiAmbassador().changeInteractionOrderType(
      senderInteraction, rti::TIMESTAMP);

  mirror.rtiAmbassador().enableTimeConstrained();
  observer.rtiAmbassador().enableTimeConstrained();
  require(!mirror.rtiAmbassador().evokeCallback(0.0) &&
              !observer.rtiAmbassador().evokeCallback(0.0),
          "Constrained recipients enabled before a grant boundary");
  auto const ownerTime = makeTimeContext(owner);
  auto const senderTime = makeTimeContext(sender);
  auto const mirrorTime = makeTimeContext(mirror);
  auto const observerTime = makeTimeContext(observer);
  require(ownerTime.factory->getName() == senderTime.factory->getName() &&
              ownerTime.factory->getName() == mirrorTime.factory->getName() &&
              ownerTime.factory->getName() == observerTime.factory->getName(),
          "Mixed TSO members selected different logical-time factories");
  auto const time1 = timeAfter(*ownerTime.factory, *ownerTime.initial,
                               *ownerTime.epsilon, 1U);
  auto const time7 = timeAfter(*ownerTime.factory, *ownerTime.initial,
                               *ownerTime.epsilon, 7U);
  auto const time8 = timeAfter(*ownerTime.factory, *ownerTime.initial,
                               *ownerTime.epsilon, 8U);
  auto const time10 = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                *ownerTime.epsilon, 10U);
  auto const encoded7 = encodeTime(*time7);
  auto const encoded8 = encodeTime(*time8);
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  sender.rtiAmbassador().enableTimeRegulation(*senderTime.epsilon);
  waitForSessions({&owner, &sender}, [&] {
    return owner.recorder().timeRegulationEnabled().size() == 1U &&
           sender.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "mixed TSO time-regulation enablement");

  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitForSessions({&owner}, [&] {
    return owner.recorder().reservationSucceeded(objectName);
  }, options, "mixed TSO object name reservation");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass, objectName);
  waitForSessions({&mirror}, [&] {
    return mirror.recorder().hasDiscovery(object);
  }, options, "mixed TSO object discovery");
  auto const mirrorObject = mirror.rtiAmbassador().getObjectInstanceHandle(objectName);
  require(mirror.recorder().discoveryCount() == 1U && mirrorObject == object,
          "Mixed TSO named object was not discovered by the mirror");

  rti::AttributeHandleValueMap attributeValues;
  attributeValues.emplace(ownerAttribute,
      rti::VariableLengthData(attributePayload.data(), attributePayload.size()));
  rti::ParameterHandleValueMap parameterValues;
  parameterValues.emplace(senderParameter,
      rti::VariableLengthData(interactionPayload.data(), interactionPayload.size()));
  auto const attributeRetraction = owner.rtiAmbassador().updateAttributeValues(
      object, attributeValues, commonTag, *time7);
  auto const interactionRetraction = sender.rtiAmbassador().sendInteraction(
      senderInteraction, parameterValues, commonTag, *time7);
  auto const deletionRetraction = owner.rtiAmbassador().deleteObjectInstance(
      object, deletionTag, *time8);
  require(attributeRetraction.isValid(),
          "Mixed TSO attribute update did not return a valid retraction handle");
  require(interactionRetraction.isValid(),
          "Mixed TSO interaction did not return a valid retraction handle");
  require(deletionRetraction.isValid(),
          "Mixed TSO deletion did not return a valid retraction handle");
  require(mirror.recorder().timedReflections().empty() &&
              observer.recorder().timedInteractions().empty(),
          "Mixed TSO submissions were delivered before recipient grants");

  owner.rtiAmbassador().requestFederationSave(saveLabel);
  owner.rtiAmbassador().timeAdvanceRequest(*time1);
  mirror.rtiAmbassador().timeAdvanceRequest(*time1);
  sender.rtiAmbassador().timeAdvanceRequest(*time1);
  observer.rtiAmbassador().timeAdvanceRequest(*time1);
  waitForSessions({&owner, &mirror, &sender, &observer}, [&] {
    return owner.federateSaveInitiations().size() == 1U &&
           mirror.federateSaveInitiations().size() == 1U &&
           sender.federateSaveInitiations().size() == 1U &&
           observer.federateSaveInitiations().size() == 1U;
  }, options, "mixed TSO four-member save initiation");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              mirror.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              sender.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              observer.federateSaveInitiations() == std::vector<std::wstring>{saveLabel},
          "Mixed TSO save did not initiate for every member");
  for (Session* member : {&owner, &mirror, &sender, &observer})
    member->rtiAmbassador().federateSaveBegun();
  for (Session* member : {&owner, &mirror, &sender, &observer})
    member->rtiAmbassador().federateSaveComplete();
  waitForSessions({&owner, &mirror, &sender, &observer}, [&] {
    return owner.federationSavedCount() == 1U &&
           mirror.federationSavedCount() == 1U &&
           sender.federationSavedCount() == 1U &&
           observer.federationSavedCount() == 1U;
  }, options, "mixed TSO four-member federation saved");

  owner.rtiAmbassador().retract(attributeRetraction);
  sender.rtiAmbassador().retract(interactionRetraction);
  owner.rtiAmbassador().retract(deletionRetraction);
  require(mirror.recorder().timedReflections().empty() &&
              observer.recorder().timedInteractions().empty(),
          "Mixed TSO retraction before restore released queued callbacks");
  requireException([&] { owner.rtiAmbassador().retract(attributeRetraction); },
                   L"MessageCanNoLongerBeRetracted", "terminal attribute retraction");
  requireException([&] { sender.rtiAmbassador().retract(interactionRetraction); },
                   L"MessageCanNoLongerBeRetracted", "terminal interaction retraction");
  requireException([&] { owner.rtiAmbassador().retract(deletionRetraction); },
                   L"MessageCanNoLongerBeRetracted", "terminal deletion retraction");

  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&owner, &mirror, &sender, &observer}, [&] {
    return owner.recorder().federationRestoreRequestsSucceeded().size() == 1U &&
           mirror.federateRestoreInitiations().size() == 1U &&
           observer.federateRestoreInitiations().size() == 1U;
  }, options, "mixed TSO four-member restore initiation");
  for (Session* member : {&owner, &mirror, &sender, &observer})
    member->rtiAmbassador().federateRestoreComplete();
  waitForSessions({&owner, &mirror, &sender, &observer}, [&] {
    return owner.federationRestoredCount() == 1U &&
           mirror.federationRestoredCount() == 1U &&
           sender.federationRestoredCount() == 1U &&
           observer.federationRestoredCount() == 1U;
  }, options, "mixed TSO four-member federation restored");

  mirror.recorder().clearCallbackOrder();
  observer.recorder().clearCallbackOrder();
  mirror.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&mirror}, [&] {
    return mirror.recorder().timedReflections().size() == 1U &&
           mirror.recorder().timedRemovals().size() == 1U &&
           mirror.recorder().flushQueueGrants().size() == 1U;
  }, options, "mirror restored reflection, removal, and flush grant");
  observer.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&observer}, [&] {
    return observer.recorder().timedInteractions().size() == 1U &&
           observer.recorder().flushQueueGrants().size() == 1U;
  }, options, "observer restored interaction and flush grant");
  auto const reflection = mirror.recorder().timedReflections().front();
  auto const removal = mirror.recorder().timedRemovals().front();
  auto const interaction = observer.recorder().timedInteractions().front();
  require(reflection.object == mirrorObject && reflection.values.size() == 1U &&
              copyBytes(reflection.values.at(mirrorAttribute)) == expectedAttribute &&
              reflection.tag == expectedCommonTag &&
              reflection.producer == owner.federateHandle() &&
              reflection.time == encoded7 && reflection.retractionPresent &&
              removal.object == mirrorObject && removal.tag == expectedDeletionTag &&
              removal.producer == owner.federateHandle() &&
              removal.time == encoded8 && removal.sentOrder == rti::TIMESTAMP &&
              removal.receivedOrder == rti::TIMESTAMP && removal.retractionPresent &&
              interaction.interaction == observerInteraction &&
              interaction.parameters.size() == 1U &&
              copyBytes(interaction.parameters.at(observerParameter)) == expectedInteraction &&
              interaction.tag == expectedCommonTag &&
              interaction.producer == sender.federateHandle() &&
              interaction.time == encoded7 && interaction.retractionPresent,
          "Mixed TSO restore changed public reflection, removal, or interaction fields");
  require(mirror.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "remove", "flush-grant"} &&
              observer.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "flush-grant"},
          "Mixed TSO flush queue did not deliver each callback before grant");

  owner.rtiAmbassador().retract(attributeRetraction);
  sender.rtiAmbassador().retract(interactionRetraction);
  owner.rtiAmbassador().retract(deletionRetraction);
  waitForSessions({&mirror, &observer}, [&] {
    return mirror.recorder().retractions().size() == 2U &&
           observer.recorder().retractions().size() == 1U;
  }, options, "mixed TSO restored retraction callbacks");
  auto const mirrorRetractions = mirror.recorder().retractions();
  auto const observerRetractions = observer.recorder().retractions();
  require(std::all_of(mirrorRetractions.begin(), mirrorRetractions.end(),
                      [](auto const& item) { return item.valid; }) &&
              observerRetractions.front().valid,
          "Mixed TSO restored retraction callbacks lacked valid handles");
  requireException([&] { owner.rtiAmbassador().retract(attributeRetraction); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal attribute retraction");
  requireException([&] { sender.rtiAmbassador().retract(interactionRetraction); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal interaction retraction");
  requireException([&] { owner.rtiAmbassador().retract(deletionRetraction); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal deletion retraction");

  mirror.rtiAmbassador().unsubscribeObjectClassAttributes(
      mirrorClass, rti::AttributeHandleSet{mirrorAttribute});
  observer.rtiAmbassador().unsubscribeInteractionClass(observerInteraction);
  mirror.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  sender.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  mirror.disconnect();
  observer.disconnect();
  sender.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioRestoreMixedTsoFourMember(
    Options const& options, rti::CallbackModel model) {
  runRestoreMixedTsoFourMember(options, model);
}

void scenarioRestoreMixedTsoFourMemberContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreMixedTsoFourMember(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
