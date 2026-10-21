#include "tck_ownership_transition_query.hpp"

#include <array>
#include <cstdint>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipTransitionQuery(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "ownership-transition query requires an adapter-supplied FOM");
  Session owner(options, model, "owner");
  auto const federation = federationName(options, "ownership-transition-query");
  owner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-ownership-transition-query",
             options.federateType, federation);

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "ownership-transition query did not resolve the configured class and attribute");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(),
          "ownership-transition query registration returned an invalid handle");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, attribute),
          "registrant did not initially own the published attribute");

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, attributes);
  waitFor(owner,
          [&] { return !owner.recorder().ownershipInformationEvents().empty(); },
          options, "ownership-transition initial owner report");
  auto const before = owner.recorder().ownershipInformationEvents();
  require(before.size() == 1U && before.front().object == object &&
              before.front().attributes == attributes &&
              before.front().owner == owner.federateHandle() &&
              owner.recorder().ownershipNotOwnedEvents().empty(),
          "initial ownership query did not identify the registrant exclusively");

  std::array<std::uint8_t, 2> const tagBytes{0x54U, 0xA7U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, attributes, tag);
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, attributes);
  waitFor(owner,
          [&] { return !owner.recorder().ownershipNotOwnedEvents().empty(); },
          options, "ownership-transition unowned report");
  auto const after = owner.recorder().ownershipNotOwnedEvents();
  require(after.size() == 1U && after.front().object == object &&
              after.front().attributes == attributes &&
              owner.recorder().ownershipInformationEvents().empty() &&
              owner.recorder().ownershipOwnedByRtiEvents().empty(),
          "post-divestiture query did not report exactly the unowned attribute");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, attribute),
          "post-divestiture ownership predicate still identifies the original owner");

  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
}

void scenarioOwnershipTransitionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipTransitionQuery(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
