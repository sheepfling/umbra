#include "tck_fqr_process_scenarios.hpp"

#include "tck_nmr_process_coordination.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runFqrQueuedTsoRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session receiver(options, model, "owner");
  receiver.connect();
  auto const federation = federationName(
      options,
      "timestamped-interaction-flush-queue-future-input-process");
  receiver.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  receiver.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = receiver.rtiAmbassador().getParameterHandle(
      interaction,
      options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "process-separated FQR requester lookup returned an invalid handle");
  receiver.rtiAmbassador().subscribeInteractionClass(interaction, true);

  auto const receiverTime = makeTimeContext(receiver);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "process-separated FQR requester Time Constrained callback");
  writeNmrProcessMarker(directory, "fqr-requester-ready.marker");
  waitForNmrProcessMarker(directory, "fqr-publisher-joined.marker", options);
  receiver.rtiAmbassador().enableTimeRegulation(*receiverTime.epsilon);
  writeNmrProcessMarker(directory, "fqr-requester-regulation-requested.marker");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "process-separated FQR requester Time Regulation callback");
  waitForNmrProcessMarker(directory, "fqr-publisher-message-sent.marker", options);
  require(
      receiver.recorder().timedInteractions().empty(),
      "process-separated FQR delivered the queued TSO interaction before the request");

  auto const requestTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  auto const messageTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      7U);
  auto observedGalt = receiverTime.factory->makeInitial();
  require(
      observedGalt != nullptr && receiver.rtiAmbassador().queryGALT(*observedGalt),
      "process-separated FQR requester could not query GALT");
  require(
      *messageTime < *requestTime,
      "process-separated FQR message timestamp must be below its request");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*requestTime);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().flushQueueGrants().size() == 1U;
      },
      options,
      "process-separated FQR queued interaction and grant");

  auto expectedGrant = receiverTime.factory->makeInitial();
  require(
      expectedGrant != nullptr,
      "process-separated FQR could not allocate an expected grant time");
  *expectedGrant = *requestTime;
  if (*observedGalt < *expectedGrant) {
    *expectedGrant = *observedGalt;
  }
  if (*messageTime < *expectedGrant) {
    *expectedGrant = *messageTime;
  }
  auto const interactions = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().flushQueueGrants();
  std::vector<std::uint8_t> const expectedPayload{
      0x46U, 0x51U, 0x52U, 0x2DU, 0x50U};
  std::vector<std::uint8_t> const expectedTag{0x46U, 0x51U, 0x52U};
  require(
      grants.front().time == encodeTime(*expectedGrant) &&
          grants.front().optimisticTime == encodeTime(*messageTime),
      "process-separated FQR grant or OLT did not reflect request, GALT, and queued TSO");
  require(
      interactions.front().interaction == interaction &&
          interactions.front().parameters.size() == 1U &&
          interactions.front().parameters.count(parameter) == 1U &&
          copyBytes(interactions.front().parameters.at(parameter)) ==
              expectedPayload &&
          interactions.front().tag == expectedTag &&
          interactions.front().time == encodeTime(*messageTime) &&
          interactions.front().producer.isValid() &&
          interactions.front().sentOrder == rti::TIMESTAMP &&
          interactions.front().receivedOrder == rti::TIMESTAMP &&
          interactions.front().retractionPresent,
      "process-separated FQR changed queued interaction identity, time, source, or order");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "process-separated FQR grant preceded its queued TSO interaction callback");
  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "process-separated FQR could not allocate Query Logical Time output");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *expectedGrant,
      "process-separated FQR logical time did not match its grant");

  writeNmrProcessMarker(directory, "fqr-requester-completed.marker");
  waitForNmrProcessMarker(directory, "fqr-publisher-resigned.marker", options);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
}

void runFqrQueuedTsoPublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "fqr-requester-ready.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options,
      "timestamped-interaction-flush-queue-future-input-process");
  publisher.join(options.memberFederateName, options.federateType, federation);

  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interaction,
      options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "process-separated FQR publisher lookup returned an invalid handle");
  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);
  writeNmrProcessMarker(directory, "fqr-publisher-joined.marker");
  waitForNmrProcessMarker(
      directory,
      "fqr-requester-regulation-requested.marker",
      options);

  auto const publisherTime = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "process-separated FQR publisher Time Regulation callback");

  std::vector<std::uint8_t> const payload{0x46U, 0x51U, 0x52U, 0x2DU, 0x50U};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x51U, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      interaction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "process-separated FQR publisher returned an invalid retraction handle");
  writeNmrProcessMarker(directory, "fqr-publisher-message-sent.marker");
  waitForNmrProcessMarker(directory, "fqr-requester-completed.marker", options);
  publisher.resign(rti::NO_ACTION);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "fqr-publisher-resigned.marker");
}

}  // namespace

void runFqrQueuedTsoProcesses(
    Options const& options,
    rti::CallbackModel model) {
  auto const directory = nextNmrProcessRunDirectory(options);
  if (options.nmrProcessRole == "requester") {
    runFqrQueuedTsoRequesterProcess(options, model, directory);
    return;
  }
  if (options.nmrProcessRole == "publisher") {
    runFqrQueuedTsoPublisherProcess(options, model, directory);
    return;
  }
  throw std::runtime_error("FQR process role must be requester or publisher");
}

}  // namespace hla_rti_cpp_tck::detail
