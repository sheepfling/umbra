#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

struct AttributeDimensionHandles final {
  rti::ObjectClassHandle baseClass;
  rti::ObjectClassHandle derivedClass;
  rti::AttributeHandle baseValue;
  rti::AttributeHandle inheritedBaseValue;
  rti::AttributeHandle derivedValue;
  rti::DimensionHandle x;
  rti::DimensionHandle y;
};

AttributeDimensionHandles resolveAttributeDimensionHandles(Session& session) {
  auto& ambassador = session.rtiAmbassador();
  AttributeDimensionHandles handles;
  handles.baseClass = ambassador.getObjectClassHandle(
      L"HLAobjectRoot.TckAttributeDimensionBaseObject");
  handles.derivedClass = ambassador.getObjectClassHandle(
      L"HLAobjectRoot.TckAttributeDimensionBaseObject.TckAttributeDimensionDerivedObject");
  handles.baseValue = ambassador.getAttributeHandle(
      handles.baseClass, L"BaseValue");
  handles.inheritedBaseValue = ambassador.getAttributeHandle(
      handles.derivedClass, L"BaseValue");
  handles.derivedValue = ambassador.getAttributeHandle(
      handles.derivedClass, L"DerivedValue");
  handles.x = ambassador.getDimensionHandle(L"TckDimensionX");
  handles.y = ambassador.getDimensionHandle(L"TckDimensionY");
  require(
      handles.baseClass.isValid() && handles.derivedClass.isValid() &&
          handles.baseValue.isValid() && handles.inheritedBaseValue.isValid() &&
          handles.derivedValue.isValid() && handles.x.isValid() &&
          handles.y.isValid(),
      "attribute-dimension FOM lookup returned an invalid handle");
  require(
      handles.baseValue == handles.inheritedBaseValue,
      "base-class attribute lookup changed when resolved through the derived object class");

  auto const baseDimensions =
      ambassador.getAvailableDimensionsForObjectClass(handles.baseClass);
  auto const derivedDimensions =
      ambassador.getAvailableDimensionsForObjectClass(handles.derivedClass);
  require(
      baseDimensions == rti::DimensionHandleSet{handles.x} &&
          derivedDimensions == rti::DimensionHandleSet{handles.x, handles.y},
      "base/derived FDD object classes did not expose their declared dimension hierarchy");
  return handles;
}

rti::RegionHandle createAttributeDimensionRegion(
    Session& session,
    rti::DimensionHandle const& dimension,
    unsigned long lower,
    unsigned long upper,
    std::string const& description) {
  auto& ambassador = session.rtiAmbassador();
  auto const region = ambassador.createRegion(rti::DimensionHandleSet{dimension});
  require(region.isValid(), description + " region creation failed");
  ambassador.setRangeBounds(region, dimension, rti::RangeBounds(lower, upper));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  require(
      ambassador.getDimensionHandleSet(region) == rti::DimensionHandleSet{dimension},
      description + " region did not retain its single FDD-associated dimension");
  return region;
}

rti::RegionHandle createAttributeDimensionRegionXY(
    Session& session,
    rti::DimensionHandle const& x,
    rti::DimensionHandle const& y,
    unsigned long xLower,
    unsigned long xUpper,
    unsigned long yLower,
    unsigned long yUpper,
    std::string const& description) {
  auto& ambassador = session.rtiAmbassador();
  auto const region = ambassador.createRegion(rti::DimensionHandleSet{x, y});
  require(region.isValid(), description + " region creation failed");
  ambassador.setRangeBounds(region, x, rti::RangeBounds(xLower, xUpper));
  ambassador.setRangeBounds(region, y, rti::RangeBounds(yLower, yUpper));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  require(
      ambassador.getDimensionHandleSet(region) == rti::DimensionHandleSet{x, y},
      description + " region did not retain its X/Y dimensions");
  return region;
}

rti::AttributeHandleSetRegionHandleSetPairVector attributeRegionPairs(
    AttributeDimensionHandles const& handles,
    rti::RegionHandle const& baseRegion,
    rti::RegionHandle const& derivedRegion) {
  return {
      {rti::AttributeHandleSet{handles.inheritedBaseValue},
       rti::RegionHandleSet{baseRegion}},
      {rti::AttributeHandleSet{handles.derivedValue},
       rti::RegionHandleSet{derivedRegion}},
  };
}

