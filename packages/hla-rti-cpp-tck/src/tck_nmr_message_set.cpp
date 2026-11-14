#include "tck_main_scenarios.hpp"
#include "tck_nmr_process_coordination.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

template <typename Operation>
auto withNmrHandleLookupContext(
    std::string operation_name,
    Operation&& operation) {
  try {
    return operation();
  } catch (rti::Exception const&) {
    throw std::runtime_error(operation_name + " threw an RTI exception");
  } catch (std::exception const& exception) {
    throw std::runtime_error(
        operation_name + ": " + exception.what());
  }
}

void requireNmrPending(
    Session& requester,
    Session& publisher,
    std::size_t expected_interactions,
    std::size_t expected_timed_interactions,
    std::size_t expected_grants,
    std::string const& context) {
  auto const deadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < deadline) {
    requester.pump();
    publisher.pump();
    require(
        requester.recorder().interactions().size() == expected_interactions &&
            requester.recorder().timedInteractions().size() ==
                expected_timed_interactions &&
            requester.recorder().timeAdvanceGrants().size() == expected_grants,
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void requireNmrPending(
    Session& requester,
    std::size_t expected_interactions,
    std::size_t expected_timed_interactions,
    std::size_t expected_grants,
    std::string const& context) {
  auto const deadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < deadline) {
    requester.pump();
    require(
        requester.recorder().interactions().size() == expected_interactions &&
            requester.recorder().timedInteractions().size() ==
                expected_timed_interactions &&
            requester.recorder().timeAdvanceGrants().size() == expected_grants,
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void runNmrRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session requester(options, model, "owner");
  requester.connect();
  auto const federation = federationName(
      options, "nmr-future-tso-arrives-while-pending");
  requester.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  writeNmrProcessMarker(directory, "federation-created.marker");
  requester.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = requester.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "NMR process requester could not resolve its interaction and parameter");
  requester.rtiAmbassador().subscribeInteractionClass(interaction, true);

  auto requesterTime = makeTimeContext(requester);
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR process requester Time Constrained callback");
  waitForNmrProcessMarker(directory, "publisher-ready.marker", options);

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const messageTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto const callbackOrderBeforeDelivery =
      requester.recorder().callbackOrder().size();
  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  requireNmrPending(
      requester,
      0U,
      0U,
      0U,
      "NMR process requester completed before the publisher sent a TSO");
  writeNmrProcessMarker(directory, "requester-pending.marker");

  waitFor(
      requester,
      [&] {
        return requester.recorder().timedInteractions().size() == 2U &&
            requester.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "independent publisher's NMR timestamped interaction cohort");

  auto const timedInteractions = requester.recorder().timedInteractions();
  auto const grants = requester.recorder().timeAdvanceGrants();
  std::vector<std::uint8_t> const firstPayload{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const secondPayload{0x4EU, 0x4DU, 0x52U, 0x32U};
  std::vector<std::uint8_t> const firstTag{0x58U};
  std::vector<std::uint8_t> const secondTag{0x59U};
  auto matchesReceived = [&](auto const& received,
                             std::vector<std::uint8_t> const& payload,
                             std::vector<std::uint8_t> const& tag) {
    return received.interaction == interaction &&
        received.parameters.size() == 1U &&
        received.parameters.count(parameter) == 1U &&
        copyBytes(received.parameters.at(parameter)) == payload &&
        received.tag == tag && received.time == encodeTime(*messageTime) &&
        received.sentOrder == rti::TIMESTAMP &&
        received.receivedOrder == rti::TIMESTAMP;
  };
  require(
      timedInteractions.size() == 2U &&
          ((matchesReceived(timedInteractions[0], firstPayload, firstTag) &&
            matchesReceived(timedInteractions[1], secondPayload, secondTag)) ||
           (matchesReceived(timedInteractions[0], secondPayload, secondTag) &&
            matchesReceived(timedInteractions[1], firstPayload, firstTag))) &&
          timedInteractions[0].producer.isValid() &&
          timedInteractions[0].producer == timedInteractions[1].producer,
      "NMR process requester lost, duplicated, or changed the TSO cohort");
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*messageTime),
      "NMR process requester did not grant at the delivered TSO timestamp");
  auto const callbackOrder = requester.recorder().callbackOrder();
  require(
      callbackOrder.size() == callbackOrderBeforeDelivery + 3U &&
          callbackOrder[callbackOrderBeforeDelivery] == "interaction" &&
          callbackOrder[callbackOrderBeforeDelivery + 1U] == "interaction" &&
          callbackOrder[callbackOrderBeforeDelivery + 2U] == "grant",
      "NMR process requester granted before its complete TSO cohort");

  waitForNmrProcessMarker(directory, "publisher-resigned.marker", options);
  requester.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
}

void runNmrPublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options, "nmr-future-tso-arrives-while-pending");
  publisher.join(options.memberFederateName, options.federateType, federation);

  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "NMR process publisher could not resolve its interaction and parameter");
  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);

  auto publisherTime = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR process publisher Time Regulation callback");
  writeNmrProcessMarker(directory, "publisher-ready.marker");
  waitForNmrProcessMarker(directory, "requester-pending.marker", options);

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> const firstPayload{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const secondPayload{0x4EU, 0x4DU, 0x52U, 0x32U};
  std::vector<std::uint8_t> const firstTag{0x58U};
  std::vector<std::uint8_t> const secondTag{0x59U};
  auto sendAtSameTimestamp = [&](std::vector<std::uint8_t> const& payload,
                                 std::vector<std::uint8_t> const& tagBytes) {
    rti::ParameterHandleValueMap values;
    values.emplace(
        parameter,
        rti::VariableLengthData(payload.data(), payload.size()));
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    return publisher.rtiAmbassador().sendInteraction(
        interaction, values, tag, *messageTime);
  };
  auto const firstRetraction = sendAtSameTimestamp(firstPayload, firstTag);
  auto const secondRetraction = sendAtSameTimestamp(secondPayload, secondTag);
  require(
      firstRetraction.isValid() && secondRetraction.isValid(),
      "NMR process publisher could not send its TSO cohort");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR process publisher Time Advance Grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "NMR process publisher advanced to an unexpected time");
  publisher.resign(rti::NO_ACTION);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "publisher-resigned.marker");
}

}  // namespace

