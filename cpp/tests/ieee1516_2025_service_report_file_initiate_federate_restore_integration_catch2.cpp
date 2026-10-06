#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Initiate Federate Restore at recipient files",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting]"
    "[initiate-federate-restore-service-report]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto requesterConfiguration = configurationForServiceReportDirectory(directory.path());
  auto peerConfiguration = configurationForServiceReportDirectory(directory.path());
  requesterConfiguration.withRtiAddress(L"in-process");
  peerConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, requesterConfiguration));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED, peerConfiguration));
  REQUIRE_NOTHROW(
      requester->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"initiate-restore-report-requester", L"restore", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"initiate-restore-report-peer", L"restore", federationName));

  auto const reportFileFor = [&directory](std::string const& federateName) {
    auto const marker = std::string{"\"HLAfederateName\":\""} + federateName + "\"";
    for (auto const& candidate : serviceReportFiles(directory.path())) {
      if (readTextFile(candidate).find(marker) != std::string::npos) {
        return candidate;
      }
    }
    FAIL("joined federate service-report file was not found");
    return std::filesystem::path{};
  };
  auto const requesterReportFile = reportFileFor("initiate-restore-report-requester");
  auto const peerReportFile = reportFileFor("initiate-restore-report-peer");
  auto const requesterDesignator = joinedFederateReportDesignator(requesterReportFile);
  auto const peerDesignator = joinedFederateReportDesignator(peerReportFile);

  // Establish one real two-member snapshot. These prerequisite report records
  // merely fix the recipient-local serial positions for the §4.30 callback.
  REQUIRE_NOTHROW(requester->requestFederationSave(L"initiate-restore-report-snapshot"));
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE_NOTHROW(requester->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(requester->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE(requesterReports.federationSavedReportCount == 1U);
  REQUIRE(peerReports.federationSavedReportCount == 1U);
  auto const requesterBeforeRestore = readTextFile(requesterReportFile);
  auto const peerBeforeRestore = readTextFile(peerReportFile);

  REQUIRE_NOTHROW(requester->requestFederationRestore(L"initiate-restore-report-snapshot"));
  auto const requestRecord =
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"initiate-restore-report-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const confirmationRecord =
      R"({"HLAserialNumber":6,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"initiate-restore-report-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const restoreBegunRecord = [](std::uint32_t serialNumber) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  // §4.30 retains all three supplied values at each recipient's own file.
  // The immutable report is written before the later HLA_EVOKED callback can
  // expose the restore instruction to either federate.
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeRestore + requestRecord +
          confirmationRecord + restoreBegunRecord(7U) +
          initiateFederateRestoreServiceReportRecord(
              8U,
              "initiate-restore-report-snapshot",
              requesterDesignator,
              "initiate-restore-report-requester"));
  REQUIRE(readTextFile(peerReportFile) == peerBeforeRestore + restoreBegunRecord(4U) +
          initiateFederateRestoreServiceReportRecord(
              5U,
              "initiate-restore-report-snapshot",
              peerDesignator,
              "initiate-restore-report-peer"));
  REQUIRE(requesterReports.initiateFederateRestoreReports.empty());
  REQUIRE(peerReports.initiateFederateRestoreReports.empty());

  while (requester->evokeCallback(0.0)) {
  }
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(peerReports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(requesterReports.initiateFederateRestoreReports.front().label ==
          L"initiate-restore-report-snapshot");
  REQUIRE(requesterReports.initiateFederateRestoreReports.front().federateName ==
          L"initiate-restore-report-requester");
  REQUIRE(peerReports.initiateFederateRestoreReports.front().federateName ==
          L"initiate-restore-report-peer");

  // Abort only to close the real multi-member restore operation; its separate
  // no-argument report form remains covered by the §4.33 lane.
  REQUIRE_NOTHROW(requester->abortFederationRestore());
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
}  // namespace
