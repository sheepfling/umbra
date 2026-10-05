#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting preserves Query Interaction Transportation Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[service-report-query-interaction-transportation-type]"
    "[query-interaction-transportation-type]"
    "[rti.service.query-interaction-transportation-type]"
    "[federate.callback.report-interaction-transportation-type]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle joinedFederate;
  REQUIRE_NOTHROW(joinedFederate = rti->joinFederationExecution(
      L"query-interaction-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass =
      rti->getInteractionClassHandle(fixture_hla::fom::server_take_order);
  auto const reliable = rti->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(joinedFederate.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.32 invocation has no successful-void report record.
  FederateHandle const invalidFederate;
  REQUIRE_THROWS_AS(
      rti->queryInteractionTransportationType(invalidFederate, interactionClass),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(
      rti->queryInteractionTransportationType(joinedFederate, interactionClass));

  auto const federateText = joinedFederate.toString();
  auto const interactionClassText = interactionClass.toString();
  std::string federateValue;
  federateValue.reserve(federateText.size());
  for (wchar_t const character : federateText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    federateValue.push_back(static_cast<char>(character));
  }
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"QueryInteractionTransportationType","HLAsuppliedArguments":[{"HLAargumentType":15,"HLAargumentName":"Federate designator","HLAargumentValue":")" +
      federateValue +
      R"("},{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The service report records accepted query invocation; the separate Report
  // Interaction Transportation Type callback remains later work.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(reports.interactionTransportationTypeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.interactionTransportationTypeReports.size() == 1U);
  REQUIRE(reports.interactionTransportationTypeReports.front().federate == joinedFederate);
  REQUIRE(
      reports.interactionTransportationTypeReports.front().interactionClass == interactionClass);
  REQUIRE(
      reports.interactionTransportationTypeReports.front().transportationType == reliable);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
