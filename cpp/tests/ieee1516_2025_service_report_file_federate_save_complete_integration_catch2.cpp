#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Federate Save Complete success-indicator forms",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][federate-save-complete]"
    "[federate-save-not-complete][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-save-not-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved][federate.callback.federation-not-saved]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"save-complete-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // §4.22 rejects a completion before §4.21 has put this member into the
  // saving state and must not reserve a successful-report serial.
  REQUIRE_THROWS_AS(rti->federateSaveComplete(), rti1516_2025::FederateHasNotBegunSave);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-save-success"));
  auto const afterSuccessfulRequest = readTextFile(reportFile);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  auto const expectedBegunSuccess =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"FederateSaveBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == afterSuccessfulRequest + expectedBegunSuccess);

  REQUIRE_NOTHROW(rti->federateSaveComplete());
  auto const expectedCompleteSuccess =
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"FederateSaveComplete","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federate save-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedFederationSavedSuccess =
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"FederationSaved","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federation save-success indicator","HLAargumentValue":true},{"HLAargumentType":34,"HLAargumentName":"Optional failure reason","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          afterSuccessfulRequest + expectedBegunSuccess + expectedCompleteSuccess +
              expectedFederationSavedSuccess);
  REQUIRE(reports.federationSavedReportCount == 0U);
  REQUIRE(reports.federationNotSavedReasons.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-save-failure"));
  auto const afterFailureRequest = readTextFile(reportFile);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  auto const expectedBegunFailure =
      R"({"HLAserialNumber":7,"HLAreturnedArgument":[null],"HLAservice":"FederateSaveBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == afterFailureRequest + expectedBegunFailure);

  REQUIRE_NOTHROW(rti->federateSaveNotComplete());
  auto const expectedCompleteFailure =
      R"({"HLAserialNumber":8,"HLAreturnedArgument":[null],"HLAservice":"FederateSaveComplete","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federate save-success indicator","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedFederationSavedFailure =
      R"({"HLAserialNumber":9,"HLAreturnedArgument":[null],"HLAservice":"FederationSaved","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federation save-success indicator","HLAargumentValue":false},{"HLAargumentType":48,"HLAargumentName":"Optional failure reason","HLAargumentValue":"FEDERATE_REPORTED_FAILURE_DURING_SAVE"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == afterFailureRequest + expectedBegunFailure +
                                       expectedCompleteFailure + expectedFederationSavedFailure);
  REQUIRE(reports.federationNotSavedReasons.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationNotSavedReasons ==
          std::vector<SaveFailureReason>{rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE});

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
