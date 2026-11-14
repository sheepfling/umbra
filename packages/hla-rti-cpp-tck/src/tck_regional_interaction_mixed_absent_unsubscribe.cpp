#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRegionalInteractionMixedAbsentUnsubscribeSet(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Mixed present/absent regional interaction unsubscription requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(
      options,
      "regional-interaction-mixed-absent-unsubscription");
  connectAndJoin(publisher, subscriber, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const subscriberHandles = ddmHandles(subscriber, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "mixed present/absent regional interaction unsubscription");
  require(
      publisherHandles.interactionClass == subscriberHandles.interactionClass &&
          publisherHandles.parameter == subscriberHandles.parameter,
      "mixed present/absent regional interaction members resolved different declaration handles");

  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      0UL,
      2UL,
      0UL,
      2UL);
  auto const subscriptionRegion = createDdmRegion(
      subscriber,
      subscriberHandles,
      0UL,
      2UL,
      0UL,
      2UL);
  auto const absentSubscriptionRegion = createDdmRegion(
      subscriber,
      subscriberHandles,
      5UL,
      7UL,
      5UL,
      7UL);
  auto const activeSubscriptionRegionSet =
      rti::RegionHandleSet{subscriptionRegion};
  auto const mixedSubscriptionRegionSet =
      rti::RegionHandleSet{subscriptionRegion, absentSubscriptionRegion};

  std::vector<std::uint8_t> const valueBytes{0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));

  auto send = [&](std::vector<std::uint8_t> const& tagBytes) {
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteractionWithRegions(
        publisherHandles.interactionClass,
        parameters,
        rti::RegionHandleSet{sourceRegion},
        tag);
  };
  auto pumpSubscriber = [&] {
    for (int pass = 0; pass != 8; ++pass) {
      subscriber.pump();
    }
  };
  auto verifyInteraction = [&](std::size_t index,
                                std::vector<std::uint8_t> const& expectedTag,
                                std::string const& description) {
    auto const records = subscriber.recorder().interactions();
    require(
        records.size() > index,
        description + " did not produce the expected callback");
    auto const& record = records.at(index);
    require(
        record.interaction == subscriberHandles.interactionClass &&
            record.parameters.size() == 1U &&
            record.parameters.count(subscriberHandles.parameter) == 1U &&
            copyBytes(record.parameters.at(subscriberHandles.parameter)) ==
                valueBytes,
        description + " changed the interaction class, parameter, or value");
    require(
        record.tag == expectedTag &&
            record.producer == publisher.federateHandle(),
        description + " changed ordinary interaction metadata");
  };

  subscriber.rtiAmbassador().subscribeInteractionClassWithRegions(
      subscriberHandles.interactionClass,
      activeSubscriptionRegionSet,
      true);
  auto const initialTag = std::vector<std::uint8_t>{0x49U, 0x4EU, 0x49U};
  send(initialTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interactions().size() >= 1U; },
      options,
      "regional interaction delivery before mixed unsubscription");
  require(
      subscriber.recorder().interactions().size() == 1U,
      "initial regional interaction subscription delivered a duplicate callback");
  verifyInteraction(
      0U,
      initialTag,
      "regional interaction delivery before mixed unsubscription");

  subscriber.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      subscriberHandles.interactionClass,
      mixedSubscriptionRegionSet);
  auto const mixedUnsubscribeTag =
      std::vector<std::uint8_t>{0x4DU, 0x49U, 0x58U};
  send(mixedUnsubscribeTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interactions().size() >= 2U; },
      options,
      "regional interaction delivery after mixed present/absent unsubscription");
  require(
      subscriber.recorder().interactions().size() == 2U,
      "unsubscription containing an absent region changed the subscription set");
  verifyInteraction(
      1U,
      mixedUnsubscribeTag,
      "regional interaction delivery after mixed present/absent unsubscription");

  subscriber.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      subscriberHandles.interactionClass,
      activeSubscriptionRegionSet);
  auto const removedRouteTag =
      std::vector<std::uint8_t>{0x52U, 0x45U, 0x4DU};
  send(removedRouteTag);
  pumpSubscriber();
  require(
      subscriber.recorder().interactions().size() == 2U,
      "a mixed absent-region no-op prevented later removal of the active region");

  subscriber.rtiAmbassador().deleteRegion(absentSubscriptionRegion);
  subscriber.rtiAmbassador().deleteRegion(subscriptionRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioRegionalInteractionMixedAbsentUnsubscribeSetContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalInteractionMixedAbsentUnsubscribeSet(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
