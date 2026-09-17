#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional multi-attribute DDM test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-multi-attribute-ddm-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path packageDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "packages" /
      "umbra-rti-native" / "tests" / "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<ReflectionReport> attributeReflectionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded two-dimensional regional object updates filter independent attribute sources",
    "[integration][development-profile][object-management][ddm]"
    "[ddm-regional-multi-attribute]"
    "[rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  // Each attribute has its own source region. An X-only or Y-only mutation
  // therefore cannot accidentally use the other attribute's overlap.
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador flavorReports;
  ReportingFederateAmbassador organicReports;
  auto publisher = makeRti();
  auto flavorSubscriber = makeRti();
  auto organicSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = packageDataPath(
      "regional-two-dimensional-multi-attribute-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(flavorSubscriber->connect(flavorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(organicSubscriber->connect(organicReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"cpp-2d-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(flavorSubscriber->joinFederationExecution(
      L"cpp-2d-attribute-flavor", L"flavor", federationName));
  REQUIRE_NOTHROW(organicSubscriber->joinFederationExecution(
      L"cpp-2d-attribute-organic", L"organic", federationName));

  auto const publisherClass = publisher->getObjectClassHandle(
      fixture_hla::fom::regional_thing);
  auto const flavorClass = flavorSubscriber->getObjectClassHandle(
      fixture_hla::fom::regional_thing);
  auto const organicClass = organicSubscriber->getObjectClassHandle(
      fixture_hla::fom::regional_thing);
  REQUIRE(publisherClass.isValid());
  REQUIRE(flavorClass.isValid());
  REQUIRE(organicClass.isValid());
  auto const publisherFlavor = publisher->getAttributeHandle(
      publisherClass,
      fixture_hla::fixture::flavor);
  auto const publisherOrganic = publisher->getAttributeHandle(
      publisherClass,
      fixture_hla::fixture::organic);
  auto const flavorAttribute = flavorSubscriber->getAttributeHandle(
      flavorClass,
      fixture_hla::fixture::flavor);
  auto const organicAttribute = organicSubscriber->getAttributeHandle(
      organicClass,
      fixture_hla::fixture::organic);
  REQUIRE(publisherFlavor.isValid());
  REQUIRE(publisherOrganic.isValid());
  REQUIRE(flavorAttribute.isValid());
  REQUIRE(organicAttribute.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass,
      AttributeHandleSet{publisherFlavor, publisherOrganic}));

  auto const publisherX = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const publisherY = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const flavorX = flavorSubscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const flavorY = flavorSubscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const organicX = organicSubscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const organicY = organicSubscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  DimensionHandleSet const publisherDimensions{publisherX, publisherY};
  auto const sourceFlavor = publisher->createRegion(publisherDimensions);
  auto const sourceOrganic = publisher->createRegion(publisherDimensions);
  auto const flavorRegion = flavorSubscriber->createRegion(
      DimensionHandleSet{flavorX, flavorY});
  auto const organicRegion = organicSubscriber->createRegion(
      DimensionHandleSet{organicX, organicY});

  auto setBounds = [](RTIambassador& ambassador,
                      RegionHandle const& region,
                      DimensionHandle const& x,
                      DimensionHandle const& y,
                      unsigned long xLower,
                      unsigned long xUpper,
                      unsigned long yLower,
                      unsigned long yUpper) {
    REQUIRE_NOTHROW(ambassador.setRangeBounds(
        region,
        x,
        RangeBounds(xLower, xUpper)));
    REQUIRE_NOTHROW(ambassador.setRangeBounds(
        region,
        y,
        RangeBounds(yLower, yUpper)));
  };
  setBounds(*publisher, sourceFlavor, publisherX, publisherY, 0UL, 2UL, 0UL, 2UL);
  setBounds(*publisher, sourceOrganic, publisherX, publisherY, 5UL, 7UL, 5UL, 7UL);
  setBounds(*flavorSubscriber, flavorRegion, flavorX, flavorY, 0UL, 2UL, 0UL, 2UL);
  setBounds(*organicSubscriber, organicRegion, organicX, organicY, 5UL, 7UL, 5UL, 7UL);
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceFlavor, sourceOrganic}));
  REQUIRE_NOTHROW(flavorSubscriber->commitRegionModifications(
      RegionHandleSet{flavorRegion}));
  REQUIRE_NOTHROW(organicSubscriber->commitRegionModifications(
      RegionHandleSet{organicRegion}));
  REQUIRE_NOTHROW(flavorSubscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(organicSubscriber->setConveyRegionDesignatorSetsSwitch(true));

  AttributeHandleSetRegionHandleSetPairVector const publisherPairs{
      {AttributeHandleSet{publisherFlavor}, RegionHandleSet{sourceFlavor}},
      {AttributeHandleSet{publisherOrganic}, RegionHandleSet{sourceOrganic}},
  };
  AttributeHandleSetRegionHandleSetPairVector const flavorPairs{
      {AttributeHandleSet{flavorAttribute}, RegionHandleSet{flavorRegion}},
  };
  AttributeHandleSetRegionHandleSetPairVector const organicPairs{
      {AttributeHandleSet{organicAttribute}, RegionHandleSet{organicRegion}},
  };
  REQUIRE_NOTHROW(flavorSubscriber->subscribeObjectClassAttributesWithRegions(
      flavorClass,
      flavorPairs));
  REQUIRE_NOTHROW(organicSubscriber->subscribeObjectClassAttributesWithRegions(
      organicClass,
      organicPairs));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      publisherClass,
      publisherPairs));
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*flavorSubscriber);
  drainCallbacks(*organicSubscriber);
  REQUIRE(flavorReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(organicReports.objectDiscoveryReports.size() == 1U);

  auto sendValues = [&](std::string_view flavorValue,
                        std::string_view organicValue,
                        std::string_view tag) {
    AttributeHandleValueMap values;
    values.emplace(
        publisherFlavor,
        VariableLengthData(flavorValue.data(), flavorValue.size()));
    values.emplace(
        publisherOrganic,
        VariableLengthData(organicValue.data(), organicValue.size()));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData(tag.data(), tag.size())));
    drainCallbacks(*flavorSubscriber);
    drainCallbacks(*organicSubscriber);
  };

  sendValues("flavor-initial", "organic-initial", "initial");
  REQUIRE(flavorReports.attributeReflectionReports.size() == 1U);
  REQUIRE(organicReports.attributeReflectionReports.size() == 1U);
  REQUIRE(flavorReports.attributeReflectionReports.back().attributeValues.size() == 1U);
  REQUIRE(organicReports.attributeReflectionReports.back().attributeValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(
              flavorReports.attributeReflectionReports.back().attributeValues.at(
                  flavorAttribute)) ==
          std::vector<unsigned char>{'f', 'l', 'a', 'v', 'o', 'r', '-', 'i', 'n', 'i', 't', 'i', 'a', 'l'});
  REQUIRE(variableLengthDataBytes(
              organicReports.attributeReflectionReports.back().attributeValues.at(
                  organicAttribute)) ==
          std::vector<unsigned char>{'o', 'r', 'g', 'a', 'n', 'i', 'c', '-', 'i', 'n', 'i', 't', 'i', 'a', 'l'});
  REQUIRE(flavorReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(organicReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(flavorReports.attributeReflectionReports.back().sentRegions ==
          RegionHandleSet{sourceFlavor});
  REQUIRE(organicReports.attributeReflectionReports.back().sentRegions ==
          RegionHandleSet{sourceOrganic});

  // Moving only Flavor outside X overlap must not suppress Organic, whose
  // independent source still overlaps in both dimensions.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceFlavor,
      publisherX,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceFlavor}));
  auto const flavorCountAfterInitial = flavorReports.attributeReflectionReports.size();
  auto const organicCountAfterInitial = organicReports.attributeReflectionReports.size();
  sendValues("flavor-x-disjoint", "organic-x-live", "x-only");
  REQUIRE(flavorReports.attributeReflectionReports.size() == flavorCountAfterInitial);
  REQUIRE(organicReports.attributeReflectionReports.size() == organicCountAfterInitial + 1U);
  REQUIRE(variableLengthDataBytes(
              organicReports.attributeReflectionReports.back().attributeValues.at(
                  organicAttribute)) ==
          std::vector<unsigned char>{'o', 'r', 'g', 'a', 'n', 'i', 'c', '-', 'x', '-', 'l', 'i', 'v', 'e'});

  // Moving Organic outside only Y overlap removes its remaining source.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceOrganic,
      publisherY,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceOrganic}));
  auto const flavorCountAfterX = flavorReports.attributeReflectionReports.size();
  auto const organicCountAfterX = organicReports.attributeReflectionReports.size();
  sendValues("flavor-none", "organic-none", "none");
  REQUIRE(flavorReports.attributeReflectionReports.size() == flavorCountAfterX);
  REQUIRE(organicReports.attributeReflectionReports.size() == organicCountAfterX);

  // Restoring both source ranges re-enables both recipient-local projections
  // exactly once and retains the source RegionHandleSet in each callback.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceFlavor,
      publisherX,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceOrganic,
      publisherY,
      RangeBounds(5UL, 7UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceFlavor, sourceOrganic}));
  auto const flavorCountBeforeRestore = flavorReports.attributeReflectionReports.size();
  auto const organicCountBeforeRestore = organicReports.attributeReflectionReports.size();
  sendValues("flavor-restored", "organic-restored", "restored");
  REQUIRE(flavorReports.attributeReflectionReports.size() == flavorCountBeforeRestore + 1U);
  REQUIRE(organicReports.attributeReflectionReports.size() == organicCountBeforeRestore + 1U);
  REQUIRE(flavorReports.attributeReflectionReports.back().sentRegions ==
          RegionHandleSet{sourceFlavor});
  REQUIRE(organicReports.attributeReflectionReports.back().sentRegions ==
          RegionHandleSet{sourceOrganic});

  REQUIRE_NOTHROW(flavorSubscriber->unsubscribeObjectClassAttributesWithRegions(
      flavorClass,
      flavorPairs));
  REQUIRE_NOTHROW(organicSubscriber->unsubscribeObjectClassAttributesWithRegions(
      organicClass,
      organicPairs));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      publisherPairs));
  REQUIRE_NOTHROW(flavorSubscriber->deleteRegion(flavorRegion));
  REQUIRE_NOTHROW(organicSubscriber->deleteRegion(organicRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceFlavor));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceOrganic));
  REQUIRE_NOTHROW(organicSubscriber->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(flavorSubscriber->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(organicSubscriber->disconnect());
  REQUIRE_NOTHROW(flavorSubscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
