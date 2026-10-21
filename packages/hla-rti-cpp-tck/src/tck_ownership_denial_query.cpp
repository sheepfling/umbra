#include "tck_ownership_denial_query.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipDenialQuery(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(), "post-denial ownership-query testing requires an adapter-supplied FOM");
  Session owner(options, model, "denial-query-owner");
  Session requester(options, model, "denial-query-requester");
  auto const federation = federationName(options, "ownership-denial-query");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid() &&
          ownerClass == requesterClass && ownerAttribute == requesterAttribute,
      "post-denial ownership-query fixture did not resolve matching standard handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "post-denial ownership-query registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "post-denial ownership-query discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const acquisitionBytes{0xA5U, 0x21U};
  std::vector<std::uint8_t> const denialBytes{0xD3U, 0x42U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionBytes.data(), acquisitionBytes.size());
  rti::VariableLengthData const denialTag(denialBytes.data(), denialBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterAttributes, acquisitionTag);
  waitFor(owner,
          [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
          options, "post-denial ownership-query release request");
  auto const release = owner.recorder().ownershipReleaseRequest();
  require(release->object == object && release->attributes == ownerAttributes &&
              release->tag == acquisitionBytes,
          "post-denial ownership-query release request lost standard metadata");
  owner.rtiAmbassador().attributeOwnershipReleaseDenied(
      object, ownerAttributes, denialTag);
  waitFor(requester,
          [&] { return requester.recorder().ownershipUnavailable().has_value(); },
          options, "post-denial ownership-query unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(unavailable->object == object &&
              unavailable->attributes == requesterAttributes &&
              unavailable->tag == denialBytes &&
              !requester.recorder().ownershipAcquisition().has_value(),
          "post-denial ownership-query rejection did not preserve the denial metadata");

  requester.rtiAmbassador().queryAttributeOwnership(object, requesterAttributes);
  waitFor(requester,
          [&] { return !requester.recorder().ownershipInformationEvents().empty(); },
          options, "post-denial Inform Attribute Ownership callback");
  auto const reports = requester.recorder().ownershipInformationEvents();
  require(reports.size() == 1U && reports.front().object == object &&
              reports.front().attributes == requesterAttributes &&
              reports.front().owner == owner.federateHandle(),
          "post-denial ownership query did not report the original owner");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterAttribute),
          "post-denial ownership query changed ownership");

  requester.resign(rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipDenialQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipDenialQuery(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