void scenarioNmrQueuedRoAndEarliestTsoCohort(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR message-set case requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session publisher(options, model, "member");
  auto const federation = federationName(
      options, "nmr-queued-ro-and-earliest-tso-cohort");
  connectAndJoin(requester, publisher, options, federation, options.fom);

  auto const requesterInteraction =
      requester.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction, options.parameterName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  require(
      requesterInteraction.isValid() && requesterParameter.isValid() &&
          publisherInteraction.isValid() && publisherParameter.isValid() &&
          requesterInteraction == publisherInteraction &&
          requesterParameter == publisherParameter,
      "NMR message-set case could not resolve consistent adapter handles");
  requester.rtiAmbassador().subscribeInteractionClass(
      requesterInteraction, true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto publisherTime = makeTimeContext(publisher);
  require(
      requesterTime.factory->getName() == publisherTime.factory->getName(),
      "NMR message-set members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *publisherTime.initial,
      "NMR message-set initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR message-set requester Time Constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR message-set publisher Time Regulation callback");

  std::vector<std::uint8_t> const payload{0x4EU, 0x4DU, 0x52U};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  std::vector<std::uint8_t> const firstReceiveOrderTag{0x11U};
  std::vector<std::uint8_t> const secondReceiveOrderTag{0x12U};
  rti::VariableLengthData const firstReceiveOrderTagData(
      firstReceiveOrderTag.data(), firstReceiveOrderTag.size());
  rti::VariableLengthData const secondReceiveOrderTagData(
      secondReceiveOrderTag.data(), secondReceiveOrderTag.size());
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, firstReceiveOrderTagData);
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, secondReceiveOrderTagData);

  auto const messageAtFive = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  auto const messageAtEight = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 8U);
  auto const messageAtTwelve = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 12U);
  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 10U);
  auto const publisherTargetSix = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 6U);
  auto const publisherTargetNine = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 9U);
  auto const publisherTargetEleven = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 11U);

  auto sendTimestamped = [&](std::vector<std::uint8_t> const& tag_bytes,
                             rti::LogicalTime const& timestamp) {
    rti::VariableLengthData const tag(tag_bytes.data(), tag_bytes.size());
    auto const retraction = publisher.rtiAmbassador().sendInteraction(
        publisherInteraction, values, tag, timestamp);
    require(retraction.isValid(), "NMR message-set TSO send returned no retraction handle");
  };
  std::vector<std::uint8_t> const firstTsoTag{0x21U};
  std::vector<std::uint8_t> const secondTsoTag{0x22U};
  std::vector<std::uint8_t> const laterTsoTag{0x28U};
  std::vector<std::uint8_t> const beyondRequestTag{0x2CU};
  sendTimestamped(firstTsoTag, *messageAtFive);
  sendTimestamped(secondTsoTag, *messageAtFive);
  sendTimestamped(laterTsoTag, *messageAtEight);
  sendTimestamped(beyondRequestTag, *messageAtTwelve);

  require(
      requester.recorder().interactions().empty() &&
          requester.recorder().timedInteractions().empty(),
      "NMR message-set requester observed queued inputs before its request");
  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  waitFor(
      requester,
      [&] { return requester.recorder().interactions().size() == 2U; },
      options,
      "NMR message-set queued receive-order interactions");
  require(
      requester.recorder().timedInteractions().empty() &&
          requester.recorder().timeAdvanceGrants().empty(),
      "NMR message-set delivered TSO or granted before the source advanced");
  requireNmrPending(
      requester,
      publisher,
      2U,
      0U,
      0U,
      "NMR message-set completed before its first queued TSO became safe");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetSix);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timedInteractions().size() == 2U &&
            requester.recorder().timeAdvanceGrants().size() == 1U &&
            publisher.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMR message-set earliest TSO cohort");
  auto const firstCohort = requester.recorder().timedInteractions();
  auto firstCohortTags = std::vector<std::uint8_t>{
      firstCohort[0].tag.front(), firstCohort[1].tag.front()};
  std::sort(firstCohortTags.begin(), firstCohortTags.end());
  require(
      firstCohortTags == std::vector<std::uint8_t>{0x21U, 0x22U},
      "NMR message-set did not deliver the complete earliest equal-time cohort");
  for (auto const& interaction : firstCohort) {
    require(
        interaction.interaction == requesterInteraction &&
            interaction.parameters.size() == 1U &&
            interaction.parameters.count(requesterParameter) == 1U &&
            copyBytes(interaction.parameters.at(requesterParameter)) == payload &&
            interaction.time == encodeTime(*messageAtFive) &&
            interaction.producer == publisher.federateHandle() &&
            interaction.sentOrder == rti::TIMESTAMP &&
            interaction.receivedOrder == rti::TIMESTAMP,
        "NMR message-set changed earliest-cohort payload or delivery metadata");
  }
  auto receiveOrderTags = std::vector<std::uint8_t>{
      requester.recorder().interactions()[0].tag.front(),
      requester.recorder().interactions()[1].tag.front()};
  std::sort(receiveOrderTags.begin(), receiveOrderTags.end());
  require(
      receiveOrderTags == std::vector<std::uint8_t>{0x11U, 0x12U},
      "NMR message-set did not deliver all queued receive-order messages");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageAtFive) &&
          publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTargetSix),
      "NMR message-set first grant did not stop at the earliest cohort time");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  requireNmrPending(
      requester,
      publisher,
      2U,
      2U,
      1U,
      "NMR message-set selected a later TSO before its source advanced");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetNine);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timedInteractions().size() == 3U &&
            requester.recorder().timeAdvanceGrants().size() == 2U &&
            publisher.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "NMR message-set follow-on TSO");
  auto const secondCohort = requester.recorder().timedInteractions();
  require(
      secondCohort.back().tag == laterTsoTag &&
          secondCohort.back().time == encodeTime(*messageAtEight) &&
          requester.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*messageAtEight),
      "NMR message-set did not select the next queued timestamp-8 message");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  requireNmrPending(
      requester,
      publisher,
      2U,
      3U,
      2U,
      "NMR message-set selected a timestamp beyond the request boundary");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetEleven);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timeAdvanceGrants().size() == 3U &&
            publisher.recorder().timeAdvanceGrants().size() == 3U;
      },
      options,
      "NMR message-set no-eligible-TSO grant");
  require(
      requester.recorder().timedInteractions().size() == 3U &&
          requester.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*requestTime) &&
          publisher.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*publisherTargetEleven),
      "NMR message-set did not grant at the request time when only TSO(12) remained");

  auto queriedTime = requesterTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "NMR message-set could not allocate Query Logical Time value");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(*queriedTime, *requestTime, "NMR message-set final logical time");

  requester.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  publisher.disconnect();
}

