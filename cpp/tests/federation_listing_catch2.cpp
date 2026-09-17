#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation-listing tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::FederationExecutionInformationVector;
using rti1516_2025::FederationExecutionMemberInformationVector;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
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
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto disconnected = makeRti();
  auto creator = makeRti();
  auto member = makeRti();
  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto const missingFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(evoked->listFederationExecutions(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->listFederationExecutionMembers(firstFederationName),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(disconnected->connect(disconnectedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(creator->connect(creatorFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(
          firstFederationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      creator->createFederationExecution(
          secondFederationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"listed-member", L"observer", firstFederationName));

  REQUIRE_NOTHROW(evoked->listFederationExecutions());
  REQUIRE(evokedReports.federationExecutionReports.empty());
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionReports.size() == 1);
  auto const& listedFederations = evokedReports.federationExecutionReports.front();
  REQUIRE(listedFederations.size() == 2);
  auto const first = std::find_if(
      listedFederations.begin(),
      listedFederations.end(),
      [&firstFederationName](auto const& federation) {
        return federation.federationExecutionName == firstFederationName;
      });
  REQUIRE(first != listedFederations.end());
  REQUIRE(first->logicalTimeImplementationName == standard_hla::mom::integer64_time);

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(firstFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.federationExecutionMemberReports.size() == 1);
  auto const& memberReport = evokedReports.federationExecutionMemberReports.front();
  REQUIRE(memberReport.federationName == firstFederationName);
  REQUIRE(memberReport.members.size() == 1);
  REQUIRE(memberReport.members.front().federateName == L"listed-member");
  REQUIRE(memberReport.members.front().federateType == L"observer");

  REQUIRE_NOTHROW(evoked->listFederationExecutionMembers(missingFederationName));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.missingFederationReports == std::vector<std::wstring>{missingFederationName});

  REQUIRE_NOTHROW(immediate->listFederationExecutions());
  REQUIRE(immediateReports.federationExecutionReports.size() == 1);
  REQUIRE(immediateReports.federationExecutionReports.front().size() == 2);

  // Disconnect must discard a report queued against the prior callback
  // session; a later Evoke cannot dereference that stale recipient.
  REQUIRE_NOTHROW(disconnected->listFederationExecutions());
  REQUIRE_NOTHROW(disconnected->disconnect());
  REQUIRE_FALSE(disconnected->evokeCallback(0.0));
  REQUIRE(disconnectedReports.federationExecutionReports.empty());

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(firstFederationName));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(secondFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}
