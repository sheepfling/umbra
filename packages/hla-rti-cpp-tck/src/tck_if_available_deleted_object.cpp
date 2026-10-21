#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioIfAvailableDeletedObject(Options const& options,
                                      rti::CallbackModel model) {
  require(!options.fom.empty(),
          "If Available deletion testing requires an adapter-supplied FOM");
  require(model == rti::HLA_EVOKED,
          "Queued If Available deletion requires evoked callback ordering");

  Session owner(options, model, "if-available-deletion-owner");
  Session requester(options, model, "if-available-deletion-requester");
  auto const federation = federationName(options, "if-available-deleted-object");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-if-available-deletion-owner",
             options.federateType, federation);
  requester.join(options.memberFederateName + L"-if-available-deletion-requester",
                 options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid(),
          "Adapter FOM did not resolve the If Available deletion handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "If Available deletion registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "If Available deletion object discovery");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterAttribute),
          "If Available deletion setup did not establish remote ownership");

  std::vector<std::uint8_t> const requestTagBytes{0x51U, 0xA7U};
  std::vector<std::uint8_t> const deletionTagBytes{0x44U, 0x25U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(), deletionTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  requester.recorder().clearRemovals();
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, requesterAttributes, requestTag);
  require(!requester.recorder().ownershipAcquisition().has_value() &&
              !requester.recorder().ownershipUnavailable().has_value(),
          "If Available deletion delivered a result before callback servicing");

  owner.rtiAmbassador().deleteObjectInstance(object, deletionTag);
  waitFor(requester,
          [&] { return !requester.recorder().removals().empty(); },
          options, "If Available deletion removal callback");
  static_cast<void>(requester.evokeMultipleCallbacks(0.0, 0.0));
  auto const removals = requester.recorder().removals();
  require(removals.size() == 1U && removals.front().object == object &&
              removals.front().tag == deletionTagBytes,
          "Deleted object did not yield exactly one tagged removal");
  require(!requester.recorder().ownershipAcquisition().has_value() &&
              !requester.recorder().ownershipUnavailable().has_value(),
          "Deleted object delivered a stale If Available ownership result");

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioIfAvailableDeletedObjectContract(Options const& options,
                                              rti::CallbackModel model) {
  scenarioIfAvailableDeletedObject(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