void requireOnlyAttributeReflection(
    Session& receiver,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& expectedAttribute,
    rti::AttributeHandle const& otherAttribute,
    std::vector<std::uint8_t> const& expectedValue,
    std::vector<std::uint8_t> const& expectedTag,
    rti::FederateHandle const& expectedProducer,
    std::string const& description) {
  auto const reflections = receiver.recorder().reflections();
  require(
      reflections.size() == 1U,
      description + " did not receive exactly one attribute reflection");
  auto const& reflection = reflections.front();
  auto const expected = reflection.values.find(expectedAttribute);
  require(
      reflection.present && reflection.object == object &&
          reflection.values.size() == 1U && expected != reflection.values.end() &&
          reflection.values.count(otherAttribute) == 0U &&
          copyBytes(expected->second) == expectedValue &&
          reflection.tag == expectedTag && reflection.producer == expectedProducer,
      description + " received the wrong object, attribute, value, tag, or producer");
}

}  // namespace

void scenarioRegionalObjectAttributeDimensionInheritance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Object-attribute dimension inheritance requires the multi-attribute DDM FOM");

  Session publisher(options, model, "attribute-dimension-publisher");
  Session xReceiver(options, model, "attribute-dimension-x-receiver");
  Session yReceiver(options, model, "attribute-dimension-y-receiver");
  Session disjointReceiver(options, model, "attribute-dimension-disjoint-receiver");
  auto const federation =
      federationName(options, "regional-object-attribute-dimension-inheritance");
  connectAndJoin(
      publisher, xReceiver, options, federation, options.multiAttributeFom);
  yReceiver.connect();
  yReceiver.join(
      options.memberFederateName + L"-attribute-dimension-y",
      options.federateType,
      federation);
  disjointReceiver.connect();
  disjointReceiver.join(
      options.memberFederateName + L"-attribute-dimension-disjoint",
      options.federateType,
      federation);

  auto const publisherHandles = resolveAttributeDimensionHandles(publisher);
  auto const xHandles = resolveAttributeDimensionHandles(xReceiver);
  auto const yHandles = resolveAttributeDimensionHandles(yReceiver);
  auto const disjointHandles = resolveAttributeDimensionHandles(disjointReceiver);
  require(
      publisherHandles.baseClass == xHandles.baseClass &&
          publisherHandles.baseClass == yHandles.baseClass &&
          publisherHandles.baseClass == disjointHandles.baseClass &&
          publisherHandles.derivedClass == xHandles.derivedClass &&
          publisherHandles.derivedClass == yHandles.derivedClass &&
          publisherHandles.derivedClass == disjointHandles.derivedClass &&
          publisherHandles.inheritedBaseValue == xHandles.inheritedBaseValue &&
          publisherHandles.inheritedBaseValue == yHandles.inheritedBaseValue &&
          publisherHandles.inheritedBaseValue == disjointHandles.inheritedBaseValue &&
          publisherHandles.derivedValue == xHandles.derivedValue &&
          publisherHandles.derivedValue == yHandles.derivedValue &&
          publisherHandles.derivedValue == disjointHandles.derivedValue &&
          publisherHandles.x == xHandles.x && publisherHandles.x == yHandles.x &&
          publisherHandles.x == disjointHandles.x &&
          publisherHandles.y == xHandles.y && publisherHandles.y == yHandles.y &&
          publisherHandles.y == disjointHandles.y,
      "members resolved inconsistent attribute-dimension FDD handles");

  rti::AttributeHandleSet const publishedAttributes{
      publisherHandles.inheritedBaseValue, publisherHandles.derivedValue};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.derivedClass, publishedAttributes);

  auto const sourceX = createAttributeDimensionRegion(
      publisher, publisherHandles.x, 10UL, 20UL, "base-attribute source X");
  auto const sourceXY = createAttributeDimensionRegionXY(
      publisher,
      publisherHandles.x,
      publisherHandles.y,
      10UL,
      20UL,
      10UL,
      20UL,
      "derived-attribute source X/Y");
  auto const invalidBaseYRegion = createAttributeDimensionRegion(
      publisher,
      publisherHandles.y,
      10UL,
      20UL,
      "invalid Y-only region for the inherited base attribute");
  auto const invalidBaseRegistration =
      rti::AttributeHandleSetRegionHandleSetPairVector{{
          rti::AttributeHandleSet{publisherHandles.inheritedBaseValue},
          rti::RegionHandleSet{invalidBaseYRegion},
      }};
  requireException(
      [&] {
        static_cast<void>(publisher.rtiAmbassador().registerObjectInstanceWithRegions(
            publisherHandles.derivedClass, invalidBaseRegistration));
      },
      L"InvalidRegionContext",
      "derived-class registration associating the inherited base attribute with Y");
  auto const xReceiverBaseRegion = createAttributeDimensionRegion(
      xReceiver, xHandles.x, 12UL, 18UL, "X receiver overlapping BaseValue");
  auto const xReceiverDerivedRegion = createAttributeDimensionRegionXY(
      xReceiver,
      xHandles.x,
      xHandles.y,
      12UL,
      18UL,
      50UL,
      60UL,
      "X receiver disjoint from DerivedValue");
  auto const yReceiverBaseRegion = createAttributeDimensionRegion(
      yReceiver, yHandles.x, 50UL, 60UL, "Y receiver disjoint from BaseValue");
  auto const yReceiverDerivedRegion = createAttributeDimensionRegionXY(
      yReceiver,
      yHandles.x,
      yHandles.y,
      12UL,
      18UL,
      12UL,
      18UL,
      "Y receiver overlapping DerivedValue");
  auto const disjointBaseRegion = createAttributeDimensionRegion(
      disjointReceiver, disjointHandles.x, 50UL, 60UL,
      "disjoint receiver BaseValue region");
  auto const disjointDerivedRegion = createAttributeDimensionRegionXY(
      disjointReceiver,
      disjointHandles.x,
      disjointHandles.y,
      50UL,
      60UL,
      50UL,
      60UL,
      "disjoint receiver DerivedValue region");

  auto const xReceiverPairs = attributeRegionPairs(
      xHandles, xReceiverBaseRegion, xReceiverDerivedRegion);
  auto const yReceiverPairs = attributeRegionPairs(
      yHandles, yReceiverBaseRegion, yReceiverDerivedRegion);
  auto const disjointReceiverPairs = attributeRegionPairs(
      disjointHandles, disjointBaseRegion, disjointDerivedRegion);
  auto const invalidBaseSubscriptionYRegion = createAttributeDimensionRegion(
      xReceiver,
      xHandles.y,
      10UL,
      20UL,
      "invalid Y-only subscription region for inherited BaseValue");
  rti::AttributeHandleSetRegionHandleSetPairVector const invalidBaseSubscription{{
      rti::AttributeHandleSet{xHandles.inheritedBaseValue},
      rti::RegionHandleSet{invalidBaseSubscriptionYRegion},
  }};
  requireException(
      [&] {
        xReceiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
            xHandles.derivedClass, invalidBaseSubscription);
      },
      L"InvalidRegionContext",
      "regional subscription associating inherited BaseValue with Y");
  std::vector<std::uint8_t> const invalidRequestTagBytes{0x72U, 0x65U, 0x67U};
  rti::VariableLengthData const invalidRequestTag(
      invalidRequestTagBytes.data(), invalidRequestTagBytes.size());
  requireException(
      [&] {
        xReceiver.rtiAmbassador().requestAttributeValueUpdateWithRegions(
            xHandles.derivedClass,
            invalidBaseSubscription,
            invalidRequestTag);
      },
      L"InvalidRegionContext",
      "regional value request associating inherited BaseValue with Y");
  xReceiver.rtiAmbassador().deleteRegion(invalidBaseSubscriptionYRegion);
  xReceiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      xHandles.derivedClass, xReceiverPairs);
  yReceiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      yHandles.derivedClass, yReceiverPairs);
  disjointReceiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      disjointHandles.derivedClass, disjointReceiverPairs);

  auto const sourcePairs = attributeRegionPairs(
      publisherHandles, sourceX, sourceXY);
  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.derivedClass, sourcePairs);
  require(object.isValid(),
          "attribute-specific region registration returned an invalid object handle");

  rti::AttributeHandleSetRegionHandleSetPairVector const inheritedOnY{{
      rti::AttributeHandleSet{publisherHandles.inheritedBaseValue},
      rti::RegionHandleSet{invalidBaseYRegion},
  }};
  requireException(
      [&] {
        publisher.rtiAmbassador().associateRegionsForUpdates(object, inheritedOnY);
      },
      L"InvalidRegionContext",
      "instance association changed inherited BaseValue dimensions to Y");

  waitForSessions(
      {&xReceiver, &yReceiver},
      [&] {
        return xReceiver.recorder().hasDiscovery(object) &&
            yReceiver.recorder().hasDiscovery(object);
      },
      options,
      "attribute-dimension object discovery on matching regional subscriptions");

  std::vector<std::uint8_t> const baseValue{0x31U, 0x32U};
  std::vector<std::uint8_t> const derivedValue{0x41U, 0x42U};
  std::vector<std::uint8_t> const tag{0x64U, 0x64U, 0x6dU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.inheritedBaseValue,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  values.emplace(
      publisherHandles.derivedValue,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  publisher.rtiAmbassador().updateAttributeValues(
      object, values, rti::VariableLengthData(tag.data(), tag.size()));

  waitForSessions(
      {&xReceiver, &yReceiver},
      [&] {
        return xReceiver.recorder().reflectionCount() >= 1U &&
            yReceiver.recorder().reflectionCount() >= 1U;
      },
      options,
      "attribute-specific X/Y regional reflection delivery");
  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    xReceiver.pump();
    yReceiver.pump();
    disjointReceiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  requireOnlyAttributeReflection(
      xReceiver,
      object,
      xHandles.inheritedBaseValue,
      xHandles.derivedValue,
      baseValue,
      tag,
      publisher.federateHandle(),
      "receiver overlapping only the inherited BaseValue X region");
  requireOnlyAttributeReflection(
      yReceiver,
      object,
      yHandles.derivedValue,
      yHandles.inheritedBaseValue,
      derivedValue,
      tag,
      publisher.federateHandle(),
      "receiver overlapping only the derived DerivedValue Y region");
  require(
      disjointReceiver.recorder().reflections().empty(),
      "attribute updates escaped both disjoint per-attribute regions");

  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePairs);
  xReceiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      xHandles.derivedClass, xReceiverPairs);
  yReceiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      yHandles.derivedClass, yReceiverPairs);
  disjointReceiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      disjointHandles.derivedClass, disjointReceiverPairs);
  publisher.rtiAmbassador().deleteRegion(sourceX);
  publisher.rtiAmbassador().deleteRegion(sourceXY);
  publisher.rtiAmbassador().deleteRegion(invalidBaseYRegion);
  xReceiver.rtiAmbassador().deleteRegion(xReceiverBaseRegion);
  xReceiver.rtiAmbassador().deleteRegion(xReceiverDerivedRegion);
  yReceiver.rtiAmbassador().deleteRegion(yReceiverBaseRegion);
  yReceiver.rtiAmbassador().deleteRegion(yReceiverDerivedRegion);
  disjointReceiver.rtiAmbassador().deleteRegion(disjointBaseRegion);
  disjointReceiver.rtiAmbassador().deleteRegion(disjointDerivedRegion);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.derivedClass, publishedAttributes);
  disjointReceiver.resign(rti::NO_ACTION);
  yReceiver.resign(rti::NO_ACTION);
  xReceiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  disjointReceiver.disconnect();
  yReceiver.disconnect();
  xReceiver.disconnect();
  publisher.disconnect();
}

