#include "tck_default_region_class_request_equivalence.hpp"

#include <map>
#include <set>
#include <vector>

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void run(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "default-region class request equivalence has native evidence only for evoked callbacks");
  require(!options.ddmFom.empty() && options.ddmDimensionNames.size() == 2U,
          "default-region class request equivalence requires an adapter dimensional FOM");

  Session owner(options, model, "owner");
  Session ordinary(options, model, "ordinary-requester");
  Session regional(options, model, "regional-requester");
  auto const federation = federationName(options, "default-region-class-request-equivalence");
  owner.connect();
  ordinary.connect();
  regional.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.ddmFom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  ordinary.join(options.memberFederateName + L"-ordinary", options.federateType, federation);
  regional.join(options.memberFederateName + L"-regional", options.federateType, federation);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const ordinaryHandles = ddmHandles(ordinary, options);
  auto const regionalHandles = ddmHandles(regional, options);
  verifyDdmClassDimensions(owner, options, ownerHandles,
                           "default-region class request equivalence");
  require(ownerHandles.objectClass == ordinaryHandles.objectClass &&
              ownerHandles.objectClass == regionalHandles.objectClass &&
              ownerHandles.attribute == ordinaryHandles.attribute &&
              ownerHandles.attribute == regionalHandles.attribute,
          "adapter FOM yielded inconsistent class or attribute handles");

  rti::AttributeHandleSet const requested{ownerHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerHandles.objectClass, requested);
  auto const sourceRegion = createDdmRegion(owner, ownerHandles, 0UL, 1UL, 0UL, 1UL);
  auto const broadRegion = createDdmRegion(regional, regionalHandles, 0UL, 3UL, 0UL, 3UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      requested, rti::RegionHandleSet{sourceRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const broadPair{{
      requested, rti::RegionHandleSet{broadRegion}}};
  auto const regionalObject = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass, sourcePair);
  auto const defaultObject = owner.rtiAmbassador().registerObjectInstance(ownerHandles.objectClass);
  require(regionalObject.isValid() && defaultObject.isValid() &&
              regionalObject != defaultObject,
          "adapter FOM could not register distinct regional and default-region objects");

  std::vector<std::uint8_t> const ordinaryTagBytes{0xA1U, 0x01U};
  std::vector<std::uint8_t> const regionalTagBytes{0xB2U, 0x02U};
  rti::VariableLengthData const ordinaryTag(ordinaryTagBytes.data(), ordinaryTagBytes.size());
  rti::VariableLengthData const regionalTag(regionalTagBytes.data(), regionalTagBytes.size());
  owner.recorder().clearProvidedUpdates();
  ordinary.rtiAmbassador().requestAttributeValueUpdate(
      ordinaryHandles.objectClass, requested, ordinaryTag);
  regional.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      regionalHandles.objectClass, broadPair, regionalTag);
  waitFor(owner,
          [&] { return owner.recorder().providedUpdates().size() >= 4U; },
          options, "default-region class request callbacks");

  std::map<std::vector<std::uint8_t>, std::set<rti::ObjectInstanceHandle>> byTag;
  auto const callbacks = owner.recorder().providedUpdates();
  require(callbacks.size() == 4U,
          "class request equivalence returned duplicate or missing callbacks");
  for (auto const& callback : callbacks) {
    require(callback.attributes == requested,
            "class request equivalence changed requested attributes");
    byTag[callback.tag].insert(callback.object);
  }
  std::set<rti::ObjectInstanceHandle> const expected{regionalObject, defaultObject};
  require(byTag.size() == 2U &&
              byTag[ordinaryTagBytes] == expected &&
              byTag[regionalTagBytes] == expected,
          "ordinary default-region and broad explicit-region requests selected different objects or tags");

  owner.rtiAmbassador().unassociateRegionsForUpdates(regionalObject, sourcePair);
  regional.rtiAmbassador().deleteRegion(broadRegion);
  owner.rtiAmbassador().deleteRegion(sourceRegion);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerHandles.objectClass, requested);
  regional.resign(rti::NO_ACTION);
  ordinary.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  regional.disconnect();
  ordinary.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioDefaultRegionClassRequestEquivalence(Options const& options,
                                                  rti::CallbackModel model) {
  run(options, model);
}

void scenarioDefaultRegionClassRequestEquivalenceContract(Options const& options,
                                                          rti::CallbackModel model) {
  run(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
