#include "tck_ownership_reacquisition_query.hpp"

#include <array>
#include <cstdint>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipReacquisitionQuery(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "ownership-reacquisition query requires an adapter-supplied FOM");
  Session owner(options, model, "owner");
  Session acquirer(options, model, "member");
  auto const federation = federationName(options, "ownership-reacquisition-query");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const acquirerClass = acquirer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const acquirerAttribute = acquirer.rtiAmbassador().getAttributeHandle(
      acquirerClass, options.attributeName);
  require(ownerClass.isValid() && acquirerClass.isValid() &&
              ownerAttribute.isValid() && acquirerAttribute.isValid() &&
              ownerClass == acquirerClass && ownerAttribute == acquirerAttribute,
          "ownership-reacquisition query did not resolve matching standard handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass, acquirerAttributes, true, L"");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "ownership-reacquisition query registration returned an invalid handle");
  waitFor(acquirer,
          [&] { return acquirer.recorder().hasDiscovery(object); },
          options, "ownership-reacquisition object discovery");
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass, acquirerAttributes);

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner,
          [&] { return !owner.recorder().ownershipInformationEvents().empty(); },
          options, "ownership-reacquisition original owner report");
  auto const original = owner.recorder().ownershipInformationEvents();
  require(original.size() == 1U && original.front().object == object &&
              original.front().attributes == ownerAttributes &&
              original.front().owner == owner.federateHandle() &&
              owner.recorder().ownershipNotOwnedEvents().empty(),
          "initial ownership query did not identify the registering federate");

  std::array<std::uint8_t, 2> const divestitureBytes{0x54U, 0xA7U};
  rti::VariableLengthData const divestitureTag(
      divestitureBytes.data(), divestitureBytes.size());
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner,
          [&] { return !owner.recorder().ownershipNotOwnedEvents().empty(); },
          options, "ownership-reacquisition unowned report");
  auto const unowned = owner.recorder().ownershipNotOwnedEvents();
  require(unowned.size() == 1U && unowned.front().object == object &&
              unowned.front().attributes == ownerAttributes &&
              owner.recorder().ownershipInformationEvents().empty(),
          "query after divestiture did not report the attribute unowned");

  std::array<std::uint8_t, 2> const acquisitionBytes{0xA7U, 0x0CU};
  rti::VariableLengthData const acquisitionTag(
      acquisitionBytes.data(), acquisitionBytes.size());
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, acquisitionTag);
  waitFor(acquirer,
          [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
          options, "ownership-reacquisition notification");
  auto const notification = acquirer.recorder().ownershipAcquisition();
  require(notification->object == object &&
              notification->attributes == acquirerAttributes &&
              !acquirer.recorder().ownershipUnavailable().has_value(),
          "second federate did not acquire exactly the divested attribute");

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner,
          [&] { return !owner.recorder().ownershipInformationEvents().empty(); },
          options, "ownership-reacquisition new owner report");
  auto const transferred = owner.recorder().ownershipInformationEvents();
  require(transferred.size() == 1U && transferred.front().object == object &&
              transferred.front().attributes == ownerAttributes &&
              transferred.front().owner == acquirer.federateHandle() &&
              owner.recorder().ownershipNotOwnedEvents().empty(),
          "final ownership query did not identify the second federate");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              acquirer.rtiAmbassador().isAttributeOwnedByFederate(
                  object, acquirerAttribute),
          "ownership predicates did not follow the query transition");

  acquirer.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  acquirer.disconnect();
  owner.disconnect();
}

void scenarioOwnershipReacquisitionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipReacquisitionQuery(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
