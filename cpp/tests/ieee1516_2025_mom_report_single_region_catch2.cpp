#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 MOM single-update-region test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"ieee1516-2025-mom-report-single-region-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReportObserver final : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    interactions.push_back(interactionClass);
    sentRegionSetWasPresent = optionalSentRegions != nullptr;
    sentRegionCount = optionalSentRegions == nullptr ? 0U : optionalSentRegions->size();
    if (optionalSentRegions == nullptr || regionQueryAmbassador == nullptr) {
      return;
    }
    try {
      for (auto const& region : *optionalSentRegions) {
        auto const dimensions = regionQueryAmbassador->getDimensionHandleSet(region);
        for (auto const& dimension : dimensions) {
          auto const name = regionQueryAmbassador->getDimensionName(dimension);
          auto const bounds = regionQueryAmbassador->getRangeBounds(region, dimension);
          sentRegionDimensionNames.insert(name);
          sentRegionBounds.emplace(
              name,
              std::pair{bounds.getLowerBound(), bounds.getUpperBound()});
        }
      }
    } catch (...) {
      regionInspectionFailed = true;
    }
  }

  rti1516_2025::RTIambassador* regionQueryAmbassador = nullptr;
  std::vector<InteractionClassHandle> interactions;
  bool sentRegionSetWasPresent = false;
  std::size_t sentRegionCount = 0U;
  bool regionInspectionFailed = false;
  std::set<std::wstring> sentRegionDimensionNames;
  std::map<std::wstring, std::pair<unsigned long, unsigned long>> sentRegionBounds;
};

void setPointRange(
    rti1516_2025::RTIambassador& rti,
    RegionHandle region,
    DimensionHandle dimension,
    unsigned long point) {
  REQUIRE_NOTHROW(rti.setRangeBounds(
      region,
      dimension,
      RangeBounds(point, point + 1UL)));
}

}  // namespace

TEST_CASE(
    "HLAreportServiceInvocation is routed through one point-region for its federate and service group",
    "[integration][development-profile][2025][mom][service-reporting]"
    "[mom-report-single-update-region][ddm]"
    "[rti.service.normalize-federate-handle][rti.service.normalize-service-group]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.get-dimension-handle-set][rti.service.get-dimension-name]"
    "[rti.service.get-range-bounds]"
    "[federate.callback.receive-interaction]") {
  ReportObserver matchingCallbacks;
  ReportObserver wrongServiceGroupCallbacks;
  rti1516_2025::NullFederateAmbassador publisherCallbacks;
  auto publisher = makeRti();
  auto matchingObserver = makeRti();
  auto wrongServiceGroupObserver = makeRti();
  matchingCallbacks.regionQueryAmbassador = matchingObserver.get();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(matchingObserver->connect(matchingCallbacks, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(wrongServiceGroupObserver->connect(
      wrongServiceGroupCallbacks, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mom-report-region-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(matchingObserver->joinFederationExecution(
      L"mom-report-region-matching-observer", L"matching-observer", federationName));
  REQUIRE_NOTHROW(wrongServiceGroupObserver->joinFederationExecution(
      L"mom-report-region-wrong-group-observer", L"wrong-group-observer", federationName));

  auto const reportClass = matchingObserver->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  auto const wrongReportClass = wrongServiceGroupObserver->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  auto const federateDimension = matchingObserver->getDimensionHandle(L"HLAfederate");
  auto const serviceGroupDimension = matchingObserver->getDimensionHandle(L"HLAserviceGroup");
  auto const wrongFederateDimension =
      wrongServiceGroupObserver->getDimensionHandle(L"HLAfederate");
  auto const wrongServiceGroupDimension =
      wrongServiceGroupObserver->getDimensionHandle(L"HLAserviceGroup");
  REQUIRE(reportClass.isValid());
  REQUIRE(wrongReportClass.isValid());
  REQUIRE(federateDimension.isValid());
  REQUIRE(serviceGroupDimension.isValid());
  REQUIRE(wrongFederateDimension.isValid());
  REQUIRE(wrongServiceGroupDimension.isValid());

  auto const normalizedFederate =
      matchingObserver->normalizeFederateHandle(publisherHandle);
  auto const wrongObserverNormalizedFederate =
      wrongServiceGroupObserver->normalizeFederateHandle(publisherHandle);
  auto const objectManagementGroup =
      matchingObserver->normalizeServiceGroup(rti1516_2025::OBJECT_MANAGEMENT);
  auto const supportServicesGroup =
      matchingObserver->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES);
  auto const wrongGroup =
      wrongServiceGroupObserver->normalizeServiceGroup(rti1516_2025::OBJECT_MANAGEMENT);
  REQUIRE(wrongObserverNormalizedFederate == normalizedFederate);
  REQUIRE(supportServicesGroup != objectManagementGroup);

  auto const matchingRegion = matchingObserver->createRegion(
      DimensionHandleSet{federateDimension, serviceGroupDimension});
  auto const wrongServiceGroupRegion = wrongServiceGroupObserver->createRegion(
      DimensionHandleSet{wrongFederateDimension, wrongServiceGroupDimension});
  REQUIRE(matchingRegion.isValid());
  REQUIRE(wrongServiceGroupRegion.isValid());
  setPointRange(*matchingObserver, matchingRegion, federateDimension, normalizedFederate);
  setPointRange(*matchingObserver, matchingRegion, serviceGroupDimension, supportServicesGroup);
  setPointRange(
      *wrongServiceGroupObserver,
      wrongServiceGroupRegion,
      wrongFederateDimension,
      wrongObserverNormalizedFederate);
  setPointRange(
      *wrongServiceGroupObserver,
      wrongServiceGroupRegion,
      wrongServiceGroupDimension,
      wrongGroup);
  REQUIRE_NOTHROW(matchingObserver->commitRegionModifications(
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(wrongServiceGroupObserver->commitRegionModifications(
      RegionHandleSet{wrongServiceGroupRegion}));
  REQUIRE_NOTHROW(matchingObserver->subscribeInteractionClassWithRegions(
      reportClass, RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(wrongServiceGroupObserver->subscribeInteractionClassWithRegions(
      wrongReportClass, RegionHandleSet{wrongServiceGroupRegion}));
  REQUIRE_NOTHROW(matchingObserver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));

  unsigned long normalizedByReportedService = 0UL;
  REQUIRE_NOTHROW(normalizedByReportedService =
      publisher->normalizeFederateHandle(publisherHandle));
  REQUIRE(normalizedByReportedService == normalizedFederate);

  REQUIRE(matchingCallbacks.interactions.size() == 1U);
  REQUIRE(matchingCallbacks.interactions.front() == reportClass);
  REQUIRE(matchingCallbacks.sentRegionSetWasPresent);
  REQUIRE(matchingCallbacks.sentRegionCount == 1U);
  REQUIRE_FALSE(matchingCallbacks.regionInspectionFailed);
  CHECK((matchingCallbacks.sentRegionDimensionNames ==
         std::set<std::wstring>{L"HLAfederate", L"HLAserviceGroup"}));
  CHECK(matchingCallbacks.sentRegionBounds.size() == 2U);
  CHECK((matchingCallbacks.sentRegionBounds.at(L"HLAfederate") ==
         std::pair<unsigned long, unsigned long>{
             normalizedFederate, normalizedFederate + 1UL}));
  CHECK((matchingCallbacks.sentRegionBounds.at(L"HLAserviceGroup") ==
         std::pair<unsigned long, unsigned long>{
             supportServicesGroup, supportServicesGroup + 1UL}));
  REQUIRE(wrongServiceGroupCallbacks.interactions.empty());
}
