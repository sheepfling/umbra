#include "tck_tara_future_timestamped_object_deletion.hpp"

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTaraFutureTimestampedObjectDeletion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "TARA future timestamped object deletion requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "tara-future-timestamped-object-deletion");

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          receiverClass.isValid() && receiverAttribute.isValid(),
      "TARA future timestamped object deletion lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "TARA future timestamped object deletion members selected different logical-time factories");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TARA future timestamped object deletion time-constrained callback");

  auto const publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "TARA future timestamped object deletion could not allocate publisher lookahead");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TARA future timestamped object deletion time-regulation callback");

  auto const publisherCurrentTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherCurrentTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TARA future timestamped object deletion regulator advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "TARA future timestamped object deletion regulator advanced to the wrong time");

  auto const deletionTime = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      3U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const expectedGalt = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      1U);
  auto actualGalt = receiverTime.factory->makeInitial();
  require(
      actualGalt != nullptr && receiver.rtiAmbassador().queryGALT(*actualGalt),
      "TARA future timestamped object deletion did not expose a defined GALT");
  requireTimeEquals(
      *actualGalt,
      *expectedGalt,
      "TARA future timestamped object deletion pre-request GALT");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "TARA future timestamped object deletion registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "TARA future timestamped object deletion object discovery");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*requestBoundary);
  receiver.pump();
  require(
      receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "TARA future timestamped object deletion completed before its future event arrived");

  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x52U, 0x41U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *deletionTime);
  require(
      retraction.isValid(),
      "TARA future timestamped object deletion returned an invalid retraction handle");

  auto const publisherAdvanceTarget = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      4U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvanceTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "TARA future timestamped object deletion regulator follow-up advance");
  require(
      publisher.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*publisherAdvanceTarget),
      "TARA future timestamped object deletion regulator follow-up grant was wrong");

  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedRemovals().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "TARA future timestamped object deletion callbacks");

  auto const removals = receiver.recorder().timedRemovals();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      removals.size() == 1U && grants.size() == 1U,
      "TARA future timestamped object deletion returned duplicate or missing callbacks");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "TARA future timestamped object deletion granted before its future removal");

  auto const& removal = removals.front();
  require(
      removal.object == object && removal.producer == publisher.federateHandle(),
      "TARA future timestamped object deletion returned the wrong removal identity");
  require(
      removal.tag == tagBytes,
      "TARA future timestamped object deletion did not preserve the user tag");
  require(
      removal.time == encodeTime(*deletionTime) && !removal.timeText.empty() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP,
      "TARA future timestamped object deletion returned the wrong time or order metadata");
  require(
      removal.retractionPresent &&
          removal.retraction == copyBytes(retraction.encode()),
      "TARA future timestamped object deletion omitted or changed the retraction metadata");
  require(
      grants.front().encoded == encodeTime(*requestBoundary),
      "TARA future timestamped object deletion returned the wrong grant time");
  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "TARA future timestamped object deletion could not allocate queried logical time");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *requestBoundary,
      "TARA future timestamped object deletion logical-time query");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioTaraFutureTimestampedObjectDeletion(
    Options const& options,
    rti::CallbackModel model) {
  runTaraFutureTimestampedObjectDeletion(options, model);
}

void scenarioTaraFutureTimestampedObjectDeletionContract(
    Options const& options,
    rti::CallbackModel model) {
  runTaraFutureTimestampedObjectDeletion(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
