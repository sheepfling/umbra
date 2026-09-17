#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The MOM service-reporting interlock test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::ReportServiceInvocationsAreSubscribed;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"mom-service-reporting-interlock-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded MOM service-reporting state excludes report-service subscriptions",
    "[integration][development-profile][federation-management]"
    "[mom][service-reporting][service-reporting-interlock][ddm]"
    "[support-switches]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]") {
  rti1516_2025::NullFederateAmbassador callbacks;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(callbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"mom-service-reporting-interlock-federate",
      L"interlock",
      federationName));

  auto const reportServiceInvocation = rti->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportServiceInvocation.isValid());
  REQUIRE_FALSE(rti->getServiceReportingSwitch());

  // Once enabled, both ordinary active and passive declarations are rejected
  // before they can become report-service subscriptions.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE(rti->getServiceReportingSwitch());
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(reportServiceInvocation),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(reportServiceInvocation, false),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE(rti->getServiceReportingSwitch());

  // HLAreportServiceInvocation carries the HLAserviceGroup dimension.  A
  // committed one-dimensional region keeps the regional path on the same
  // valid FOM context as the ordinary subscription path.
  auto const serviceGroup = rti->getDimensionHandle(L"HLAserviceGroup");
  REQUIRE(serviceGroup.isValid());
  RegionHandle region;
  REQUIRE_NOTHROW(region = rti->createRegion(DimensionHandleSet{serviceGroup}));
  REQUIRE(region.isValid());
  REQUIRE_NOTHROW(rti->setRangeBounds(
      region,
      serviceGroup,
      RangeBounds(0UL, 7UL)));
  REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));

  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClassWithRegions(
          reportServiceInvocation,
          RegionHandleSet{region}),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClassWithRegions(
          reportServiceInvocation,
          RegionHandleSet{region},
          false),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE(rti->getServiceReportingSwitch());

  // With reporting disabled, a passive ordinary subscription is legal but it
  // still counts as a subscription and therefore blocks re-enablement.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportServiceInvocation, false));
  REQUIRE_THROWS_AS(
      rti->setServiceReportingSwitch(true),
      ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(reportServiceInvocation));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE(rti->getServiceReportingSwitch());

  // The same subscription-count rule applies to a committed regional
  // declaration, for both active and passive selectors.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region}));
  REQUIRE_THROWS_AS(
      rti->setServiceReportingSwitch(true),
      ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region}));

  REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region},
      false));
  REQUIRE_THROWS_AS(
      rti->setServiceReportingSwitch(true),
      ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region}));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE(rti->getServiceReportingSwitch());

  REQUIRE_NOTHROW(rti->deleteRegion(region));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
