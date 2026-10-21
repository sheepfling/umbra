#include "tck_restore_live_tso_attribute_multi_recipient.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

struct RecipientHandles {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
};

RecipientHandles subscribe(Session& receiver, Options const& options) {
  auto const objectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = receiver.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter FOM lacks recipient object class or attribute");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      objectClass, rti::AttributeHandleSet{attribute});
  return {objectClass, attribute};
}

void checkReflected(Session& receiver, RecipientHandles const& handles,
                    rti::ObjectInstanceHandle const& object,
                    rti::FederateHandle const& publisherHandle,
                    rti::MessageRetractionHandle const& saved,
                    rti::LogicalTime const& time7,
                    std::array<std::uint8_t, 5> const& valueBytes,
                    std::array<std::uint8_t, 5> const& tagBytes) {
  auto const reflections = receiver.recorder().timedReflections();
  require(reflections.size() == 1U && receiver.recorder().flushQueueGrants().size() == 1U,
          "Recipient did not receive exactly one restored reflection and FQG");
  auto const& reflection = reflections.front();
  require(reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(handles.attribute) == 1U &&
              copyBytes(reflection.values.at(handles.attribute)) ==
                  std::vector<std::uint8_t>(valueBytes.begin(), valueBytes.end()) &&
              reflection.tag ==
                  std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
              reflection.producer == publisherHandle && !reflection.regions.has_value() &&
              reflection.time == copyBytes(time7.encode()) &&
              reflection.sentOrder == rti::TIMESTAMP &&
              reflection.receivedOrder == rti::TIMESTAMP &&
              reflection.retractionPresent &&
              reflection.retraction == copyBytes(saved.encode()) &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"reflect", "flush-grant"},
          "Restored recipient reflection or callback ordering changed");
}

void runRestoreLiveTsoAttributeMultiRecipient(Options const& options,
                                              rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Multi-recipient attribute restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Multi-recipient attribute restore requires adapter FOM and time implementation");
  Session publisher(options, model, "owner");
  Session first(options, model, "member-a");
  Session second(options, model, "member-b");
  auto const federation = federationName(options, "restore-live-tso-attribute-multi");
  std::wstring const saveLabel = L"tck-live-tso-attribute-multi-baseline";
  std::array<std::uint8_t, 5> const valueBytes{'M', 'U', 'L', 'T', 'I'};
  std::array<std::uint8_t, 5> const tagBytes{'M', 'E', 'M', '-', 'T'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  first.connect();
  second.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  first.join(options.memberFederateName + L"-a", options.federateType + L"-subscriber",
             federation);
  second.join(options.memberFederateName + L"-b", options.federateType + L"-subscriber",
              federation);
  auto const publisherHandle = publisher.federateHandle();
  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter FOM lacks publisher object class or attribute");
  rti::AttributeHandleSet const attributes{attribute};
  rti::AttributeHandleValueMap values;
  values.emplace(attribute,
                 rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  publisher.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  auto const firstHandles = subscribe(first, options);
  auto const secondHandles = subscribe(second, options);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      objectClass, attributes, rti::TIMESTAMP);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Attribute update target was invalid");
  waitForSessions({&first, &second}, [&] {
    return first.recorder().hasDiscovery(object) && second.recorder().hasDiscovery(object);
  }, options, "both attribute target discoveries");
  first.rtiAmbassador().enableTimeConstrained();
  second.rtiAmbassador().enableTimeConstrained();
  first.pump();
  second.pump();
  auto const time = makeTimeContext(publisher);
  auto const time7 = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time10 = timeAfter(*time.factory, *time.initial, *time.epsilon, 10U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  auto const saved = publisher.rtiAmbassador().updateAttributeValues(
      object, values, tag, *time7);
  require(saved.isValid() && first.recorder().timedReflections().empty() &&
              second.recorder().timedReflections().empty(),
          "Timestamp-seven update was not queued for both recipients");
  first.rtiAmbassador().timeAdvanceRequest(*time1);
  second.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "three-member attribute save initiation");
  publisher.rtiAmbassador().federateSaveBegun();
  first.rtiAmbassador().federateSaveBegun();
  second.rtiAmbassador().federateSaveBegun();
  publisher.rtiAmbassador().federateSaveComplete();
  first.rtiAmbassador().federateSaveComplete();
  second.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationSavedCount() == 1U &&
           first.federationSavedCount() == 1U && second.federationSavedCount() == 1U;
  }, options, "three-member attribute federation saved");

  publisher.rtiAmbassador().retract(saved);
  first.pump();
  second.pump();
  require(first.recorder().timedReflections().empty() &&
              second.recorder().timedReflections().empty(),
          "Post-save retract leaked a queued reflection");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save terminal update handle");
  publisher.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           first.federateRestoreInitiations().size() == 1U &&
           second.federateRestoreInitiations().size() == 1U;
  }, options, "three-member attribute restore initiation");
  publisher.rtiAmbassador().federateRestoreComplete();
  first.rtiAmbassador().federateRestoreComplete();
  second.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           first.federationRestoredCount() == 1U &&
           second.federationRestoredCount() == 1U;
  }, options, "three-member attribute federation restored");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&first}, [&] {
    return first.recorder().timedReflections().size() == 1U &&
           first.recorder().flushQueueGrants().size() == 1U;
  }, options, "first restored reflection and FQG");
  require(second.recorder().timedReflections().empty() &&
              second.recorder().flushQueueGrants().empty() &&
              second.recorder().callbackOrder().empty(),
          "First FQR prematurely released the second reflection");
  second.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&second}, [&] {
    return second.recorder().timedReflections().size() == 1U &&
           second.recorder().flushQueueGrants().size() == 1U;
  }, options, "second restored reflection and FQG");
  checkReflected(first, firstHandles, object, publisherHandle, saved, *time7,
                 valueBytes, tagBytes);
  checkReflected(second, secondHandles, object, publisherHandle, saved, *time7,
                 valueBytes, tagBytes);

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().retract(saved);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().retractions().size() == 1U &&
           second.recorder().retractions().size() == 1U;
  }, options, "both restored attribute request-retraction callbacks");
  for (Session* receiver : {&first, &second}) {
    auto const retraction = receiver->recorder().retractions().front();
    require(retraction.valid && retraction.encoded == copyBytes(saved.encode()) &&
                receiver->recorder().callbackOrder() ==
                    std::vector<std::string>{"request-retraction"},
            "Restored attribute retraction callback differed between recipients");
  }
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal update handle");

  first.rtiAmbassador().unsubscribeObjectClassAttributes(
      firstHandles.objectClass, rti::AttributeHandleSet{firstHandles.attribute});
  second.rtiAmbassador().unsubscribeObjectClassAttributes(
      secondHandles.objectClass, rti::AttributeHandleSet{secondHandles.attribute});
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreLiveTsoAttributeMultiRecipient(Options const& options,
                                                   rti::CallbackModel model) {
  runRestoreLiveTsoAttributeMultiRecipient(options, model);
}

void scenarioRestoreLiveTsoAttributeMultiRecipientContract(Options const& options,
                                                           rti::CallbackModel model) {
  runRestoreLiveTsoAttributeMultiRecipient(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
