#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioDefaultRegionInteractionUnsubscribeIsolation(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "Default-region interaction declarations require a DDM FOM");
  require(options.ddmDimensionNames.size() >= 2U,
          "Default-region interaction declarations require two dimensions");

  Session publisher(options, model, "default-region-interaction-publisher");
  Session regional(options, model, "default-region-interaction-regional");
  Session mixed(options, model, "default-region-interaction-mixed");
  auto const federation = federationName(
      options, "default-region-interaction-declaration-services");
  connectAndJoin(publisher, regional, options, federation, options.ddmFom);
  mixed.connect();
  mixed.join(
      options.memberFederateName + L"-default-region-interaction-mixed",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const regionalHandles = ddmHandles(regional, options);
  auto const mixedHandles = ddmHandles(mixed, options);
  require(
      publisherHandles.interactionClass == regionalHandles.interactionClass &&
          publisherHandles.interactionClass == mixedHandles.interactionClass &&
          publisherHandles.parameter == regionalHandles.parameter &&
          publisherHandles.parameter == mixedHandles.parameter &&
          publisherHandles.firstDimension == regionalHandles.firstDimension &&
          publisherHandles.firstDimension == mixedHandles.firstDimension &&
          publisherHandles.secondDimension == regionalHandles.secondDimension &&
          publisherHandles.secondDimension == mixedHandles.secondDimension,
      "default-region interaction members resolved inconsistent handles");

  auto const firstUpperBound = publisher.rtiAmbassador().getDimensionUpperBound(
      publisherHandles.firstDimension);
  auto const secondUpperBound = publisher.rtiAmbassador().getDimensionUpperBound(
      publisherHandles.secondDimension);
  require(firstUpperBound >= 4UL && secondUpperBound >= 4UL &&
              regional.rtiAmbassador().getDimensionUpperBound(
                  regionalHandles.firstDimension) == firstUpperBound &&
              mixed.rtiAmbassador().getDimensionUpperBound(
                  mixedHandles.firstDimension) == firstUpperBound &&
              regional.rtiAmbassador().getDimensionUpperBound(
                  regionalHandles.secondDimension) == secondUpperBound &&
              mixed.rtiAmbassador().getDimensionUpperBound(
                  mixedHandles.secondDimension) == secondUpperBound,
          "default-region interaction members lack matching dimension ranges");

  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  auto const sourceRegion = createDdmRegion(
      publisher, publisherHandles, 0UL, 1UL, 0UL, 1UL);
  auto const disjointSourceRegion = createDdmRegion(
      publisher, publisherHandles, 2UL, 3UL, 2UL, 3UL);
  auto const regionalRegion = createDdmRegion(
      regional, regionalHandles, 0UL, 1UL, 0UL, 1UL);
  auto const mixedRegion = createDdmRegion(
      mixed, mixedHandles, 2UL, 3UL, 2UL, 3UL);
  rti::RegionHandleSet const sourceRegionSet{sourceRegion};
  rti::RegionHandleSet const disjointSourceRegionSet{disjointSourceRegion};
  rti::RegionHandleSet const regionalRegionSet{regionalRegion};
  rti::RegionHandleSet const mixedRegionSet{mixedRegion};

  regional.rtiAmbassador().subscribeInteractionClassWithRegions(
      regionalHandles.interactionClass, regionalRegionSet, true);
  mixed.rtiAmbassador().subscribeInteractionClass(
      mixedHandles.interactionClass, true);
  mixed.rtiAmbassador().subscribeInteractionClassWithRegions(
      mixedHandles.interactionClass, mixedRegionSet, true);

  auto send = [&](rti::RegionHandleSet const& regions,
                  std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherHandles.parameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteractionWithRegions(
        publisherHandles.interactionClass, parameters, regions, tag);
  };
  auto hasPayload = [&](InteractionRecord const& record,
                        std::vector<std::uint8_t> const& value,
                        std::vector<std::uint8_t> const& tag) {
    auto const found = record.parameters.find(publisherHandles.parameter);
    return record.interaction == publisherHandles.interactionClass &&
        found != record.parameters.end() && copyBytes(found->second) == value &&
        record.tag == tag && record.producer == publisher.federateHandle();
  };

  std::vector<std::uint8_t> const initialValue{0x31U, 0x01U};
  std::vector<std::uint8_t> const initialTag{0x41U, 0x01U};
  send(sourceRegionSet, initialValue, initialTag);
  waitForSessions(
      {&publisher, &regional, &mixed},
      [&] {
        return regional.recorder().interactions().size() >= 1U &&
            mixed.recorder().interactions().size() >= 1U;
      },
      options,
      "ordinary/default-region and matching regional interaction subscriptions");
  auto const initialRegional = regional.recorder().interactions();
  auto const initialMixed = mixed.recorder().interactions();
  require(initialRegional.size() == 1U && initialMixed.size() == 1U &&
              hasPayload(initialRegional.front(), initialValue, initialTag) &&
              hasPayload(initialMixed.front(), initialValue, initialTag),
          "ordinary interaction subscription did not remain effective beside a disjoint regional declaration");

  mixed.rtiAmbassador().unsubscribeInteractionClass(
      mixedHandles.interactionClass);
  std::vector<std::uint8_t> const disjointValue{0x31U, 0x02U};
  std::vector<std::uint8_t> const disjointTag{0x41U, 0x02U};
  send(sourceRegionSet, disjointValue, disjointTag);
  waitFor(
      regional,
      [&] { return regional.recorder().interactions().size() >= 2U; },
      options,
      "matching regional delivery after ordinary interaction unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    mixed.pump();
  }
  auto const regionalAfterUnsubscribe = regional.recorder().interactions();
  require(regionalAfterUnsubscribe.size() == 2U &&
              hasPayload(regionalAfterUnsubscribe.back(), disjointValue, disjointTag) &&
              mixed.recorder().interactions().size() == 1U,
          "ordinary interaction unsubscribe retained its default-region route or removed the disjoint regional declaration");

  std::vector<std::uint8_t> const regionalValue{0x31U, 0x03U};
  std::vector<std::uint8_t> const regionalTag{0x41U, 0x03U};
  send(disjointSourceRegionSet, regionalValue, regionalTag);
  waitFor(
      mixed,
      [&] { return mixed.recorder().interactions().size() >= 2U; },
      options,
      "explicit regional interaction delivery after ordinary unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    regional.pump();
  }
  auto const mixedAfterUnsubscribe = mixed.recorder().interactions();
  require(mixedAfterUnsubscribe.size() == 2U &&
              hasPayload(mixedAfterUnsubscribe.back(), regionalValue, regionalTag) &&
              regional.recorder().interactions().size() == 2U,
          "ordinary interaction unsubscribe removed the independent matching regional subscription");

  mixed.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      mixedHandles.interactionClass, mixedRegionSet);
  regional.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      regionalHandles.interactionClass, regionalRegionSet);
  std::vector<std::uint8_t> const afterFinalUnsubscribeValue{0x31U, 0x04U};
  std::vector<std::uint8_t> const afterFinalUnsubscribeTag{0x41U, 0x04U};
  send(sourceRegionSet, afterFinalUnsubscribeValue, afterFinalUnsubscribeTag);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    regional.pump();
    mixed.pump();
  }
  require(
      regional.recorder().interactions().size() == 2U &&
          mixed.recorder().interactions().size() == 2U,
      "an interaction subscription remained effective after all ordinary and regional declarations were removed");

  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  publisher.rtiAmbassador().deleteRegion(disjointSourceRegion);
  regional.rtiAmbassador().deleteRegion(regionalRegion);
  mixed.rtiAmbassador().deleteRegion(mixedRegion);
  mixed.resign(rti::NO_ACTION);
  regional.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioDefaultRegionInteractionUnsubscribeIsolationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDefaultRegionInteractionUnsubscribeIsolation(options, model);
}

} // namespace hla_rti_cpp_tck::detail