void scenarioNmrFutureTsoArrivesWhilePending(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR future-arrival case requires an adapter-supplied logical-time implementation");

  if (!options.nmrProcessRole.empty()) {
    auto const processDirectory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmrRequesterProcess(options, model, processDirectory);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmrPublisherProcess(options, model, processDirectory);
      return;
    }
    throw std::runtime_error(
        "NMR process role must be either requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "NMR process coordination directory requires an explicit process role");

  Session requester(options, model, "owner");
  Session publisher(options, model, "member");
  auto const federation = federationName(
      options, "nmr-future-tso-arrives-while-pending");
  connectAndJoin(requester, publisher, options, federation, options.fom);

  auto const requesterInteraction =
      requester.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction, options.parameterName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  require(
      requesterInteraction.isValid() && requesterParameter.isValid() &&
          publisherInteraction.isValid() && publisherParameter.isValid() &&
          requesterInteraction == publisherInteraction &&
          requesterParameter == publisherParameter,
      "NMR future-arrival case could not resolve consistent adapter handles");
  requester.rtiAmbassador().subscribeInteractionClass(
      requesterInteraction, true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto publisherTime = makeTimeContext(publisher);
  require(
      requesterTime.factory->getName() == publisherTime.factory->getName(),
      "NMR future-arrival members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *publisherTime.initial,
      "NMR future-arrival initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR future-arrival requester Time Constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR future-arrival publisher Time Regulation callback");

  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 10U);
  auto const messageTime = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  requireNmrPending(
      requester,
      publisher,
      0U,
      0U,
      0U,
      "NMR completed before a post-request TSO arrival existed");

  std::vector<std::uint8_t> const firstPayload{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const secondPayload{0x4EU, 0x4DU, 0x52U, 0x32U};
  std::vector<std::uint8_t> const firstTag{0x58U};
  std::vector<std::uint8_t> const secondTag{0x59U};
  auto sendAtSameTimestamp = [&](std::vector<std::uint8_t> const& payload,
                                 std::vector<std::uint8_t> const& tagBytes) {
    rti::ParameterHandleValueMap values;
    values.emplace(
        publisherParameter,
        rti::VariableLengthData(payload.data(), payload.size()));
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    return publisher.rtiAmbassador().sendInteraction(
        publisherInteraction, values, tag, *messageTime);
  };
  auto const firstRetraction = sendAtSameTimestamp(firstPayload, firstTag);
  auto const secondRetraction = sendAtSameTimestamp(secondPayload, secondTag);
  require(
      firstRetraction.isValid() && secondRetraction.isValid(),
      "NMR future-arrival TSO cohort send returned no retraction handle");
  requireNmrPending(
      requester,
      publisher,
      0U,
      0U,
      0U,
      "NMR delivered the late TSO cohort or granted before its timestamp became safe");

  auto const callbackOrderBeforeDelivery =
      requester.recorder().callbackOrder().size();
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timedInteractions().size() == 2U &&
            requester.recorder().timeAdvanceGrants().size() == 1U &&
            publisher.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMR post-request timestamped interaction arrival");

  auto const timedInteractions = requester.recorder().timedInteractions();
  auto const requesterGrants = requester.recorder().timeAdvanceGrants();
  auto const publisherGrants = publisher.recorder().timeAdvanceGrants();
  auto const callbackOrder = requester.recorder().callbackOrder();
  auto matchesReceived = [&](auto const& received,
                             std::vector<std::uint8_t> const& payload,
                             std::vector<std::uint8_t> const& tag) {
    return received.interaction == requesterInteraction &&
        received.parameters.size() == 1U &&
        received.parameters.count(requesterParameter) == 1U &&
        copyBytes(received.parameters.at(requesterParameter)) == payload &&
        received.tag == tag && received.time == encodeTime(*messageTime) &&
        received.producer == publisher.federateHandle() &&
        received.sentOrder == rti::TIMESTAMP &&
        received.receivedOrder == rti::TIMESTAMP;
  };
  require(
      timedInteractions.size() == 2U &&
          ((matchesReceived(timedInteractions[0], firstPayload, firstTag) &&
            matchesReceived(timedInteractions[1], secondPayload, secondTag)) ||
           (matchesReceived(timedInteractions[0], secondPayload, secondTag) &&
            matchesReceived(timedInteractions[1], firstPayload, firstTag))),
      "NMR lost, duplicated, or changed a same-time post-request TSO cohort member");
  require(
      requesterGrants.size() == 1U &&
          requesterGrants.front().encoded == encodeTime(*messageTime) &&
          publisherGrants.size() == 1U &&
          publisherGrants.front().encoded == encodeTime(*publisherTarget),
      "NMR did not grant at the late TSO timestamp after the publisher advanced");
  require(
      callbackOrder.size() == callbackOrderBeforeDelivery + 3U &&
          callbackOrder[callbackOrderBeforeDelivery] == "interaction" &&
          callbackOrder[callbackOrderBeforeDelivery + 1U] == "interaction" &&
          callbackOrder[callbackOrderBeforeDelivery + 2U] == "grant",
      "NMR delivered the post-request TSO after rather than before its grant");

  requester.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  publisher.disconnect();
}

void scenarioNmrFutureTsoArrivesWhilePendingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrFutureTsoArrivesWhilePending(options, model);
}

namespace {

void requireDirectedPending(
    Session& requester,
    std::size_t expected_directed,
    std::size_t expected_grants,
    std::string const& context) {
  auto const deadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < deadline) {
    requester.pump();
    require(
        requester.recorder().timedDirectedInteractions().size() ==
                expected_directed &&
            requester.recorder().timeAdvanceGrants().size() == expected_grants,
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void requireDirectedDelivery(
    Session& requester,
    rti::ObjectInstanceHandle const& target,
    rti::InteractionClassHandle const& interaction,
    rti::FederateHandle const& producer,
    std::vector<std::uint8_t> const& encoded_time,
    std::vector<std::uint8_t> const& tag,
    std::size_t expected_grants) {
  auto const delivered = requester.recorder().timedDirectedInteractions();
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(
      delivered.size() == 1U && delivered.front().object == target &&
          delivered.front().interaction == interaction &&
          delivered.front().producer == producer &&
          delivered.front().parameters.empty() && delivered.front().tag == tag &&
          delivered.front().time == encoded_time &&
          delivered.front().sentOrder == rti::TIMESTAMP &&
          delivered.front().receivedOrder == rti::TIMESTAMP &&
          delivered.front().retractionPresent,
      "directed future-arrival case changed or duplicated the directed TSO");
  require(
      grants.size() == expected_grants &&
          grants.front().encoded == encoded_time,
      "directed future-arrival case granted at the wrong time");
  auto const order = requester.recorder().callbackOrder();
  auto const directed = std::find(order.begin(), order.end(), "directed");
  auto const grant = std::find(order.begin(), order.end(), "grant");
  require(
      directed != order.end() && grant != order.end() && directed < grant,
      "directed interaction callback did not precede its grant");
}

void prepareDirectedReceiver(Session& requester, Options const& options) {
  auto const objectClass = withNmrHandleLookupContext(
      "NMR directed receiver getObjectClassHandle",
      [&] {
        return requester.rtiAmbassador().getObjectClassHandle(
            options.secondaryDirectedObjectClassName);
      });
  auto const attribute = withNmrHandleLookupContext(
      "NMR directed receiver getAttributeHandle",
      [&] {
        return requester.rtiAmbassador().getAttributeHandle(
            objectClass, options.attributeName);
      });
  auto const interaction = withNmrHandleLookupContext(
      "NMR directed receiver getInteractionClassHandle",
      [&] {
        return requester.rtiAmbassador().getInteractionClassHandle(
            options.secondaryDirectedInteractionClassName);
      });
  require(
      objectClass.isValid() && attribute.isValid() && interaction.isValid(),
      "directed future-arrival receiver could not resolve its FOM handles");
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      objectClass, {attribute}, true, L"");
  rti::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(interaction);
  requester.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      objectClass, directedInteractions, true);
}

void prepareDirectedPublisher(Session& publisher, Options const& options) {
  auto const objectClass = withNmrHandleLookupContext(
      "NMR directed publisher getObjectClassHandle",
      [&] {
        return publisher.rtiAmbassador().getObjectClassHandle(
            options.secondaryDirectedObjectClassName);
      });
  auto const attribute = withNmrHandleLookupContext(
      "NMR directed publisher getAttributeHandle",
      [&] {
        return publisher.rtiAmbassador().getAttributeHandle(
            objectClass, options.attributeName);
      });
  auto const interaction = withNmrHandleLookupContext(
      "NMR directed publisher getInteractionClassHandle",
      [&] {
        return publisher.rtiAmbassador().getInteractionClassHandle(
            options.secondaryDirectedInteractionClassName);
      });
  require(
      objectClass.isValid() && attribute.isValid() && interaction.isValid(),
      "directed future-arrival publisher could not resolve its FOM handles");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      objectClass, {attribute});
  rti::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(interaction);
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      objectClass, directedInteractions);
  publisher.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);
}

void runNmrDirectedRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory,
    bool useAvailableRequest) {
  std::string const advanceName = useAvailableRequest ? "NMRA" : "NMR";
  Session requester(options, model, "owner");
  requester.connect();
  auto const federation = federationName(
      options,
      useAvailableRequest
          ? "nmra-directed-future-tso-arrives-while-pending"
          : "nmr-directed-future-tso-arrives-while-pending");
  requester.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  writeNmrProcessMarker(directory, "federation-created.marker");
  requester.join(options.ownerFederateName, options.federateType, federation);
  prepareDirectedReceiver(requester, options);
  auto const interaction = requester.rtiAmbassador().getInteractionClassHandle(
      options.secondaryDirectedInteractionClassName);
  auto requesterTime = makeTimeContext(requester);
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      advanceName + " directed process requester Time Constrained callback");
  waitForNmrProcessMarker(directory, "publisher-ready.marker", options);
  auto const expectedProducer = requester.rtiAmbassador().getFederateHandle(
      options.memberFederateName);
  require(
      expectedProducer.isValid(),
      "NMR directed process requester could not resolve the publisher handle");
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const messageTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  if (useAvailableRequest) {
    requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);
  } else {
    requester.rtiAmbassador().nextMessageRequest(*requestTime);
  }
  requireDirectedPending(
      requester,
      0U,
      0U,
      advanceName +
          " directed process requester completed before a directed TSO arrived");
  writeNmrProcessMarker(directory, "requester-pending.marker");
  waitFor(
      requester,
      [&] { return requester.recorder().discoveryCount() == 1U; },
      options,
      advanceName + " directed process target-object discovery");
  auto const targetDiscovery = requester.recorder().discovery();
  require(
      targetDiscovery.present && targetDiscovery.objectClass ==
          requester.rtiAmbassador().getObjectClassHandle(
              options.secondaryDirectedObjectClassName),
      "NMR directed process requester discovered the wrong target class");
  waitFor(
      requester,
      [&] {
        return requester.recorder().timedDirectedInteractions().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "independent publisher's " + advanceName +
          " directed timestamped interaction");
  std::vector<std::uint8_t> tag{0x4EU, 0x4DU, 0x52U};
  if (useAvailableRequest) {
    tag.push_back(0x41U);
  }
  tag.push_back(0x44U);
  requireDirectedDelivery(
      requester,
      targetDiscovery.object,
      interaction,
      expectedProducer,
      encodeTime(*messageTime),
      tag,
      1U);
  requester.resign(rti::NO_ACTION);
  writeNmrProcessMarker(directory, "requester-resigned.marker");
  waitForNmrProcessMarker(directory, "publisher-resigned.marker", options);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
}

void runNmrDirectedPublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory,
    bool useAvailableRequest) {
  std::string const advanceName = useAvailableRequest ? "NMRA" : "NMR";
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options,
      useAvailableRequest
          ? "nmra-directed-future-tso-arrives-while-pending"
          : "nmr-directed-future-tso-arrives-while-pending");
  publisher.join(options.memberFederateName, options.federateType, federation);
  prepareDirectedPublisher(publisher, options);
  auto publisherTime = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      advanceName + " directed process publisher Time Regulation callback");
  writeNmrProcessMarker(directory, "publisher-ready.marker");
  waitForNmrProcessMarker(directory, "requester-pending.marker", options);
  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.secondaryDirectedInteractionClassName);
  auto const target = publisher.rtiAmbassador().registerObjectInstance(
      objectClass);
  require(target.isValid(), "NMR directed process target registration failed");
  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> tag{0x4EU, 0x4DU, 0x52U};
  if (useAvailableRequest) {
    tag.push_back(0x41U);
  }
  tag.push_back(0x44U);
  rti::VariableLengthData encodedTag(tag.data(), tag.size());
  rti::ParameterHandleValueMap parameters;
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      interaction, target, parameters, encodedTag, *messageTime);
  require(
      retraction.isValid(),
      advanceName +
          " directed process publisher could not send the timestamped interaction");
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      useAvailableRequest ? 4U : 5U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      advanceName + " directed process publisher Time Advance Grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      advanceName + " directed process publisher advanced to an unexpected time");
  waitForNmrProcessMarker(directory, "requester-resigned.marker", options);
  publisher.resign(rti::NO_ACTION);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "publisher-resigned.marker");
}

}  // namespace

