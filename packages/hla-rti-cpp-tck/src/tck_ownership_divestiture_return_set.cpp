#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioDivestitureIfWantedReturnSet(
    Options const& options, rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Divestiture return-set testing requires an adapter-supplied two-attribute FOM");

  Session owner(options, model, "if-wanted-return-owner");
  Session requester(options, model, "if-wanted-return-requester");
  auto const federation = federationName(options, "if-wanted-return-set");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-if-wanted-return-owner",
             options.federateType, federation);
  requester.join(options.memberFederateName + L"-if-wanted-return-requester",
                 options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.multiAttributeSecondName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              requesterFirst.isValid() && requesterSecond.isValid() &&
              ownerFirst != ownerSecond && requesterFirst != requesterSecond,
          "Adapter FOM did not resolve two distinct ordinary ownership attributes");

  rti::AttributeHandleSet const ownerBoth{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterBoth{requesterFirst, requesterSecond};
  rti::AttributeHandleSet const ownerFirstOnly{ownerFirst};
  rti::AttributeHandleSet const ownerSecondOnly{ownerSecond};
  rti::AttributeHandleSet const requesterFirstOnly{requesterFirst};
  rti::AttributeHandleSet const requesterSecondOnly{requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerBoth);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterBoth);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterBoth, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Divestiture return-set registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "divestiture return-set object discovery");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond),
          "Registrant did not initially own both ordinary attributes");

  rti::AttributeHandleSet divested;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object, ownerBoth, rti::VariableLengthData{}, divested);
  require(divested.empty() &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond),
          "If Wanted returned or transferred an attribute before any acquisition");

  std::vector<std::uint8_t> const requestTagBytes{0xD1U, 0xF2U};
  std::vector<std::uint8_t> const transferTagBytes{0xC7U, 0x25U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  rti::VariableLengthData const transferTag(
      transferTagBytes.data(), transferTagBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterFirstOnly, requestTag);
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterFirst),
          "First acquisition transferred ownership before the owner divested");

  divested.clear();
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object, ownerBoth, transferTag, divested);
  require(divested == ownerFirstOnly,
          "If Wanted did not return exactly the first requested attribute");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
              requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterFirst) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterSecond),
          "Partial If Wanted return set did not match immediate ownership state");
  waitFor(requester,
          [&] { return requester.recorder().ownershipAcquisition().has_value(); },
          options, "first partial divestiture acquisition notification");
  auto const firstAcquisition = requester.recorder().ownershipAcquisition();
  require(firstAcquisition->object == object &&
              firstAcquisition->attributes == requesterFirstOnly &&
              firstAcquisition->tag == transferTagBytes,
          "First partial divestiture notification had the wrong object, set, or tag");

  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterSecondOnly, requestTag);
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterSecond),
          "Earlier If Wanted call retained a deferred transfer of the unreturned attribute");
  if (model == rti::HLA_EVOKED) {
    requester.pump();
    require(!requester.recorder().ownershipAcquisition().has_value(),
            "Second attribute transferred before a new If Wanted call");
  }
  divested.clear();
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object, ownerSecondOnly, transferTag, divested);
  require(divested == ownerSecondOnly &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, ownerSecond) &&
              requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterSecond),
          "New If Wanted call did not transfer exactly the later requested attribute");
  waitFor(requester,
          [&] { return requester.recorder().ownershipAcquisition().has_value(); },
          options, "later divestiture acquisition notification");
  auto const secondAcquisition = requester.recorder().ownershipAcquisition();
  require(secondAcquisition->object == object &&
              secondAcquisition->attributes == requesterSecondOnly &&
              secondAcquisition->tag == transferTagBytes,
          "Later divestiture notification had the wrong object, set, or tag");

  owner.rtiAmbassador().deleteObjectInstance(object, rti::VariableLengthData{});
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioDivestitureIfWantedReturnSetContract(
    Options const& options, rti::CallbackModel model) {
  scenarioDivestitureIfWantedReturnSet(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
