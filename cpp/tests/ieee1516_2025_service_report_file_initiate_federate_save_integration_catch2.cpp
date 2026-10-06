#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Initiate Federate Save at recipient files",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting]"
    "[initiate-federate-save-service-report]"
    "[rti.service.request-federation-save]"
    "[federate.callback.initiate-federate-save]") {
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
      L"initiate-save-report-requester", L"save", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"initiate-save-report-peer", L"save", federationName));

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
  auto const requesterReportFile = reportFileFor("initiate-save-report-requester");
  auto const peerReportFile = reportFileFor("initiate-save-report-peer");
  auto const requesterBeforeSave = readTextFile(requesterReportFile);
  auto const peerBeforeSave = readTextFile(peerReportFile);

  REQUIRE_NOTHROW(requester->requestFederationSave(L"recipient-file-save"));
  auto const requestRecord =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationSave","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"recipient-file-save"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const initiateRecord = [](std::uint32_t serialNumber) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"InitiateFederateSave","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"recipient-file-save"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  // §4.20 is RTI-initiated at both joined federates. Its record belongs to
  // each recipient's own report file and is durable before either evoked
  // callback can reveal the instruction.
  REQUIRE(readTextFile(requesterReportFile) ==
          requesterBeforeSave + requestRecord + initiateRecord(1U));
  REQUIRE(readTextFile(peerReportFile) == peerBeforeSave + initiateRecord(0U));
  REQUIRE(requesterReports.initiateFederateSaveReports.empty());
  REQUIRE(peerReports.initiateFederateSaveReports.empty());

  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE(requesterReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"recipient-file-save"});
  REQUIRE(peerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"recipient-file-save"});

  // Finish the real shared operation so teardown does not rely on resignation
  // to discard active save-control state.
  REQUIRE_NOTHROW(requester->abortFederationSave());
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE_FALSE(peer->evokeCallback(0.0));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
}  // namespace
