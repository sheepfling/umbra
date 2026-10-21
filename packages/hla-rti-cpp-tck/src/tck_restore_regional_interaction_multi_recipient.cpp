#include "tck_restore_regional_interaction_multi_recipient.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

struct RecipientRegion {
  rti::InteractionClassHandle interaction;
  rti::ParameterHandle parameter;
  rti::RegionHandle region;
};

rti::RegionHandle createRegion(Session& member, rti::DimensionHandle const& x,
                               rti::DimensionHandle const& y,
                               unsigned long lower, unsigned long upper) {
  auto& ambassador = member.rtiAmbassador();
  auto const region = ambassador.createRegion(rti::DimensionHandleSet{x, y});
  ambassador.setRangeBounds(region, x, rti::RangeBounds(lower, upper));
  ambassador.setRangeBounds(region, y, rti::RangeBounds(lower, upper));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  return region;
}

RecipientRegion subscribe(Session& receiver, Options const& options,
                          unsigned long lower, unsigned long upper) {
  auto& ambassador = receiver.rtiAmbassador();
  auto const interaction = ambassador.getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = ambassador.getParameterHandle(
      interaction, options.parameterName);
  auto const x = ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const y = ambassador.getDimensionHandle(options.ddmDimensionNames.at(1));
  require(interaction.isValid() && parameter.isValid() && x.isValid() && y.isValid(),
          "Adapter DDM FOM lacks recipient interaction, parameter, or dimensions");
  require(ambassador.getDimensionUpperBound(x) >= 9UL &&
              ambassador.getDimensionUpperBound(y) >= 9UL,
          "Adapter DDM FOM recipient dimensions do not support the test ranges");
  auto const region = createRegion(receiver, x, y, lower, upper);
  ambassador.subscribeInteractionClassWithRegions(
      interaction, rti::RegionHandleSet{region});
  require(!ambassador.getConveyRegionDesignatorSetsSwitch(),
          "Default conveyed-region switch was not false");
  ambassador.setConveyRegionDesignatorSetsSwitch(true);
  return {interaction, parameter, region};
}

void checkDelivered(Session& receiver, RecipientRegion const& handles,
                    rti::RegionHandle const& sourceRegion,
                    rti::FederateHandle const& publisherHandle,
                    rti::MessageRetractionHandle const& saved,
                    rti::LogicalTime const& time7, rti::LogicalTime const& time5,
                    std::array<std::uint8_t, 5> const& payload,
                    std::array<std::uint8_t, 7> const& tag) {
  auto const deliveries = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().flushQueueGrants();
  require(deliveries.size() == 1U && grants.size() == 1U,
          "Recipient did not receive exactly one restored regional interaction and FQG");
  auto const& delivered = deliveries.front();
  require(delivered.interaction == handles.interaction &&
              delivered.parameters.size() == 1U &&
              delivered.parameters.count(handles.parameter) == 1U &&
              copyBytes(delivered.parameters.at(handles.parameter)) ==
                  std::vector<std::uint8_t>(payload.begin(), payload.end()) &&
              delivered.tag == std::vector<std::uint8_t>(tag.begin(), tag.end()) &&
              delivered.producer == publisherHandle &&
              delivered.time == copyBytes(time7.encode()) &&
              delivered.sentOrder == rti::TIMESTAMP &&
              delivered.receivedOrder == rti::TIMESTAMP &&
              delivered.regions.has_value() &&
              delivered.regions.value() == rti::RegionHandleSet{sourceRegion} &&
              delivered.retractionPresent &&
              delivered.retraction == copyBytes(saved.encode()) &&
              grants.front().time == copyBytes(time5.encode()) &&
              grants.front().optimisticTime == copyBytes(time7.encode()) &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "flush-grant"},
          "Restored regional interaction, source region, grant, or callback order changed");
}

