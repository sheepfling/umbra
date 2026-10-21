#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioNmraQueuedRoAndEarliestTsoCohort(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMRA message-set case requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session publisher(options, model, "member");
  auto const federation = federationName(
      options, "nmra-queued-ro-and-earliest-tso-cohort");
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
          publisherInteraction.isValid() && publisherParameter.isValid(),
      "NMRA message-set case could not resolve the adapter interaction and parameter");
  requester.rtiAmbassador().subscribeInteractionClass(
      requesterInteraction, true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto publisherTime = makeTimeContext(publisher);
  require(
      requesterTime.factory->getName() == publisherTime.factory->getName(),
      "NMRA message-set members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *publisherTime.initial,
      "NMRA message-set initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA message-set requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA message-set requester Time Regulation callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA message-set publisher Time Regulation callback");

  std::vector<std::uint8_t> const payload{0x4EU, 0x4DU, 0x52U, 0x41U};
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
  auto const laterMessageAtEight = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 8U);
  auto const requesterTarget = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 10U);
  auto const publisherAdvance = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 12U);
  std::vector<std::uint8_t> const firstTsoTag{0x21U};
  std::vector<std::uint8_t> const secondTsoTag{0x22U};
  std::vector<std::uint8_t> const laterTsoTag{0x30U};
  rti::VariableLengthData const firstTsoTagData(
      firstTsoTag.data(), firstTsoTag.size());
  rti::VariableLengthData const secondTsoTagData(
      secondTsoTag.data(), secondTsoTag.size());
  rti::VariableLengthData const laterTsoTagData(
      laterTsoTag.data(), laterTsoTag.size());
  static_cast<void>(publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, firstTsoTagData, *messageAtFive));
  static_cast<void>(publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, secondTsoTagData, *messageAtFive));
  static_cast<void>(publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, laterTsoTagData, *laterMessageAtEight));

  require(
      requester.recorder().interactions().empty() &&
          requester.recorder().timedInteractions().empty(),
      "NMRA message-set requester observed messages before the request boundary");
  requester.rtiAmbassador().nextMessageRequestAvailable(*requesterTarget);
  waitFor(
      requester,
      [&] { return requester.recorder().interactions().size() >= 2U; },
      options,
      "NMRA message-set queued receive-order interactions");
  require(
      requester.recorder().interactions().size() == 2U &&
      requester.recorder().timedInteractions().empty() &&
          requester.recorder().timeAdvanceGrants().empty(),
      "NMRA message-set delivered timestamped input or granted before the source advanced");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitForSessions(
      {&requester, &publisher},
      [&] {
        return requester.recorder().timeAdvanceGrants().size() == 1U &&
            publisher.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMRA message-set earliest TSO cohort and grants");

  auto const receiveOrderInteractions = requester.recorder().interactions();
  auto const timedInteractions = requester.recorder().timedInteractions();
  auto const requesterGrants = requester.recorder().timeAdvanceGrants();
  auto const publisherGrants = publisher.recorder().timeAdvanceGrants();
  require(
      receiveOrderInteractions.size() == 2U && timedInteractions.size() == 2U &&
          requesterGrants.size() == 1U && publisherGrants.size() == 1U,
      "NMRA message-set did not deliver exactly the queued receive-order set, "
      "earliest TSO cohort, and one grant per advancing federate");
  std::vector<std::uint8_t> receiveOrderTags;
  for (auto const& interaction : receiveOrderInteractions) {
    require(
        interaction.interaction == requesterInteraction && interaction.tag.size() == 1U,
        "NMRA message-set receive-order callback used the wrong interaction class");
    receiveOrderTags.push_back(interaction.tag.empty() ? 0U : interaction.tag.front());
  }
  std::sort(receiveOrderTags.begin(), receiveOrderTags.end());
  require(
      receiveOrderTags == std::vector<std::uint8_t>{0x11U, 0x12U},
      "NMRA message-set did not deliver both queued receive-order interactions");

  std::vector<std::uint8_t> deliveredTsoTags;
  for (auto const& interaction : timedInteractions) {
    require(
        interaction.interaction == requesterInteraction &&
            interaction.tag.size() == 1U &&
            interaction.time == encodeTime(*messageAtFive) &&
            interaction.receivedOrder == rti::TIMESTAMP,
        "NMRA message-set delivered a non-earliest or malformed timestamped interaction");
    deliveredTsoTags.push_back(interaction.tag.empty() ? 0U : interaction.tag.front());
  }
  std::sort(deliveredTsoTags.begin(), deliveredTsoTags.end());
  require(
      deliveredTsoTags == std::vector<std::uint8_t>{0x21U, 0x22U},
      "NMRA message-set did not deliver the complete earliest equal-time TSO cohort");
  require(
      requesterGrants.front().encoded == encodeTime(*messageAtFive) &&
          publisherGrants.front().encoded == encodeTime(*publisherAdvance),
      "NMRA message-set granted at a time other than its earliest cohort boundary");
  require(
      requester.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "interaction", "grant"},
      "NMRA message-set did not deliver the earliest TSO cohort before its grant");

  auto queriedTime = requesterTime.factory->makeInitial();
  require(queriedTime != nullptr, "NMRA message-set could not allocate Query Logical Time value");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime, *messageAtFive, "NMRA message-set actual grant time");

  requester.rtiAmbassador().nextMessageRequestAvailable(*requesterTarget);
  waitFor(
      requester,
      [&] {
        return requester.recorder().timedInteractions().size() >= 3U &&
            requester.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "NMRA message-set remaining timestamp-8 TSO");
  auto const followOnTimedInteractions = requester.recorder().timedInteractions();
  auto const followOnGrants = requester.recorder().timeAdvanceGrants();
  require(
      followOnTimedInteractions.size() == 3U &&
          followOnTimedInteractions.back().tag == laterTsoTag &&
          followOnTimedInteractions.back().time == encodeTime(*laterMessageAtEight),
      "NMRA message-set re-delivered the timestamp-5 cohort instead of only the remaining timestamp-8 message");
  require(
      followOnGrants.size() == 2U &&
          followOnGrants.back().encoded == encodeTime(*laterMessageAtEight),
      "NMRA message-set did not grant at the remaining timestamp-8 message after retiring the timestamp-5 cohort");
  auto followOnQueriedTime = requesterTime.factory->makeInitial();
  require(
      followOnQueriedTime != nullptr,
      "NMRA message-set could not allocate the follow-on Query Logical Time value");
  requester.rtiAmbassador().queryLogicalTime(*followOnQueriedTime);
  requireTimeEquals(
      *followOnQueriedTime,
      *laterMessageAtEight,
      "NMRA message-set follow-on actual grant time");

  requester.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  publisher.disconnect();
}

void scenarioNmraQueuedRoAndEarliestTsoCohortContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraQueuedRoAndEarliestTsoCohort(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
