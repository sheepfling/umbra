#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRegionalInteractionPartialDefaultRange(
    Options const& options, rti::CallbackModel model) {
  auto const fom = options.scenarioFoms.find(
      "ddm-region-realization-default-range");
  require(
      fom != options.scenarioFoms.end(),
      "Partial interaction-region realization requires the restricted default-range FOM");

  Session publisher(options, model, "partial-region-interaction-publisher");
  Session overlapping(options, model, "partial-region-interaction-overlapping");
  Session disjoint(options, model, "partial-region-interaction-disjoint");
  Session boundary(options, model, "partial-region-interaction-boundary");
  auto const federation = federationName(
      options, "regional-interaction-partial-default-range");
  connectAndJoin(publisher, overlapping, options, federation, fom->second);
  disjoint.connect();
  disjoint.join(
      options.memberFederateName + L"-partial-region-disjoint",
      options.federateType,
      federation);
  boundary.connect();
  boundary.join(
      options.memberFederateName + L"-partial-region-boundary",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const overlappingHandles = ddmHandles(overlapping, options);
  auto const disjointHandles = ddmHandles(disjoint, options);
  auto const boundaryHandles = ddmHandles(boundary, options);
  verifyDdmClassDimensions(
      publisher, options, publisherHandles,
      "partial interaction-region default realization");
  require(
      publisherHandles.interactionClass == overlappingHandles.interactionClass &&
          publisherHandles.interactionClass == disjointHandles.interactionClass &&
          publisherHandles.interactionClass == boundaryHandles.interactionClass &&
          publisherHandles.parameter == overlappingHandles.parameter &&
          publisherHandles.parameter == disjointHandles.parameter &&
          publisherHandles.parameter == boundaryHandles.parameter,
      "partial interaction-region members resolved different FDD declarations");
  require(
      publisher.rtiAmbassador().getAvailableDimensionsForInteractionClass(
          publisherHandles.interactionClass) ==
          rti::DimensionHandleSet{
              publisherHandles.firstDimension,
              publisherHandles.secondDimension},
      "sent interaction class did not expose both FDD-associated dimensions");

  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);

  auto const sourceRegion = publisher.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{publisherHandles.firstDimension});
  publisher.rtiAmbassador().setRangeBounds(
      sourceRegion,
      publisherHandles.firstDimension,
      rti::RangeBounds(1UL, 4UL));
  publisher.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{sourceRegion});
  require(
      sourceRegion.isValid() &&
          publisher.rtiAmbassador().getDimensionHandleSet(sourceRegion) ==
              rti::DimensionHandleSet{publisherHandles.firstDimension},
      "non-empty source region did not remain dimensioned by X only");

  auto makeReceiverRegion = [&](Session& receiver,
                                DdmHandles const& handles,
                                bool useFomDefaultY,
                                unsigned long yLower,
                                unsigned long yUpper) {
    auto const region = receiver.rtiAmbassador().createRegion(
        rti::DimensionHandleSet{
            handles.firstDimension,
            handles.secondDimension});
    receiver.rtiAmbassador().setRangeBounds(
        region,
        handles.firstDimension,
        rti::RangeBounds(2UL, 3UL));
    if (!useFomDefaultY) {
      receiver.rtiAmbassador().setRangeBounds(
          region,
          handles.secondDimension,
          rti::RangeBounds(yLower, yUpper));
    }
    auto const pendingY = receiver.rtiAmbassador().getRangeBounds(
        region, handles.secondDimension);
    require(
        pendingY.getLowerBound() == yLower &&
            pendingY.getUpperBound() == yUpper,
        useFomDefaultY
            ? "the RTI did not supply the interaction dimension's [0,5) FOM default"
            : "the explicit interaction-region Y bounds did not round-trip");
    receiver.rtiAmbassador().commitRegionModifications(
        rti::RegionHandleSet{region});
    auto const committedY = receiver.rtiAmbassador().getRangeBounds(
        region, handles.secondDimension);
    require(
        committedY.getLowerBound() == yLower &&
            committedY.getUpperBound() == yUpper,
        "the interaction-region Y range changed at commit");
    receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
        handles.interactionClass,
        rti::RegionHandleSet{region});
    return region;
  };

  auto const overlappingRegion = makeReceiverRegion(
      overlapping,
      overlappingHandles,
      true,
      0UL,
      5UL);
  auto const disjointRegion = makeReceiverRegion(
      disjoint,
      disjointHandles,
      false,
      6UL,
      9UL);
  auto const boundaryRegion = makeReceiverRegion(
      boundary,
      boundaryHandles,
      false,
      5UL,
      7UL);

  std::vector<std::uint8_t> const payload{0x4dU, 0x21U};
  std::vector<std::uint8_t> const tag{0x70U, 0x61U, 0x72U, 0x74U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const userTag(tag.data(), tag.size());
  publisher.rtiAmbassador().sendInteractionWithRegions(
      publisherHandles.interactionClass,
      parameters,
      rti::RegionHandleSet{sourceRegion},
      userTag);

  waitFor(
      overlapping,
      [&] { return overlapping.recorder().interactions().size() >= 1U; },
      options,
      "delivery within the omitted Y dimension's FDD default range");
  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    overlapping.pump();
    disjoint.pump();
    boundary.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto const overlappingInteractions = overlapping.recorder().interactions();
  require(
      overlappingInteractions.size() == 1U &&
          overlappingInteractions.front().interaction ==
              overlappingHandles.interactionClass &&
          overlappingInteractions.front().parameters.size() == 1U &&
          overlappingInteractions.front().parameters.count(
              overlappingHandles.parameter) == 1U &&
          copyBytes(overlappingInteractions.front().parameters.at(
              overlappingHandles.parameter)) == payload &&
          overlappingInteractions.front().tag == tag &&
          overlappingInteractions.front().producer ==
              publisher.federateHandle(),
      "interaction did not reach the receiver overlapping the FDD default Y range exactly once");
  require(
      disjoint.recorder().interactions().empty(),
      "interaction escaped the omitted Y dimension's FDD default [0,5) range");
  require(
      boundary.recorder().interactions().empty(),
      "interaction reached a Y region beginning at the FDD default's exclusive upper bound");

  overlapping.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      overlappingHandles.interactionClass,
      rti::RegionHandleSet{overlappingRegion});
  disjoint.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      disjointHandles.interactionClass,
      rti::RegionHandleSet{disjointRegion});
  boundary.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      boundaryHandles.interactionClass,
      rti::RegionHandleSet{boundaryRegion});
  overlapping.rtiAmbassador().deleteRegion(overlappingRegion);
  disjoint.rtiAmbassador().deleteRegion(disjointRegion);
  boundary.rtiAmbassador().deleteRegion(boundaryRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  disjoint.resign(rti::NO_ACTION);
  boundary.resign(rti::NO_ACTION);
  overlapping.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  disjoint.disconnect();
  boundary.disconnect();
  overlapping.disconnect();
  publisher.disconnect();
}

void scenarioRegionalInteractionPartialDefaultRangeContract(
    Options const& options, rti::CallbackModel model) {
  scenarioRegionalInteractionPartialDefaultRange(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
