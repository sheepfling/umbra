#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

struct RegistrationDdmHandles final {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle explicitlyAssociatedAttribute;
  rti::AttributeHandle omittedAttribute;
  rti::DimensionHandle firstDimension;
  rti::DimensionHandle secondDimension;
};

RegistrationDdmHandles registrationDdmHandles(
    Session& session,
    Options const& options) {
  auto& ambassador = session.rtiAmbassador();
  RegistrationDdmHandles result;
  result.objectClass =
      ambassador.getObjectClassHandle(options.multiAttributeObjectClassName);
  result.explicitlyAssociatedAttribute = ambassador.getAttributeHandle(
      result.objectClass, options.multiAttributeFirstName);
  result.omittedAttribute = ambassador.getAttributeHandle(
      result.objectClass, options.multiAttributeSecondName);
  require(result.objectClass.isValid(),
          "multi-attribute registration returned an invalid object-class handle");
  require(result.explicitlyAssociatedAttribute.isValid() &&
              result.omittedAttribute.isValid() &&
              result.explicitlyAssociatedAttribute != result.omittedAttribute,
          "multi-attribute registration returned invalid or duplicate attribute handles");
  require(options.ddmDimensionNames.size() >= 2U,
          "multi-attribute registration requires two adapter-supplied DDM dimensions");
  result.firstDimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  result.secondDimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(1));
  require(result.firstDimension.isValid() && result.secondDimension.isValid() &&
              result.firstDimension != result.secondDimension,
          "multi-attribute registration returned invalid or duplicate dimension handles");
  return result;
}

rti::RegionHandle createRegistrationRegion(
    Session& session,
    RegistrationDdmHandles const& handles,
    unsigned long lower,
    unsigned long upper) {
  auto& ambassador = session.rtiAmbassador();
  auto const region = ambassador.createRegion(
      rti::DimensionHandleSet{handles.firstDimension, handles.secondDimension});
  require(region.isValid(),
          "multi-attribute registration returned an invalid region handle");
  ambassador.setRangeBounds(
      region, handles.firstDimension, rti::RangeBounds(lower, upper));
  ambassador.setRangeBounds(
      region, handles.secondDimension, rti::RangeBounds(lower, upper));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  return region;
}

} // namespace

void scenarioRegisterObjectInstanceWithRegionsOmittedAttributesDefaultRegion(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "default-region-registration-owner");
  Session subscriber(options, model, "default-region-registration-subscriber");
  auto const federation =
      federationName(options, "register-regions-omitted-attribute-default-region");
  connectAndJoin(owner, subscriber, options, federation, options.multiAttributeFom);

  auto const ownerHandles = registrationDdmHandles(owner, options);
  auto const subscriberHandles = registrationDdmHandles(subscriber, options);
  rti::AttributeHandleSet const publishedAttributes{
      ownerHandles.explicitlyAssociatedAttribute,
      ownerHandles.omittedAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass, publishedAttributes);

  auto const updateRegion = createRegistrationRegion(owner, ownerHandles, 1UL, 5UL);
  auto const disjointSubscriptionRegion =
      createRegistrationRegion(subscriber, subscriberHandles, 7UL, 9UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const suppliedUpdateRegions{{
      rti::AttributeHandleSet{ownerHandles.explicitlyAssociatedAttribute},
      rti::RegionHandleSet{updateRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const omittedAttributeSubscription{{
      rti::AttributeHandleSet{subscriberHandles.omittedAttribute},
      rti::RegionHandleSet{disjointSubscriptionRegion},
  }};
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      omittedAttributeSubscription,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass, suppliedUpdateRegions);
  require(object.isValid(),
          "region-qualified registration returned an invalid object handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "discovery through the omitted attribute's default region");

  std::vector<std::uint8_t> const value{0x73U, 0x52U, 0x31U};
  std::vector<std::uint8_t> const tag{0x44U, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      ownerHandles.omittedAttribute,
      rti::VariableLengthData(value.data(), value.size()));
  rti::VariableLengthData userTag(tag.data(), tag.size());
  owner.rtiAmbassador().updateAttributeValues(object, values, userTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "reflection through the omitted attribute's default region");

  auto const reflection = subscriber.recorder().reflection();
  require(reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(subscriberHandles.omittedAttribute) == 1U,
          "default-region reflection returned the wrong object or attribute set");
  require(copyBytes(reflection.values.at(subscriberHandles.omittedAttribute)) == value,
          "default-region reflection returned the wrong omitted-attribute value");
  require(reflection.tag == tag && reflection.producer == owner.federateHandle(),
          "default-region reflection changed its tag or producer");

  subscriber.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  owner.disconnect();
}

void scenarioRegisterObjectInstanceWithRegionsEmptyRegionSetDefaultRegion(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "empty-region-set-registration-owner");
  Session subscriber(options, model, "empty-region-set-registration-subscriber");
  auto const federation =
      federationName(options, "register-empty-region-set-default-region");
  connectAndJoin(owner, subscriber, options, federation, options.multiAttributeFom);

  auto const ownerHandles = registrationDdmHandles(owner, options);
  auto const subscriberHandles = registrationDdmHandles(subscriber, options);
  rti::AttributeHandleSet const publishedAttributes{
      ownerHandles.explicitlyAssociatedAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass, publishedAttributes);

  auto const disjointSubscriptionRegion =
      createRegistrationRegion(subscriber, subscriberHandles, 7UL, 9UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      rti::AttributeHandleSet{ownerHandles.explicitlyAssociatedAttribute},
      rti::RegionHandleSet{},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const regionalSubscription{{
      rti::AttributeHandleSet{subscriberHandles.explicitlyAssociatedAttribute},
      rti::RegionHandleSet{disjointSubscriptionRegion},
  }};
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass, regionalSubscription, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass, emptyRegionPair);
  require(object.isValid(),
          "empty-region-set registration returned an invalid object handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "discovery through an empty region set's default-region association");

  std::vector<std::uint8_t> const value{0x45U, 0x52U, 0x53U};
  std::vector<std::uint8_t> const tag{0x45U, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      ownerHandles.explicitlyAssociatedAttribute,
      rti::VariableLengthData(value.data(), value.size()));
  rti::VariableLengthData userTag(tag.data(), tag.size());
  owner.rtiAmbassador().updateAttributeValues(object, values, userTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "reflection through an empty region set's default-region association");

  auto const reflection = subscriber.recorder().reflection();
  require(reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(subscriberHandles.explicitlyAssociatedAttribute) == 1U,
          "empty-region-set reflection returned the wrong object or attribute set");
  require(copyBytes(reflection.values.at(subscriberHandles.explicitlyAssociatedAttribute)) == value,
          "empty-region-set reflection returned the wrong attribute value");
  require(reflection.tag == tag && reflection.producer == owner.federateHandle(),
          "empty-region-set reflection changed its tag or producer");

  subscriber.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  owner.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
