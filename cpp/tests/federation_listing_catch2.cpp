#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "ieee1516_2025_federation_listing_test_support.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation-listing tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederationExecutionInformationVector;
using rti1516_2025::FederationExecutionMemberInformationVector;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

class FederationListingFederateAmbassador final : public NullFederateAmbassador {
 public:
  void reportFederationExecutions(
      FederationExecutionInformationVector const& report) override {
    federationExecutionReports.push_back(report);
  }

  void reportFederationExecutionMembers(
      std::wstring const& federationName,
      FederationExecutionMemberInformationVector const& report) override {
    federationExecutionMemberReports.push_back({federationName, report});
  }

  void reportFederationExecutionDoesNotExist(
      std::wstring const& federationName) override {
    missingFederationReports.push_back(federationName);
  }

  struct MemberReport final {
    std::wstring federationName;
    FederationExecutionMemberInformationVector members;
  };

  std::vector<FederationExecutionInformationVector> federationExecutionReports;
  std::vector<MemberReport> federationExecutionMemberReports;
  std::vector<std::wstring> missingFederationReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-federation-listing-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded federation-list services dispatch standards reports in both callback models",
    "[integration][development-profile][federation-management][callbacks]"
    "[federation-listing][rti.service.list-federation-executions]"
    "[rti.service.list-federation-execution-members][2025]") {
  FederationListingFederateAmbassador evokedReports;
  FederationListingFederateAmbassador immediateReports;
  FederationListingFederateAmbassador disconnectedReports;
  NullFederateAmbassador creatorFederate;
  NullFederateAmbassador memberFederate;
  umbra::test::ieee1516_2025::runFederationListingScenario(
      evokedReports,
      immediateReports,
      disconnectedReports,
      creatorFederate,
      memberFederate,
      makeRti,
      resourcePath,
      nextFederationName);
}
