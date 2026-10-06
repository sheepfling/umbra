#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service-report files have one immutable joined-federate lifetime",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withConfigurationName(L"report-file-test").withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-subject", L"observer", federationName));
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());

  auto files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const firstFile = files.front();
  auto const initialText = readTextFile(firstFile);
  REQUIRE_FALSE(initialText.empty());
  REQUIRE(initialText.front() == '{');
  REQUIRE(initialText.find("\"CallbackModel\":\"HLA_EVOKED\"") != std::string::npos);
  REQUIRE(initialText.find("\"ConfigurationName\":\"report-file-test\"") != std::string::npos);
  REQUIRE(initialText.find("\"HLAfederationName\":\"umbra-catch2-federation-") !=
          std::string::npos);
  REQUIRE(initialText.find("\"HLAMIMDesignator\":\"HLAstandardMIM\"") != std::string::npos);
  REQUIRE(initialText.find("\"HLAfederateName\":\"service-report-subject\"") !=
          std::string::npos);
  REQUIRE(initialText.find("\"HLAserialNumber\"") == std::string::npos);

  // File reporting switches gate future report appends only. They cannot
  // rotate or replace the preallocated joined-federate file, even when both
  // switches were disabled at the join that created its initial record.
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  REQUIRE(readTextFile(firstFile) == initialText);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(true));
  auto const firstRecord =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(firstFile) == initialText + firstRecord);
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  REQUIRE(readTextFile(firstFile) == initialText + firstRecord);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->setExceptionReportingSwitch(false));
  auto const secondRecord =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(serviceReportFiles(directory.path()) ==
          std::vector<std::filesystem::path>{firstFile});
  REQUIRE(readTextFile(firstFile) == initialText + firstRecord + secondRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-subject", L"observer", federationName));
  files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  REQUIRE(files.front() != files.back());
  REQUIRE(std::find(files.begin(), files.end(), firstFile) != files.end());
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