void scenarioNmrDirectedFutureTsoArrivesWhilePending(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty() &&
          !options.secondaryDirectedObjectClassName.empty() &&
          !options.secondaryDirectedInteractionClassName.empty(),
      "NMR directed future-arrival case requires adapter-supplied time and FOM names");

  if (!options.nmrProcessRole.empty()) {
    auto const processDirectory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmrDirectedRequesterProcess(options, model, processDirectory, false);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmrDirectedPublisherProcess(options, model, processDirectory, false);
      return;
    }
    throw std::runtime_error(
        "NMR process role must be either requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "NMR process coordination directory requires an explicit process role");

  Session requester(options, model, "owner");
  Session publisher(options, model, "member");
  auto const federation = federationName(
      options, "nmr-directed-future-tso-arrives-while-pending");
  connectAndJoin(requester, publisher, options, federation, options.fom);
  prepareDirectedReceiver(requester, options);
  prepareDirectedPublisher(publisher, options);
  auto requesterTime = makeTimeContext(requester);
  auto publisherTime = makeTimeContext(publisher);
  require(
      requesterTime.factory->getName() == publisherTime.factory->getName(),
      "NMR directed future-arrival members selected different time factories");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR directed requester Time Constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR directed publisher Time Regulation callback");
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  requireDirectedPending(
      requester,
      0U,
      0U,
      "NMR completed before the timestamped directed interaction arrived");

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.secondaryDirectedInteractionClassName);
  auto const target = publisher.rtiAmbassador().registerObjectInstance(
      objectClass);
  require(target.isValid(), "NMR directed future-arrival target registration failed");
  waitFor(
      requester,
      [&] { return requester.recorder().discoveryCount() == 1U; },
      options,
      "NMR directed future-arrival target discovery");
  auto const targetDiscovery = requester.recorder().discovery();
  require(
      targetDiscovery.present && targetDiscovery.object == target,
      "NMR receiver discovered a different directed target object");
  std::vector<std::uint8_t> const tag{0x4EU, 0x4DU, 0x52U, 0x44U};
  rti::VariableLengthData encodedTag(tag.data(), tag.size());
  rti::ParameterHandleValueMap parameters;
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      interaction, target, parameters, encodedTag, *messageTime);
  require(
      retraction.isValid(),
      "NMR directed future-arrival send returned no retraction handle");
  publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timedDirectedInteractions().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U &&
            publisher.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMR directed future-arrival delivery and grant");
  auto const expectedProducer = requester.rtiAmbassador().getFederateHandle(
      options.memberFederateName);
  require(
      expectedProducer.isValid(),
      "NMR directed requester could not resolve the publisher handle");
  requireDirectedDelivery(
      requester,
      target,
      interaction,
      expectedProducer,
      encodeTime(*messageTime),
      tag,
      1U);
  requester.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  publisher.disconnect();
}

