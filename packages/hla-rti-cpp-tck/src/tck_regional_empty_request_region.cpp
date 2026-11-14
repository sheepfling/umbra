#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void exerciseRegionalEmptyRequestRegion(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "regional-empty-request-owner");
  Session requester(options, model, "regional-empty-request-requester");
  auto const federation =
      federationName(options, "regional-empty-request-region-set");
  connectAndJoin(owner, requester, options, federation, options.ddmFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject");
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckKnownClassObject");
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, L"Name");
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, L"Name");
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "empty regional request class or attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
      "empty regional request setup did not create an owner for the default-region object");

  rti::AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      requesterAttributes,
      rti::RegionHandleSet{},
  }};
  std::vector<std::uint8_t> const emptyRegionTagBytes{0x19U, 0x36U};
  rti::VariableLengthData const emptyRegionTag(
      emptyRegionTagBytes.data(), emptyRegionTagBytes.size());
  std::vector<std::uint8_t> const ordinaryRequestTagBytes{0x51U, 0x63U};
  rti::VariableLengthData const ordinaryRequestTag(
      ordinaryRequestTagBytes.data(), ordinaryRequestTagBytes.size());

  owner.recorder().clearProvidedUpdates();
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterClass, emptyRegionPair, emptyRegionTag);
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterClass, requesterAttributes, ordinaryRequestTag);

  waitFor(
      owner,
      [&] {
        auto const requests = owner.recorder().providedUpdates();
        return std::any_of(
            requests.begin(), requests.end(),
            [&](auto const& request) {
              return request.tag == ordinaryRequestTagBytes;
            });
      },
      options,
      "ordinary class request following an empty regional request");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.01));
  }

  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.size() == 1U && requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          requests.front().tag == ordinaryRequestTagBytes,
      "empty region set issued an update request or ordinary request did not reach the default-region owner");

  rti::RegionHandle const invalidRegion;
  rti::AttributeHandleSetRegionHandleSetPairVector const invalidRegionPair{{
      requesterAttributes,
      rti::RegionHandleSet{invalidRegion},
  }};
  requireException(
      [&] {
        requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
            requesterClass, invalidRegionPair, emptyRegionTag);
      },
      L"InvalidRegion",
      "regional attribute value request with a non-region designator");

  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.resign(rti::DELETE_OBJECTS);
  requester.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
