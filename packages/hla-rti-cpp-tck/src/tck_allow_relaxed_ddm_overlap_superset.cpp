#include "tck_allow_relaxed_ddm_overlap_superset.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

namespace {

void verifyStrictOverlapExecution(
    Options const& options,
    rti::CallbackModel model,
    bool allowRelaxedDdm) {
  Session publisher(
      options,
      model,
      allowRelaxedDdm ? "relaxed-superset-publisher" : "strict-superset-publisher");
  Session subscriber(
      options,
      model,
      allowRelaxedDdm ? "relaxed-superset-subscriber" : "strict-superset-subscriber");
  auto const federation = federationName(
      options,
      allowRelaxedDdm
          ? "allow-relaxed-ddm-overlap-superset-enabled"
          : "allow-relaxed-ddm-overlap-superset-disabled");

  publisher.connect();
  subscriber.connect();
  std::vector<std::wstring> modules{options.ddmFom.wstring()};
  if (allowRelaxedDdm) {
    modules.push_back(options.switchesFom.wstring());
  }
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      modules,
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  subscriber.join(options.memberFederateName, options.federateType, federation);

  require(
      publisher.rtiAmbassador().getAllowRelaxedDDMSwitch() == allowRelaxedDdm &&
          subscriber.rtiAmbassador().getAllowRelaxedDDMSwitch() == allowRelaxedDdm,
      "Allow Relaxed DDM strict-overlap control joined with the wrong FDD setting");
  auto const publisherHandles = ddmHandles(publisher, options);
  auto const subscriberHandles = ddmHandles(subscriber, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "Allow Relaxed DDM strict-overlap publisher");

  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      rti::AttributeHandleSet{publisherHandles.attribute});
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      4UL,
      1UL,
      4UL);
  auto const subscriptionRegion = createDdmRegion(
      subscriber,
      subscriberHandles,
      2UL,
      5UL,
      2UL,
      5UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const updateRegions{{
      rti::AttributeHandleSet{publisherHandles.attribute},
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const subscriptionRegions{{
      rti::AttributeHandleSet{subscriberHandles.attribute},
      rti::RegionHandleSet{subscriptionRegion},
  }};
  auto const interactionSourceRegions = rti::RegionHandleSet{sourceRegion};
  auto const interactionSubscriptionRegions =
      rti::RegionHandleSet{subscriptionRegion};
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriptionRegions,
      true,
      L"");
  subscriber.rtiAmbassador().subscribeInteractionClassWithRegions(
      subscriberHandles.interactionClass,
      interactionSubscriptionRegions,
      true);

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      updateRegions);
  require(object.isValid(), "strict-overlap object registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "Allow Relaxed DDM strict-overlap object discovery");
  require(
      subscriber.recorder().discovery().objectClass == subscriberHandles.objectClass,
      "strict-overlap discovery returned the wrong object class");

  std::vector<std::uint8_t> const updateBytes{0x53U, 0x55U, 0x50U};
  std::vector<std::uint8_t> const updateTag{0x53U, 0x55U};
  rti::AttributeHandleValueMap const values{{
      publisherHandles.attribute,
      rti::VariableLengthData(updateBytes.data(), updateBytes.size()),
  }};
  rti::VariableLengthData const attributeTag(updateTag.data(), updateTag.size());
  publisher.rtiAmbassador().updateAttributeValues(object, values, attributeTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "Allow Relaxed DDM strict-overlap object update");
  auto const reflection = subscriber.recorder().reflection();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(subscriberHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(subscriberHandles.attribute)) == updateBytes &&
          reflection.tag == updateTag,
      "strict-overlap object update changed its delivered attribute or tag");

  std::vector<std::uint8_t> const parameterBytes{0x53U, 0x55U, 0x50U, 0x49U};
  std::vector<std::uint8_t> const interactionTagBytes{0x53U, 0x55U, 0x50U, 0x54U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const interactionTag(
      interactionTagBytes.data(), interactionTagBytes.size());
  publisher.rtiAmbassador().sendInteractionWithRegions(
      publisherHandles.interactionClass,
      parameters,
      interactionSourceRegions,
      interactionTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interaction().present; },
      options,
      "Allow Relaxed DDM strict-overlap interaction");
  auto const interaction = subscriber.recorder().interaction();
  require(
      interaction.interaction == subscriberHandles.interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(subscriberHandles.parameter) == 1U &&
          copyBytes(interaction.parameters.at(subscriberHandles.parameter)) ==
              parameterBytes &&
          interaction.tag == interactionTagBytes,
      "strict-overlap interaction changed its delivered parameter or tag");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriptionRegions);
  subscriber.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      subscriberHandles.interactionClass,
      interactionSubscriptionRegions);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, updateRegions);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      rti::AttributeHandleSet{publisherHandles.attribute});
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  subscriber.rtiAmbassador().deleteRegion(subscriptionRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace

void verifyAllowRelaxedDdmStrictOverlap(
    Options const& options, rti::CallbackModel model) {
  verifyStrictOverlapExecution(options, model, false);
  verifyStrictOverlapExecution(options, model, true);
}

}  // namespace hla_rti_cpp_tck::detail