void scenarioNmrDirectedFutureTsoArrivesWhilePendingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrDirectedFutureTsoArrivesWhilePending(options, model);
}

void scenarioNmraDirectedFutureTsoArrivesWhilePending(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty() &&
          !options.secondaryDirectedObjectClassName.empty() &&
          !options.secondaryDirectedInteractionClassName.empty(),
      "NMRA directed future-arrival case requires adapter-supplied time and FOM names");
  if (!options.nmrProcessRole.empty()) {
    auto const processDirectory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmrDirectedRequesterProcess(options, model, processDirectory, true);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmrDirectedPublisherProcess(options, model, processDirectory, true);
      return;
    }
    throw std::runtime_error(
        "NMR/NMRA process role must be either requester or publisher");
  }
  require(
      options.nmrProcessRole.empty() &&
          options.nmrProcessCoordinationDirectory.empty(),
      "NMRA directed future-arrival case does not use the NMR process-role adapter");

  Session requester(options, model, "owner");
  Session publisher(options, model, "member");
  auto const federation = federationName(
      options, "nmra-directed-future-tso-arrives-while-pending");
  connectAndJoin(requester, publisher, options, federation, options.fom);
  prepareDirectedReceiver(requester, options);
  prepareDirectedPublisher(publisher, options);
  auto requesterTime = makeTimeContext(requester);
  auto publisherTime = makeTimeContext(publisher);
  require(
      requesterTime.factory->getName() == publisherTime.factory->getName(),
      "NMRA directed future-arrival members selected different time factories");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA directed requester Time Constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA directed publisher Time Regulation callback");

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);
  requireDirectedPending(
      requester,
      0U,
      0U,
      "NMRA completed before a qualifying directed TSO arrived");

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.secondaryDirectedInteractionClassName);
  auto const target = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(target.isValid(), "NMRA directed future-arrival target registration failed");
  waitFor(
      requester,
      [&] { return requester.recorder().discoveryCount() == 1U; },
      options,
      "NMRA directed future-arrival target discovery");
  auto const targetDiscovery = requester.recorder().discovery();
  require(
      targetDiscovery.present && targetDiscovery.object == target &&
          targetDiscovery.objectClass == objectClass,
      "NMRA receiver discovered a different directed target object");

  std::vector<std::uint8_t> const tag{
      0x4EU, 0x4DU, 0x52U, 0x41U, 0x44U};
  rti::VariableLengthData encodedTag(tag.data(), tag.size());
  rti::ParameterHandleValueMap parameters;
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      interaction, target, parameters, encodedTag, *messageTime);
  require(
      retraction.isValid(),
      "NMRA directed future-arrival send returned no retraction handle");
  requireDirectedPending(
      requester,
      0U,
      0U,
      "NMRA delivered or granted before the sender advanced to the message floor");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timedDirectedInteractions().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U &&
            publisher.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMRA directed future-arrival delivery and grant");
  auto const expectedProducer = requester.rtiAmbassador().getFederateHandle(
      options.memberFederateName);
  require(
      expectedProducer.isValid(),
      "NMRA directed requester could not resolve the publisher handle");
  requireDirectedDelivery(
      requester,
      target,
      interaction,
      expectedProducer,
      encodeTime(*messageTime),
      tag,
      1U);
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "NMRA directed publisher advanced to an unexpected time");

  requester.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  publisher.disconnect();
}

void scenarioNmraDirectedFutureTsoArrivesWhilePendingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraDirectedFutureTsoArrivesWhilePending(options, model);
}

void scenarioNmrQueuedRoAndEarliestTsoCohortContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrQueuedRoAndEarliestTsoCohort(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
