#include "tck_ownership_competing_if_available.hpp"

#include <array>
#include <cstdint>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipCompetingIfAvailable(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "competing If Available ownership requires an adapter-supplied FOM");
  Session owner(options, model, "owner");
  Session first(options, model, "first-candidate");
  Session second(options, model, "second-candidate");
  auto const federation = federationName(options, "ownership-competing-if-available");
  connectAndJoin(owner, first, options, federation, options.fom);
  second.connect();
  second.join(options.memberFederateName + L"-second-if-available",
              options.federateType, federation);
  require(owner.federateHandle() != first.federateHandle() &&
              owner.federateHandle() != second.federateHandle() &&
              first.federateHandle() != second.federateHandle(),
          "competing If Available fixture did not create distinct federates");

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const firstClass = first.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const secondClass = second.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const firstAttribute = first.rtiAmbassador().getAttributeHandle(
      firstClass, options.attributeName);
  auto const secondAttribute = second.rtiAmbassador().getAttributeHandle(
      secondClass, options.attributeName);
  require(ownerClass.isValid() && firstClass == ownerClass &&
              secondClass == ownerClass && ownerAttribute.isValid() &&
              firstAttribute == ownerAttribute && secondAttribute == ownerAttribute,
          "competing If Available fixture did not resolve matching standard handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  first.rtiAmbassador().subscribeObjectClassAttributes(
      firstClass, firstAttributes, true, L"");
  second.rtiAmbassador().subscribeObjectClassAttributes(
      secondClass, secondAttributes, true, L"");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "competing If Available registration returned an invalid object handle");
  auto drive = [&](Session& session) {
    if (model == rti::HLA_IMMEDIATE) {
      require(session.rtiAmbassador().getObjectClassHandle(options.objectClassName) ==
                  ownerClass,
              "callback servicing changed the configured object class handle");
    }
  };
  waitFor(first,
          [&] { drive(first); return first.recorder().hasDiscovery(object); },
          options, "first competing candidate discovery");
  waitFor(second,
          [&] { drive(second); return second.recorder().hasDiscovery(object); },
          options, "second competing candidate discovery");
  first.rtiAmbassador().publishObjectClassAttributes(firstClass, firstAttributes);
  second.rtiAmbassador().publishObjectClassAttributes(secondClass, secondAttributes);

  std::array<std::uint8_t, 2> const divestitureBytes{0x54U, 0xA7U};
  rti::VariableLengthData const divestitureTag(
      divestitureBytes.data(), divestitureBytes.size());
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner,
          [&] {
            drive(owner);
            return !owner.recorder().ownershipNotOwnedEvents().empty();
          },
          options, "competing If Available initial unowned query");
  auto const unowned = owner.recorder().ownershipNotOwnedEvents();
  require(unowned.size() == 1U && unowned.front().object == object &&
              unowned.front().attributes == ownerAttributes &&
              owner.recorder().ownershipInformationEvents().empty(),
          "competing If Available fixture did not establish an unowned attribute");

  std::array<std::uint8_t, 2> const firstBytes{0xA7U, 0x0EU};
  std::array<std::uint8_t, 2> const secondBytes{0xA7U, 0x0FU};
  rti::VariableLengthData const firstTag(firstBytes.data(), firstBytes.size());
  rti::VariableLengthData const secondTag(secondBytes.data(), secondBytes.size());
  first.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, firstAttributes, firstTag);
  second.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, secondAttributes, secondTag);
  if (model == rti::HLA_EVOKED) {
    require(!first.recorder().ownershipAcquisition().has_value() &&
                !first.recorder().ownershipUnavailable().has_value() &&
                !second.recorder().ownershipAcquisition().has_value() &&
                !second.recorder().ownershipUnavailable().has_value(),
            "evoked competing acquisition delivered a callback before servicing");
  }
  auto awaitTerminal = [&](Session& candidate, rti::AttributeHandleSet const& attributes) {
    waitFor(candidate,
            [&] {
              drive(candidate);
              return candidate.recorder().ownershipAcquisition().has_value() ||
                  candidate.recorder().ownershipUnavailable().has_value();
            },
            options, "competing If Available terminal callback");
    auto const acquisition = candidate.recorder().ownershipAcquisition();
    auto const unavailable = candidate.recorder().ownershipUnavailable();
    require(acquisition.has_value() != unavailable.has_value(),
            "competing candidate did not receive exactly one terminal outcome");
    auto const& event = acquisition ? *acquisition : *unavailable;
    require(event.object == object && event.attributes == attributes,
            "competing candidate terminal callback reported the wrong object or attribute");
  };
  // Service the second candidate first; the winner is deliberately unspecified.
  awaitTerminal(second, secondAttributes);
  awaitTerminal(first, firstAttributes);
  bool const firstWon = first.recorder().ownershipAcquisition().has_value();
  bool const secondWon = second.recorder().ownershipAcquisition().has_value();
  require(firstWon != secondWon &&
              first.recorder().ownershipUnavailableEvents().size() +
                  second.recorder().ownershipUnavailableEvents().size() == 1U,
          "competing If Available requests did not resolve to one winner and one loser");
  auto const winner = firstWon ? first.federateHandle() : second.federateHandle();

  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().queryAttributeOwnership(object, ownerAttributes);
  waitFor(owner,
          [&] {
            drive(owner);
            return !owner.recorder().ownershipInformationEvents().empty();
          },
          options, "competing If Available winner query");
  auto const reports = owner.recorder().ownershipInformationEvents();
  require(reports.size() == 1U && reports.front().object == object &&
              reports.front().attributes == ownerAttributes &&
              reports.front().owner == winner &&
              owner.recorder().ownershipNotOwnedEvents().empty(),
          "ownership query did not identify the actual If Available winner");

  second.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  first.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  owner.disconnect();
}

void scenarioOwnershipCompetingIfAvailableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipCompetingIfAvailable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
