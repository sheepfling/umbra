#pragma once

#include <catch2/catch_test_macros.hpp>

#include <RTI/RTI1516.h>

#include "internal/fom/hla_names.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace umbra::test::ieee1516_2025 {

namespace standard_hla = umbra::detail::hla::wide;

template <
    typename ReportingAmbassador,
    typename CreatorAmbassador,
    typename MemberAmbassador,
    typename MakeRti,
    typename ResourcePath,
    typename NextFederationName>
void runFederationListingScenario(
    ReportingAmbassador& evokedReports,
    ReportingAmbassador& immediateReports,
    ReportingAmbassador& disconnectedReports,
    CreatorAmbassador& creatorFederate,
    MemberAmbassador& memberFederate,
    MakeRti makeRti,
    ResourcePath resourcePath,
    NextFederationName nextFederationName) {
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto disconnected = makeRti();
  auto creator = makeRti();
  auto member = makeRti();
  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto const missingFederationName = nextFederationName();
  auto const fomModule =
      resourcePath(std::filesystem::path("examples/RestaurantFOMmodule-2025.xml"))
          .wstring();

  REQUIRE_THROWS_AS(evoked->listFederationExecutions(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->listFederationExecutionMembers(firstFederationName),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(
      immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      disconnected->connect(disconnectedReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(
      creator->connect(creatorFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(member->connect(memberFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(creator->createFederationExecution(
      firstFederationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(creator->createFederationExecution(
      secondFederationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(member->joinFederationExecution(
      L"listed-member", L"observer", firstFederationName));

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
  REQUIRE(evokedReports.missingFederationReports ==
          std::vector<std::wstring>{missingFederationName});

  REQUIRE_NOTHROW(immediate->listFederationExecutions());
  REQUIRE(immediateReports.federationExecutionReports.size() == 1);
  REQUIRE(immediateReports.federationExecutionReports.front().size() == 2);

  // Disconnect must discard a report queued against the prior callback
  // session; a later Evoke cannot dereference that stale recipient.
  REQUIRE_NOTHROW(disconnected->listFederationExecutions());
  REQUIRE_NOTHROW(disconnected->disconnect());
  REQUIRE_FALSE(disconnected->evokeCallback(0.0));
  REQUIRE(disconnectedReports.federationExecutionReports.empty());

  REQUIRE_NOTHROW(member->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(firstFederationName));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(secondFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

}  // namespace umbra::test::ieee1516_2025