void scenarioRegionalObjectAttributeDimensionInheritanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalObjectAttributeDimensionInheritance(options, model);
}

void scenarioRegionalObjectAttributeDefaultRegionFddDimensions(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Object-attribute default-region FDD test requires the multi-attribute DDM FOM");

  Session publisher(options, model, "attribute-default-region-publisher");
  Session baseReceiver(options, model, "attribute-default-region-base-receiver");
  Session derivedReceiver(options, model, "attribute-default-region-derived-receiver");
  auto const federation =
      federationName(options, "regional-object-attribute-default-region-fdd-dimensions");
  connectAndJoin(
      publisher, baseReceiver, options, federation, options.multiAttributeFom);
  derivedReceiver.connect();
  derivedReceiver.join(
      options.memberFederateName + L"-attribute-default-region-derived",
      options.federateType,
      federation);

  auto const publisherHandles = resolveAttributeDimensionHandles(publisher);
  auto const baseHandles = resolveAttributeDimensionHandles(baseReceiver);
  auto const derivedHandles = resolveAttributeDimensionHandles(derivedReceiver);
  require(
      publisherHandles.derivedClass == baseHandles.derivedClass &&
          publisherHandles.derivedClass == derivedHandles.derivedClass &&
          publisherHandles.inheritedBaseValue == baseHandles.inheritedBaseValue &&
          publisherHandles.inheritedBaseValue == derivedHandles.inheritedBaseValue &&
          publisherHandles.derivedValue == baseHandles.derivedValue &&
          publisherHandles.derivedValue == derivedHandles.derivedValue &&
          publisherHandles.x == baseHandles.x &&
          publisherHandles.x == derivedHandles.x &&
          publisherHandles.y == baseHandles.y &&
          publisherHandles.y == derivedHandles.y,
      "members resolved inconsistent FDD dimensions for default-region delivery");

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& baseAmbassador = baseReceiver.rtiAmbassador();
  auto& derivedAmbassador = derivedReceiver.rtiAmbassador();
  auto const xUpperBound = publisherAmbassador.getDimensionUpperBound(publisherHandles.x);
  auto const yUpperBound = publisherAmbassador.getDimensionUpperBound(publisherHandles.y);
  require(
      xUpperBound == 100UL && yUpperBound == 100UL,
      "multi-attribute FDD dimensions did not retain their declared upper bounds");
  auto const xHighLower = xUpperBound - 1UL;
  auto const yHighLower = yUpperBound - 1UL;

  rti::AttributeHandleSet const publishedAttributes{
      publisherHandles.inheritedBaseValue, publisherHandles.derivedValue};
  publisherAmbassador.publishObjectClassAttributes(
      publisherHandles.derivedClass, publishedAttributes);

  auto const baseHighRegion = createAttributeDimensionRegion(
      baseReceiver,
      baseHandles.x,
      xHighLower,
      xUpperBound,
      "near-upper-bound inherited BaseValue subscription");
  auto const derivedHighYRegion = createAttributeDimensionRegion(
      derivedReceiver,
      derivedHandles.y,
      yHighLower,
      yUpperBound,
      "near-upper-bound derived DerivedValue Y subscription");
  auto const invalidBaseYRegion = createAttributeDimensionRegion(
      baseReceiver,
      baseHandles.y,
      yHighLower,
      yUpperBound,
      "unavailable Y-only inherited BaseValue subscription");
  rti::AttributeHandleSetRegionHandleSetPairVector const invalidBaseSubscription{{
      rti::AttributeHandleSet{baseHandles.inheritedBaseValue},
      rti::RegionHandleSet{invalidBaseYRegion},
  }};
  requireException(
      [&] {
        baseAmbassador.subscribeObjectClassAttributesWithRegions(
            baseHandles.derivedClass, invalidBaseSubscription);
      },
      L"InvalidRegionContext",
      "FDD-unavailable Y dimension for inherited BaseValue subscription");
  baseAmbassador.deleteRegion(invalidBaseYRegion);

  rti::AttributeHandleSetRegionHandleSetPairVector const baseSubscription{{
      rti::AttributeHandleSet{baseHandles.inheritedBaseValue},
      rti::RegionHandleSet{baseHighRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const derivedSubscription{{
      rti::AttributeHandleSet{derivedHandles.derivedValue},
      rti::RegionHandleSet{derivedHighYRegion},
  }};
  baseAmbassador.subscribeObjectClassAttributesWithRegions(
      baseHandles.derivedClass, baseSubscription);
  derivedAmbassador.subscribeObjectClassAttributesWithRegions(
      derivedHandles.derivedClass, derivedSubscription);

  auto const object = publisherAmbassador.registerObjectInstance(
      publisherHandles.derivedClass);
  require(
      object.isValid(),
      "default-region object registration returned an invalid object handle");
  waitForSessions(
      {&baseReceiver, &derivedReceiver},
      [&] {
        return baseReceiver.recorder().hasDiscovery(object) &&
            derivedReceiver.recorder().hasDiscovery(object);
      },
      options,
      "default-region object discovery through FDD-associated attributes");

  std::vector<std::uint8_t> const baseValue{0x51U, 0x52U};
  std::vector<std::uint8_t> const derivedValue{0x61U, 0x62U};
  std::vector<std::uint8_t> const tag{0x66U, 0x64U, 0x64U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.inheritedBaseValue,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  values.emplace(
      publisherHandles.derivedValue,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  publisherAmbassador.updateAttributeValues(
      object, values, rti::VariableLengthData(tag.data(), tag.size()));

  waitForSessions(
      {&baseReceiver, &derivedReceiver},
      [&] {
        return baseReceiver.recorder().reflectionCount() >= 1U &&
            derivedReceiver.recorder().reflectionCount() >= 1U;
      },
      options,
      "implicit default-region delivery at FDD upper-bound ranges");
  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    baseReceiver.pump();
    derivedReceiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  requireOnlyAttributeReflection(
      baseReceiver,
      object,
      baseHandles.inheritedBaseValue,
      baseHandles.derivedValue,
      baseValue,
      tag,
      publisher.federateHandle(),
      "X-only receiver near the FDD upper bound for inherited BaseValue");
  requireOnlyAttributeReflection(
      derivedReceiver,
      object,
      derivedHandles.derivedValue,
      derivedHandles.inheritedBaseValue,
      derivedValue,
      tag,
      publisher.federateHandle(),
      "Y-only receiver near the FDD upper bound for derived DerivedValue");

  baseAmbassador.unsubscribeObjectClassAttributesWithRegions(
      baseHandles.derivedClass, baseSubscription);
  derivedAmbassador.unsubscribeObjectClassAttributesWithRegions(
      derivedHandles.derivedClass, derivedSubscription);
  baseAmbassador.deleteRegion(baseHighRegion);
  derivedAmbassador.deleteRegion(derivedHighYRegion);
  publisherAmbassador.unpublishObjectClassAttributes(
      publisherHandles.derivedClass, publishedAttributes);
  derivedReceiver.resign(rti::NO_ACTION);
  baseReceiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
  derivedReceiver.disconnect();
  baseReceiver.disconnect();
  publisher.disconnect();
}

void scenarioRegionalObjectAttributeDefaultRegionFddDimensionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalObjectAttributeDefaultRegionFddDimensions(options, model);
}

void scenarioRegionalObjectAttributeUnspecifiedDimensionDefaultRange(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "FDD unspecified-dimension test requires the multi-attribute DDM FOM");

  Session publisher(options, model, "attribute-unspecified-range-publisher");
  Session baseReceiver(options, model, "attribute-unspecified-range-base-receiver");
  Session yReceiver(options, model, "attribute-unspecified-range-y-receiver");
  auto const federation =
      federationName(options, "regional-object-attribute-unspecified-dimension-default-range");
  connectAndJoin(
      publisher, baseReceiver, options, federation, options.multiAttributeFom);
  yReceiver.connect();
  yReceiver.join(
      options.memberFederateName + L"-attribute-unspecified-range-y",
      options.federateType,
      federation);

  auto const publisherHandles = resolveAttributeDimensionHandles(publisher);
  auto const baseHandles = resolveAttributeDimensionHandles(baseReceiver);
  auto const yHandles = resolveAttributeDimensionHandles(yReceiver);
  require(
      publisherHandles.derivedClass == baseHandles.derivedClass &&
          publisherHandles.derivedClass == yHandles.derivedClass &&
          publisherHandles.inheritedBaseValue == baseHandles.inheritedBaseValue &&
          publisherHandles.inheritedBaseValue == yHandles.inheritedBaseValue &&
          publisherHandles.derivedValue == baseHandles.derivedValue &&
          publisherHandles.derivedValue == yHandles.derivedValue &&
          publisherHandles.x == baseHandles.x && publisherHandles.x == yHandles.x &&
          publisherHandles.y == baseHandles.y && publisherHandles.y == yHandles.y,
      "members resolved inconsistent FDD handles for unspecified-dimension routing");

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& baseAmbassador = baseReceiver.rtiAmbassador();
  auto& yAmbassador = yReceiver.rtiAmbassador();
  auto const xUpperBound = publisherAmbassador.getDimensionUpperBound(publisherHandles.x);
  auto const yUpperBound = publisherAmbassador.getDimensionUpperBound(publisherHandles.y);
  require(
      xUpperBound == 100UL && yUpperBound == 100UL,
      "FDD dimension default-range fixture must use [0, upperBound) for X and Y");

  rti::AttributeHandleSet const publishedAttributes{
      publisherHandles.inheritedBaseValue, publisherHandles.derivedValue};
  publisherAmbassador.publishObjectClassAttributes(
      publisherHandles.derivedClass, publishedAttributes);

  auto const baseSourceX = createAttributeDimensionRegion(
      publisher,
      publisherHandles.x,
      10UL,
      20UL,
      "inherited BaseValue X-only source specification");
  auto const derivedSourceX = createAttributeDimensionRegion(
      publisher,
      publisherHandles.x,
      10UL,
      20UL,
      "derived DerivedValue X-only source specification");
  require(
      publisherAmbassador.getDimensionHandleSet(derivedSourceX) ==
          rti::DimensionHandleSet{publisherHandles.x},
      "derived source region specification explicitly included Y instead of leaving it unspecified");

  auto const baseReceiverX = createAttributeDimensionRegion(
      baseReceiver,
      baseHandles.x,
      10UL,
      20UL,
      "BaseValue X receiver region");
  auto const yReceiverHigh = createAttributeDimensionRegion(
      yReceiver,
      yHandles.y,
      yUpperBound - 1UL,
      yUpperBound,
      "Y-only receiver region at the FDD default range's upper edge");
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePairs{
      {rti::AttributeHandleSet{publisherHandles.inheritedBaseValue},
       rti::RegionHandleSet{baseSourceX}},
      {rti::AttributeHandleSet{publisherHandles.derivedValue},
       rti::RegionHandleSet{derivedSourceX}},
  };
  rti::AttributeHandleSetRegionHandleSetPairVector const baseSubscription{{
      rti::AttributeHandleSet{baseHandles.inheritedBaseValue},
      rti::RegionHandleSet{baseReceiverX},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const ySubscription{{
      rti::AttributeHandleSet{yHandles.derivedValue},
      rti::RegionHandleSet{yReceiverHigh},
  }};
  baseAmbassador.subscribeObjectClassAttributesWithRegions(
      baseHandles.derivedClass, baseSubscription);
  yAmbassador.subscribeObjectClassAttributesWithRegions(
      yHandles.derivedClass, ySubscription);

  auto const object = publisherAmbassador.registerObjectInstanceWithRegions(
      publisherHandles.derivedClass, sourcePairs);
  require(
      object.isValid(),
      "FDD unspecified-dimension regional registration returned an invalid object handle");
  waitForSessions(
      {&baseReceiver, &yReceiver},
      [&] {
        return baseReceiver.recorder().hasDiscovery(object) &&
            yReceiver.recorder().hasDiscovery(object);
      },
      options,
      "object discovery for explicit X and implicit default-Y source dimensions");

  std::vector<std::uint8_t> const baseValue{0x71U, 0x72U};
  std::vector<std::uint8_t> const derivedValue{0x81U, 0x82U};
  std::vector<std::uint8_t> const tag{0x76U, 0x77U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.inheritedBaseValue,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  values.emplace(
      publisherHandles.derivedValue,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  publisherAmbassador.updateAttributeValues(
      object, values, rti::VariableLengthData(tag.data(), tag.size()));
  waitForSessions(
      {&baseReceiver, &yReceiver},
      [&] {
        return baseReceiver.recorder().reflectionCount() >= 1U &&
            yReceiver.recorder().reflectionCount() >= 1U;
      },
      options,
      "FDD default-range routing for the omitted Y source dimension");
  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    baseReceiver.pump();
    yReceiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  requireOnlyAttributeReflection(
      baseReceiver,
      object,
      baseHandles.inheritedBaseValue,
      baseHandles.derivedValue,
      baseValue,
      tag,
      publisher.federateHandle(),
      "X receiver for the inherited BaseValue source region");
  requireOnlyAttributeReflection(
      yReceiver,
      object,
      yHandles.derivedValue,
      yHandles.inheritedBaseValue,
      derivedValue,
      tag,
      publisher.federateHandle(),
      "Y-only receiver reached through FDD default range [0, upperBound)");

  publisherAmbassador.unassociateRegionsForUpdates(object, sourcePairs);
  baseAmbassador.unsubscribeObjectClassAttributesWithRegions(
      baseHandles.derivedClass, baseSubscription);
  yAmbassador.unsubscribeObjectClassAttributesWithRegions(
      yHandles.derivedClass, ySubscription);
  publisherAmbassador.deleteRegion(baseSourceX);
  publisherAmbassador.deleteRegion(derivedSourceX);
  baseAmbassador.deleteRegion(baseReceiverX);
  yAmbassador.deleteRegion(yReceiverHigh);
  publisherAmbassador.unpublishObjectClassAttributes(
      publisherHandles.derivedClass, publishedAttributes);
  yReceiver.resign(rti::NO_ACTION);
  baseReceiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
  yReceiver.disconnect();
  baseReceiver.disconnect();
  publisher.disconnect();
}

void scenarioRegionalObjectAttributeUnspecifiedDimensionDefaultRangeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalObjectAttributeUnspecifiedDimensionDefaultRange(options, model);
}

void scenarioRegionalObjectAttributeExcludedDimensionNotRealized(
    Options const& options,
    rti::CallbackModel model) {
  auto const excludedDimensionsFom = options.scenarioFoms.find(
      "ddm-region-realization-excluded-dimensions");
  require(
      excludedDimensionsFom != options.scenarioFoms.end(),
      "Object-attribute excluded-dimension test requires its adapter FOM");

  Session publisher(options, model, "attribute-excluded-dimension-publisher");
  Session xReceiver(options, model, "attribute-excluded-dimension-x-receiver");
  Session yReceiver(options, model, "attribute-excluded-dimension-y-receiver");
  auto const federation =
      federationName(options, "regional-object-attribute-excluded-dimension-not-realized");
  connectAndJoin(
      publisher,
      xReceiver,
      options,
      federation,
      excludedDimensionsFom->second);
  yReceiver.connect();
  yReceiver.join(
      options.memberFederateName + L"-attribute-excluded-dimension-y",
      options.federateType,
      federation);

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& xAmbassador = xReceiver.rtiAmbassador();
  auto& yAmbassador = yReceiver.rtiAmbassador();
  auto const objectClass = publisherAmbassador.getObjectClassHandle(
      L"HLAobjectRoot.TckObject");
  auto const xObjectClass = xAmbassador.getObjectClassHandle(
      L"HLAobjectRoot.TckObject");
  auto const yObjectClass = yAmbassador.getObjectClassHandle(
      L"HLAobjectRoot.TckObject");
  auto const attribute = publisherAmbassador.getAttributeHandle(
      objectClass, L"Value");
  auto const xAttribute = xAmbassador.getAttributeHandle(xObjectClass, L"Value");
  auto const yAttribute = yAmbassador.getAttributeHandle(yObjectClass, L"Value");
  auto const x = publisherAmbassador.getDimensionHandle(L"TckDimensionX");
  auto const y = publisherAmbassador.getDimensionHandle(L"TckDimensionY");
  auto const xReceiverDimension = xAmbassador.getDimensionHandle(L"TckDimensionX");
  auto const yReceiverDimension = yAmbassador.getDimensionHandle(L"TckDimensionY");
  require(
      objectClass.isValid() && xObjectClass == objectClass &&
          yObjectClass == objectClass && attribute.isValid() &&
          xAttribute == attribute && yAttribute == attribute && x.isValid() &&
          y.isValid() && xReceiverDimension == x && yReceiverDimension == y,
      "members resolved inconsistent object-attribute excluded-dimension handles");
  require(
      publisherAmbassador.getAvailableDimensionsForObjectClass(objectClass) ==
              rti::DimensionHandleSet{x, y} &&
          publisherAmbassador.getDimensionUpperBound(x) == 10UL &&
          publisherAmbassador.getDimensionUpperBound(y) == 10UL,
      "excluded-dimension object FOM did not expose X/Y with its declared bounds");

  rti::AttributeHandleSet const attributes{attribute};
  publisherAmbassador.publishObjectClassAttributes(objectClass, attributes);
  auto const sourceX = createAttributeDimensionRegion(
      publisher, x, 1UL, 4UL, "excluded-dimension X-only source");
  auto const receiverX = createAttributeDimensionRegion(
      xReceiver, xReceiverDimension, 2UL, 5UL, "excluded-dimension X receiver");
  auto const receiverY = createAttributeDimensionRegion(
      yReceiver, yReceiverDimension, 1UL, 4UL, "excluded-dimension Y receiver");

  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePairs{{
      attributes,
      rti::RegionHandleSet{sourceX},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const xSubscription{{
      rti::AttributeHandleSet{xAttribute},
      rti::RegionHandleSet{receiverX},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const ySubscription{{
      rti::AttributeHandleSet{yAttribute},
      rti::RegionHandleSet{receiverY},
  }};
  xAmbassador.subscribeObjectClassAttributesWithRegions(
      xObjectClass, xSubscription);
  yAmbassador.subscribeObjectClassAttributesWithRegions(
      yObjectClass, ySubscription);
  auto const object = publisherAmbassador.registerObjectInstanceWithRegions(
      objectClass, sourcePairs);
  require(
      object.isValid(),
      "excluded-dimension regional registration returned an invalid object handle");
  waitForSessions(
      {&xReceiver, &yReceiver},
      [&] { return xReceiver.recorder().hasDiscovery(object); },
      options,
      "X-overlapping object discovery with excluded Y omitted");

  std::vector<std::uint8_t> const value{0x51U, 0x52U};
  std::vector<std::uint8_t> const tag{0x61U, 0x62U};
  rti::AttributeHandleValueMap values;
  values.emplace(attribute, rti::VariableLengthData(value.data(), value.size()));
  publisherAmbassador.updateAttributeValues(
      object, values, rti::VariableLengthData(tag.data(), tag.size()));
  waitForSessions(
      {&xReceiver, &yReceiver},
      [&] { return xReceiver.recorder().reflectionCount() >= 1U; },
      options,
      "X-only positive-control reflection with Y excluded by the FDD");
  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    xReceiver.pump();
    yReceiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto const xReflections = xReceiver.recorder().reflections();
  require(
      xReflections.size() == 1U,
      "overlapping X receiver did not receive exactly one reflection");
  auto const& xReflection = xReflections.front();
  auto const reflectedValue = xReflection.values.find(xAttribute);
  require(
      xReflection.present && xReflection.object == object &&
          xReflection.values.size() == 1U &&
          reflectedValue != xReflection.values.end() &&
          copyBytes(reflectedValue->second) == value && xReflection.tag == tag &&
          xReflection.producer == publisher.federateHandle(),
      "X positive control received the wrong object, value, tag, or producer");
  require(
      !yReceiver.recorder().hasDiscovery(object) &&
          yReceiver.recorder().reflections().empty(),
      "FDD-excluded Y was implicitly realized for object discovery or update delivery");

  publisherAmbassador.unassociateRegionsForUpdates(object, sourcePairs);
  xAmbassador.unsubscribeObjectClassAttributesWithRegions(
      xObjectClass, xSubscription);
  yAmbassador.unsubscribeObjectClassAttributesWithRegions(
      yObjectClass, ySubscription);
  publisherAmbassador.deleteRegion(sourceX);
  xAmbassador.deleteRegion(receiverX);
  yAmbassador.deleteRegion(receiverY);
  publisherAmbassador.unpublishObjectClassAttributes(objectClass, attributes);
  yReceiver.resign(rti::NO_ACTION);
  xReceiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
  yReceiver.disconnect();
  xReceiver.disconnect();
  publisher.disconnect();
}

void scenarioRegionalObjectAttributeExcludedDimensionNotRealizedContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalObjectAttributeExcludedDimensionNotRealized(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
