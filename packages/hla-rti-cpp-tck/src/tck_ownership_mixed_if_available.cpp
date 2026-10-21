#include "tck_ownership_mixed_if_available.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipMixedIfAvailable(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "mixed If Available ownership requires an adapter-supplied FOM");
  require(options.attributeName != options.secondaryAttributeName,
          "mixed If Available ownership requires distinct adapter attributes");
  Session owner(options, model, "owner");
  Session requester(options, model, "requester");
  auto const federation = federationName(options, "ownership-mixed-if-available");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const free = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const held = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.secondaryAttributeName);
  auto const requesterFree = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  auto const requesterHeld = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.secondaryAttributeName);
  require(ownerClass.isValid() && requesterClass == ownerClass &&
              free.isValid() && held.isValid() && free != held &&
              requesterFree == free && requesterHeld == held,
          "mixed If Available fixture did not resolve matching standard handles");
  rti::AttributeHandleSet const ownerBoth{free, held};
  rti::AttributeHandleSet const requesterBoth{requesterFree, requesterHeld};
  rti::AttributeHandleSet const freeOnly{free};
  rti::AttributeHandleSet const heldOnly{held};
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterBoth, true, L"");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerBoth);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "mixed If Available registration returned an invalid object handle");
  auto drive = [&](Session& session) {
    if (model == rti::HLA_IMMEDIATE) {
      require(session.rtiAmbassador().getObjectClassHandle(
                  options.objectClassName) == ownerClass,
              "callback servicing changed the configured object class handle");
    }
  };
  waitFor(requester,
          [&] { drive(requester); return requester.recorder().hasDiscovery(object); },
          options, "mixed If Available object discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterBoth);

  auto query = [&](rti::AttributeHandleSet const& attributes,
                   bool expectedOwned,
                   rti::FederateHandle const& expectedOwner) {
    owner.recorder().clearOwnershipRecords();
    owner.rtiAmbassador().queryAttributeOwnership(object, attributes);
    waitFor(owner,
            [&] {
              drive(owner);
              return !owner.recorder().ownershipInformationEvents().empty() ||
                  !owner.recorder().ownershipNotOwnedEvents().empty();
            },
            options, "mixed If Available attribute ownership query");
    auto const information = owner.recorder().ownershipInformationEvents();
    auto const notOwned = owner.recorder().ownershipNotOwnedEvents();
    if (expectedOwned) {
      require(information.size() == 1U && notOwned.empty() &&
                  information.front().object == object &&
                  information.front().attributes == attributes &&
                  information.front().owner == expectedOwner,
              "mixed If Available query reported the wrong federate owner");
    } else {
      require(notOwned.size() == 1U && information.empty() &&
                  notOwned.front().object == object &&
                  notOwned.front().attributes == attributes,
              "mixed If Available query did not report the unowned attribute");
    }
  };
  query(freeOnly, true, owner.federateHandle());
  query(heldOnly, true, owner.federateHandle());

  std::array<std::uint8_t, 2> const divestitureBytes{0x54U, 0x10U};
  rti::VariableLengthData const divestitureTag(
      divestitureBytes.data(), divestitureBytes.size());
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, freeOnly, divestitureTag);
  query(freeOnly, false, owner.federateHandle());
  query(heldOnly, true, owner.federateHandle());

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  std::array<std::uint8_t, 3> const requestBytes{0xA7U, 0x10U, 0x11U};
  rti::VariableLengthData const requestTag(
      requestBytes.data(), requestBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, requesterBoth, requestTag);
  if (model == rti::HLA_EVOKED) {
    require(!requester.recorder().ownershipAcquisition().has_value() &&
                requester.recorder().ownershipUnavailableEvents().empty(),
            "evoked mixed If Available callback arrived before servicing");
  }
  waitFor(requester,
          [&] {
            drive(requester);
            return requester.recorder().ownershipAcquisition().has_value() &&
                !requester.recorder().ownershipUnavailableEvents().empty();
          },
          options, "mixed If Available terminal callbacks");
  auto const acquired = requester.recorder().ownershipAcquisition();
  auto const unavailable = requester.recorder().ownershipUnavailableEvents();
  auto const expectedTag = std::vector<std::uint8_t>(
      requestBytes.begin(), requestBytes.end());
  require(acquired->object == object && acquired->attributes == freeOnly &&
              acquired->tag == expectedTag && unavailable.size() == 1U &&
              unavailable.front().object == object &&
              unavailable.front().attributes == heldOnly &&
              unavailable.front().tag == expectedTag,
          "mixed If Available callbacks did not partition the attributes");
  if (model == rti::HLA_EVOKED) {
    owner.pump();
  } else {
    drive(owner);
  }
  require(!owner.recorder().ownershipReleaseRequest().has_value(),
          "If Available requested release of the still-owned attribute");
  query(freeOnly, true, requester.federateHandle());
  query(heldOnly, true, owner.federateHandle());

  requester.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipMixedIfAvailableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipMixedIfAvailable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
