#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records the seven object and update-rate lookup failures",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.lookup-failure-matrix]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"lookup-failure-matrix-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(L"lookup-failure-matrix-object"));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  auto const object = owner->registerObjectInstance(server, L"lookup-failure-matrix-object");
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));

  REQUIRE_THROWS_AS(
      owner->getKnownObjectClassHandle(ObjectInstanceHandle{}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->getObjectInstanceHandle(L"lookup-failure-matrix-missing"),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->getObjectInstanceName(ObjectInstanceHandle{}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(ObjectClassHandle{}, fixture_hla::fixture::efficiency),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(server, AttributeHandle{}),
      rti1516_2025::InvalidAttributeHandle);
  REQUIRE_THROWS_AS(
      owner->getUpdateRateValue(L"lookup-failure-matrix-missing-rate"),
      rti1516_2025::InvalidUpdateRateDesignator);
  REQUIRE_THROWS_AS(
      owner->getUpdateRateValueForAttribute(ObjectInstanceHandle{}, name),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(owner->getObjectClassHandle(fixture_hla::fom::employee_server) == server);

  auto const reportText = readTextFile(reportFile);
  REQUIRE(reportText.starts_with(initialText));
  for (std::uint32_t serial = 0U; serial <= 7U; ++serial) {
    REQUIRE(reportText.find(
                "\"HLAserialNumber\":" + std::to_string(serial)) != std::string::npos);
  }
  auto countOccurrences = [](std::string const& text, std::string const& needle) {
    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = text.find(needle, offset)) != std::string::npos) {
      ++count;
      offset += needle.size();
    }
    return count;
  };
  REQUIRE(countOccurrences(reportText, R"("HLAsuccessIndicator":false)") == 7U);
  REQUIRE(countOccurrences(reportText, R"("HLAreturnedArgument":[null])") == 7U);
  for (auto const& service : {
           "GetKnownObjectClassHandle",
           "GetObjectInstanceHandle",
           "GetObjectInstanceName",
           "GetAttributeHandle",
           "GetAttributeName",
           "GetUpdateRateValue",
           "GetUpdateRateValueForAttribute"}) {
    REQUIRE(reportText.find("\"HLAservice\":\"" + std::string(service) + "\"") !=
            std::string::npos);
  }
  REQUIRE(reportText.find(
              R"("HLAexception":"ObjectInstanceNotKnown: The supplied ObjectInstanceHandle is not known to this federate.")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAexception":"ObjectInstanceNotKnown: The supplied object instance name is not known to this federate.")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAexception":"InvalidObjectClassHandle: Get Attribute Handle requires a valid ObjectClassHandle.")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAexception":"InvalidAttributeHandle: Get Attribute Name requires a valid AttributeHandle.")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAexception":"InvalidUpdateRateDesignator: The supplied update-rate designator is not defined by the current FDD.")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAexception":"ObjectInstanceNotKnown: Get Update Rate Value For Attribute requires a known ObjectInstanceHandle.")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAservice":"GetObjectClassHandle","HLAsuppliedArguments")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAargumentName":"Object instance name","HLAargumentValue":"lookup-failure-matrix-missing")") !=
          std::string::npos);
  REQUIRE(reportText.find(
              R"("HLAargumentName":"Update rate name","HLAargumentValue":"lookup-failure-matrix-missing-rate")") !=
          std::string::npos);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
