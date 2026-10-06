#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Federation Restore Begun at recipient files",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting]"
    "[federation-restore-begun-service-report]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.abort-federation-restore]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]") {
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
      L"federation-restore-begun-report-requester", L"restore", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"federation-restore-begun-report-peer", L"restore", federationName));

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
  auto const requesterReportFile =
      reportFileFor("federation-restore-begun-report-requester");
  auto const peerReportFile = reportFileFor("federation-restore-begun-report-peer");
  auto const requesterDesignator = joinedFederateReportDesignator(requesterReportFile);
  auto const peerDesignator = joinedFederateReportDesignator(peerReportFile);

  // Establish a real two-member snapshot. These save records establish the
  // recipient-local report serials asserted below without conflating this
  // focused §4.29 regression with the save service forms.
  REQUIRE_NOTHROW(requester->requestFederationSave(L"restore-begun-report-snapshot"));
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE(requesterReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"restore-begun-report-snapshot"});
  REQUIRE(peerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"restore-begun-report-snapshot"});
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

  REQUIRE_NOTHROW(requester->requestFederationRestore(L"restore-begun-report-snapshot"));
  auto const requestRecord =
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"restore-begun-report-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const confirmationRecord =
      R"({"HLAserialNumber":6,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"restore-begun-report-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const restoreBegunRecord = [](std::uint32_t serialNumber) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  };
  auto const requesterInitiateRestore = initiateFederateRestoreServiceReportRecord(
      8U,
      "restore-begun-report-snapshot",
      requesterDesignator,
      "federation-restore-begun-report-requester");
  auto const peerInitiateRestore = initiateFederateRestoreServiceReportRecord(
      5U,
      "restore-begun-report-snapshot",
      peerDesignator,
      "federation-restore-begun-report-peer");

  // §4.29 is RTI-initiated at every joined federate, including the requester.
  // Each record belongs to that recipient's existing report file and is
  // durable before any request-success or Federation Restore Begun callback
  // can be evoked.
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeRestore + requestRecord +
          confirmationRecord + restoreBegunRecord(7U) + requesterInitiateRestore);
  REQUIRE(readTextFile(peerReportFile) ==
          peerBeforeRestore + restoreBegunRecord(4U) + peerInitiateRestore);
  REQUIRE(requesterReports.federationRestoreBegunReportCount == 0U);
  REQUIRE(peerReports.federationRestoreBegunReportCount == 0U);

  while (requester->evokeCallback(0.0)) {
  }
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"restore-begun-report-snapshot"});
  REQUIRE(requesterReports.federationRestoreBegunReportCount == 1U);
  REQUIRE(peerReports.federationRestoreBegunReportCount == 1U);

  // Abort only to close the real multi-member restore operation; its service
  // report shape is covered by its dedicated lane.
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
