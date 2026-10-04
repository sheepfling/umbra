#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting preserves Request Interaction Transportation Type Change arguments",
    "[integration][development-profile][federation-management][interaction-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[service-report-request-interaction-transportation-type-change]"
    "[request-interaction-transportation-type-change]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[federate.callback.confirm-interaction-transportation-type-change]") {
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
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"interaction-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const bestEffort = rti->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(interactionClass.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  // An unsuccessful §6.30 invocation has no successful-void report record.
  REQUIRE_THROWS_AS(
      rti->requestInteractionTransportationTypeChange(interactionClass, bestEffort),
      rti1516_2025::InteractionClassNotPublished);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(
      rti->requestInteractionTransportationTypeChange(interactionClass, bestEffort));
  auto const interactionClassText = interactionClass.toString();
  auto const transportationText = bestEffort.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  std::string transportationValue;
  transportationValue.reserve(transportationText.size());
  for (wchar_t const character : transportationText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    transportationValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"RequestInteractionTransportationTypeChange","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":59,"HLAargumentName":"Transportation type","HLAargumentValue":")" +
      transportationValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  auto const expectedPublishRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The report describes accepted request invocation. The preferred
  // transportation does not change until the separately queued confirmation.
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);
  REQUIRE(reports.interactionTransportationTypeChangeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.interactionTransportationTypeChangeReports.size() == 1U);
  REQUIRE(reports.interactionTransportationTypeChangeReports.front().interactionClass ==
          interactionClass);
  REQUIRE(reports.interactionTransportationTypeChangeReports.front().transportationType ==
          bestEffort);
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