void runRestoreRegionalInteractionMultiRecipient(Options const& options,
                                                 rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Regional interaction restore has native evidence only for evoked callbacks");
  require(!options.ddmFom.empty() && !options.logicalTimeImplementationName.empty() &&
              options.ddmDimensionNames.size() >= 2U,
          "Regional restore requires adapter DDM FOM, time implementation, and two dimensions");
  Session publisher(options, model, "owner");
  Session first(options, model, "member-a");
  Session second(options, model, "member-b");
  auto const federation = federationName(options, "restore-regional-interaction-multi");
  std::wstring const saveLabel = L"tck-regional-interaction-multi-baseline";
  std::array<std::uint8_t, 5> const payload{'M', 'U', 'L', 'T', 'I'};
  std::array<std::uint8_t, 7> const tagBytes{'R', 'E', 'S', 'T', 'O', 'R', 'E'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  first.connect();
  second.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.ddmFom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  first.join(options.memberFederateName + L"-a", options.federateType + L"-subscriber",
             federation);
  second.join(options.memberFederateName + L"-b", options.federateType + L"-subscriber",
              federation);
  auto const publisherHandle = publisher.federateHandle();
  auto& ambassador = publisher.rtiAmbassador();
  auto const interaction = ambassador.getInteractionClassHandle(options.interactionClassName);
  auto const parameter = ambassador.getParameterHandle(interaction, options.parameterName);
  auto const x = ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const y = ambassador.getDimensionHandle(options.ddmDimensionNames.at(1));
  require(interaction.isValid() && parameter.isValid() && x.isValid() && y.isValid(),
          "Adapter DDM FOM lacks publisher interaction, parameter, or dimensions");
  require(ambassador.getDimensionUpperBound(x) >= 9UL &&
              ambassador.getDimensionUpperBound(y) >= 9UL,
          "Adapter DDM FOM publisher dimensions do not support the test ranges");
  rti::ParameterHandleValueMap values;
  values.emplace(parameter, rti::VariableLengthData(payload.data(), payload.size()));
  ambassador.publishInteractionClass(interaction);
  ambassador.changeInteractionOrderType(interaction, rti::TIMESTAMP);
  auto const sourceRegion = createRegion(publisher, x, y, 1UL, 7UL);
  auto const firstHandles = subscribe(first, options, 3UL, 8UL);
  auto const secondHandles = subscribe(second, options, 5UL, 9UL);
  first.rtiAmbassador().enableTimeConstrained();
  second.rtiAmbassador().enableTimeConstrained();
  first.pump();
  second.pump();
  auto const time = makeTimeContext(publisher);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time5 = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const time7 = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const time10 = timeAfter(*time.factory, *time.initial, *time.epsilon, 10U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  ambassador.enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  auto const saved = ambassador.sendInteractionWithRegions(
      interaction, values, rti::RegionHandleSet{sourceRegion}, tag, *time7);
  require(saved.isValid() && first.recorder().timedInteractions().empty() &&
              second.recorder().timedInteractions().empty(),
          "Timestamp-seven regional interaction was not queued for both recipients");
  first.rtiAmbassador().timeAdvanceRequest(*time1);
  second.rtiAmbassador().timeAdvanceRequest(*time1);
  ambassador.requestFederationSave(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.recorder().timeAdvanceGrants().size() == 1U &&
           second.recorder().timeAdvanceGrants().size() == 1U;
  }, options, "three-member regional save initiation and time-one grants");
  require(first.recorder().timeAdvanceGrants().front().encoded == copyBytes(time1->encode()) &&
              second.recorder().timeAdvanceGrants().front().encoded == copyBytes(time1->encode()),
          "Regional recipient did not reach time one before save");
  ambassador.federateSaveBegun();
  first.rtiAmbassador().federateSaveBegun();
  second.rtiAmbassador().federateSaveBegun();
  ambassador.federateSaveComplete();
  first.rtiAmbassador().federateSaveComplete();
  second.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationSavedCount() == 1U &&
           first.federationSavedCount() == 1U && second.federationSavedCount() == 1U;
  }, options, "three-member regional federation saved");
  require(first.recorder().timedInteractions().empty() &&
              second.recorder().timedInteractions().empty(),
          "A queued regional interaction arrived while saving");

  ambassador.retract(saved);
  first.pump();
  second.pump();
  require(first.recorder().timedInteractions().empty() &&
              second.recorder().timedInteractions().empty(),
          "Post-save retract leaked a queued regional interaction");
  requireException([&] { ambassador.retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save regional handle");
  ambassador.requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           first.federateRestoreInitiations().size() == 1U &&
           second.federateRestoreInitiations().size() == 1U;
  }, options, "three-member regional restore initiation");
  ambassador.federateRestoreComplete();
  first.rtiAmbassador().federateRestoreComplete();
  second.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           first.federationRestoredCount() == 1U &&
           second.federationRestoredCount() == 1U;
  }, options, "three-member regional federation restored");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&first}, [&] {
    return first.recorder().timedInteractions().size() == 1U &&
           first.recorder().flushQueueGrants().size() == 1U;
  }, options, "first restored regional interaction and FQG");
  require(second.recorder().timedInteractions().empty() &&
              second.recorder().flushQueueGrants().empty() &&
              second.recorder().callbackOrder().empty(),
          "First regional FQR prematurely released the second recipient");
  second.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&second}, [&] {
    return second.recorder().timedInteractions().size() == 1U &&
           second.recorder().flushQueueGrants().size() == 1U;
  }, options, "second restored regional interaction and FQG");
  checkDelivered(first, firstHandles, sourceRegion, publisherHandle, saved,
                 *time7, *time5, payload, tagBytes);
  checkDelivered(second, secondHandles, sourceRegion, publisherHandle, saved,
                 *time7, *time5, payload, tagBytes);

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  ambassador.retract(saved);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().retractions().size() == 1U &&
           second.recorder().retractions().size() == 1U;
  }, options, "both restored regional request-retraction callbacks");
  for (Session* recipient : {&first, &second}) {
    auto const retraction = recipient->recorder().retractions().front();
    require(retraction.valid && retraction.encoded == copyBytes(saved.encode()) &&
                recipient->recorder().callbackOrder() ==
                    std::vector<std::string>{"request-retraction"},
            "Restored regional retraction callback differed between recipients");
  }
  requireException([&] { ambassador.retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored regional handle");

  first.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      firstHandles.interaction, rti::RegionHandleSet{firstHandles.region});
  second.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      secondHandles.interaction, rti::RegionHandleSet{secondHandles.region});
  first.rtiAmbassador().deleteRegion(firstHandles.region);
  second.rtiAmbassador().deleteRegion(secondHandles.region);
  ambassador.deleteRegion(sourceRegion);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreRegionalInteractionMultiRecipient(Options const& options,
                                                      rti::CallbackModel model) {
  runRestoreRegionalInteractionMultiRecipient(options, model);
}

void scenarioRestoreRegionalInteractionMultiRecipientContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreRegionalInteractionMultiRecipient(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
