#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Federation Saved at recipient files",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting]"
    "[federation-saved-service-report]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.abort-federation-save]"
    "[federate.callback.federation-saved][federate.callback.federation-not-saved]") {
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
      L"federation-saved-report-requester", L"save", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"federation-saved-report-peer", L"save", federationName));

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
  auto const requesterReportFile = reportFileFor("federation-saved-report-requester");
  auto const peerReportFile = reportFileFor("federation-saved-report-peer");
  auto const federationSavedSuccess = [](std::uint32_t serialNumber) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"FederationSaved","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federation save-success indicator","HLAargumentValue":true},{"HLAargumentType":34,"HLAargumentName":"Optional failure reason","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };
  auto const federationSavedAborted = [](std::uint32_t serialNumber) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"FederationSaved","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federation save-success indicator","HLAargumentValue":false},{"HLAargumentType":48,"HLAargumentName":"Optional failure reason","HLAargumentValue":"SAVE_ABORTED"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  REQUIRE_NOTHROW(requester->requestFederationSave(L"federation-saved-success"));
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE_NOTHROW(requester->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(requester->federateSaveComplete());
  auto const requesterBeforeSuccessResult = readTextFile(requesterReportFile);
  auto const peerBeforeSuccessResult = readTextFile(peerReportFile);

  REQUIRE_NOTHROW(peer->federateSaveComplete());
  auto const peerSuccessfulComplete =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"FederateSaveComplete","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federate save-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  // §4.23 result records are recipient-local and durable before either
  // HLA_EVOKED success callback can observe the completed federation save.
  REQUIRE(readTextFile(requesterReportFile) ==
          requesterBeforeSuccessResult + federationSavedSuccess(4U));
  REQUIRE(readTextFile(peerReportFile) ==
          peerBeforeSuccessResult + peerSuccessfulComplete + federationSavedSuccess(3U));
  REQUIRE(requesterReports.federationSavedReportCount == 0U);
  REQUIRE(peerReports.federationSavedReportCount == 0U);
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE(requesterReports.federationSavedReportCount == 1U);
  REQUIRE(peerReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(requester->requestFederationSave(L"federation-saved-aborted"));
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  auto const requesterBeforeAbort = readTextFile(requesterReportFile);
  auto const peerBeforeAbort = readTextFile(peerReportFile);

  REQUIRE_NOTHROW(requester->abortFederationSave());
  auto const abortRecord =
      R"({"HLAserialNumber":7,"HLAreturnedArgument":[null],"HLAservice":"AbortFederationSave","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  // The failed §4.23 form uses the type-48 Table 5 reason and must remain
  // durable before the separate federationNotSaved callbacks are evoked.
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeAbort + abortRecord +
          federationSavedAborted(8U));
  REQUIRE(readTextFile(peerReportFile) == peerBeforeAbort + federationSavedAborted(5U));
  REQUIRE(requesterReports.federationNotSavedReasons.empty());
  REQUIRE(peerReports.federationNotSavedReasons.empty());
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE(requesterReports.federationNotSavedReasons ==
          std::vector<SaveFailureReason>{rti1516_2025::SAVE_ABORTED});
  REQUIRE(peerReports.federationNotSavedReasons ==
          std::vector<SaveFailureReason>{rti1516_2025::SAVE_ABORTED});

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
}  // namespace
