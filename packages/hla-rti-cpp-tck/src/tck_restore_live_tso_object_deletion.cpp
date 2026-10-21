#include "tck_restore_live_tso_object_deletion.hpp"

#include <array>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreLiveTsoObjectDeletion(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Live TSO deletion restore requires adapter FOM and time implementation");
  require(model == rti::HLA_EVOKED || model == rti::HLA_IMMEDIATE,
          "Live TSO deletion restore requires a standard callback model");
  bool const immediate = model == rti::HLA_IMMEDIATE;

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-live-tso-object-deletion");
  std::wstring const saveLabel = L"tck-live-tso-deletion-baseline";
  std::array<std::uint8_t, 5> const tagBytes{'S', 'A', 'V', '-', 'D'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  std::vector<std::uint8_t> const expectedTag(tagBytes.begin(), tagBytes.end());

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName,
                 options.federateType + L"-publisher", federation);
  receiver.join(options.memberFederateName,
                options.federateType + L"-subscriber", federation);

  auto const pubTime = makeTimeContext(publisher);
  auto const recvTime = makeTimeContext(receiver);
  require(pubTime.factory->getName() == recvTime.factory->getName(),
          "Live deletion members selected different time factories");
  auto const time1 = timeAfter(*recvTime.factory, *recvTime.initial, *recvTime.epsilon, 1U);
  auto const time6 = timeAfter(*recvTime.factory, *recvTime.initial, *recvTime.epsilon, 6U);
  auto lookahead = pubTime.factory->makeZero();
  require(lookahead != nullptr, "Could not allocate live deletion lookahead");
  for (unsigned step = 0; step != 5U; ++step) *lookahead += *pubTime.epsilon;
  auto const encoded6 = encodeTime(*time6);

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const reliable = publisher.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  auto const secondary = publisher.rtiAmbassador().getAttributeHandle(
      objectClass, options.secondaryAttributeName);
  require(objectClass.isValid() && reliable.isValid() && secondary.isValid() &&
              reliable != secondary,
          "Adapter FOM lacks configured object class or two distinct attributes");
  rti::AttributeHandleSet const attributes{reliable, secondary};
  publisher.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  waitForSessions({&publisher, &receiver}, [&] {
    return receiver.recorder().hasDiscovery(object);
  }, options, "live deletion object discovery");
  require(receiver.recorder().discoveryCount() == 1U,
          "Live deletion object was not discovered exactly once");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
          "Live deletion object name did not resolve at receiver");

  receiver.rtiAmbassador().enableTimeConstrained();
  if (!immediate) {
    waitFor(receiver, [&] {
      return receiver.recorder().timeConstrainedEnabled().size() == 1U;
    }, options, "live deletion receiver constrained enablement");
  }
  require(receiver.recorder().timeConstrainedEnabled().size() == 1U,
          "Live deletion receiver was not constrained");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  if (!immediate) {
    waitFor(publisher, [&] {
      return publisher.recorder().timeRegulationEnabled().size() == 1U;
    }, options, "live deletion publisher regulation enablement");
  }
  require(publisher.recorder().timeRegulationEnabled().size() == 1U,
          "Live deletion publisher was not regulating");

  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object, tag, *time6);
  require(retraction.isValid() && receiver.recorder().timedRemovals().empty() &&
              receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
          "Timestamped deletion arrived before the receiver grant");

  if (immediate) receiver.rtiAmbassador().disableCallbacks();
  receiver.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  if (immediate) receiver.rtiAmbassador().enableCallbacks();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federateSaveInitiations().size() == 1U &&
           receiver.federateSaveInitiations().size() == 1U;
  }, options, "live deletion two-member save initiation");
  require(publisher.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel},
          "Live deletion save initiated with unexpected label");
  while (receiver.rtiAmbassador().evokeCallback(0.0)) {
  }
  while (publisher.rtiAmbassador().evokeCallback(0.0)) {
  }
  publisher.rtiAmbassador().federateSaveBegun();
  receiver.rtiAmbassador().federateSaveBegun();
  publisher.rtiAmbassador().federateSaveComplete();
  receiver.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federationSavedCount() == 1U &&
           receiver.federationSavedCount() == 1U;
  }, options, "live deletion federation saved");
  require(publisher.federationNotSavedReasons().empty() &&
              receiver.federationNotSavedReasons().empty(),
          "Live deletion federation save failed");

  publisher.rtiAmbassador().retract(retraction);
  receiver.pump();
  require(receiver.recorder().timedRemovals().empty() &&
              receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
          "Post-save retraction delivered the queued deletion");
  requireException([&] { publisher.rtiAmbassador().retract(retraction); },
                   L"MessageCanNoLongerBeRetracted",
                   "terminal post-save deletion designator");

  publisher.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           receiver.federateRestoreInitiations().size() == 1U;
  }, options, "live deletion restore initiation");
  require(publisher.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              publisher.federateRestoreInitiations().front().label == saveLabel &&
              receiver.federateRestoreInitiations().front().label == saveLabel,
          "Live deletion restore used an unexpected label");
  publisher.rtiAmbassador().federateRestoreComplete();
  receiver.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           receiver.federationRestoredCount() == 1U;
  }, options, "live deletion federation restored");
  require(publisher.federationNotRestoredReasons().empty() &&
              receiver.federationNotRestoredReasons().empty(),
          "Live deletion federation restore failed");

  receiver.rtiAmbassador().flushQueueRequest(*time6);
  if (!immediate) {
    waitFor(receiver, [&] {
      return receiver.recorder().timedRemovals().size() == 1U &&
             receiver.recorder().flushQueueGrants().size() == 1U;
    }, options, "restored live deletion and flush grant");
  }
  auto const removals = receiver.recorder().timedRemovals();
  auto const flushGrants = receiver.recorder().flushQueueGrants();
  auto const order = receiver.recorder().callbackOrder();
  require(removals.size() == 1U,
          "Restored deletion was not delivered exactly once");
  require(flushGrants.size() == 1U,
          "Restored deletion flush grant was not delivered exactly once");
  require(flushGrants.front().time == encoded6,
          "Restored deletion flush grant had time " +
              std::string(flushGrants.front().timeText.begin(),
                          flushGrants.front().timeText.end()) +
              " instead of the requested time six");
  require(order.size() >= 2U &&
              std::vector<std::string>(order.end() - 2, order.end()) ==
                  std::vector<std::string>{"remove", "flush-grant"},
          "Restored deletion did not precede the flush grant");
  auto const& removal = removals.front();
  require(removal.object == object &&
              removal.producer == publisher.federateHandle() &&
              removal.tag == expectedTag && removal.time == encoded6 &&
              removal.sentOrder == rti::TIMESTAMP &&
              removal.receivedOrder == rti::TIMESTAMP &&
              removal.retractionPresent &&
              removal.retraction == copyBytes(retraction.encode()),
          "Restored deletion lost public callback metadata or retraction handle");
  requireException([&] { receiver.rtiAmbassador().getObjectInstanceHandle(objectName); },
                   L"ObjectInstanceNotKnown",
                   "removed object name after restored deletion");

  publisher.rtiAmbassador().retract(retraction);
  if (!immediate) {
    waitFor(receiver, [&] { return receiver.recorder().retractions().size() == 1U; },
            options, "request retraction after restored deletion");
  }
  auto const requests = receiver.recorder().retractions();
  require(requests.size() == 1U && requests.front().valid &&
              requests.front().encoded == copyBytes(retraction.encode()) &&
              receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
          "Request Retraction did not restore the object lookup identity");
  requireException([&] { publisher.rtiAmbassador().retract(retraction); },
                   L"MessageCanNoLongerBeRetracted",
                   "terminal saved deletion designator after retraction");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(objectClass, attributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreLiveTsoObjectDeletion(
    Options const& options, rti::CallbackModel model) {
  runRestoreLiveTsoObjectDeletion(options, model);
}

void scenarioRestoreLiveTsoObjectDeletionContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreLiveTsoObjectDeletion(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
