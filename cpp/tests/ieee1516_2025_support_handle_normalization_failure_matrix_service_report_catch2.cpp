#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed handle normalization invocations",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[handle-normalization-service-reports]"
    "[rti.service.handle-normalization-failure-matrix]") {
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
      L"handle-normalization-failure-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Keep setup and the first failure pass out of the report. The second pass
  // proves that every failure appends to the same joined-federate file.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->normalizeServiceGroup(static_cast<ServiceGroup>(99)),
      rti1516_2025::InvalidServiceGroup);
  REQUIRE_THROWS_AS(
      owner->normalizeFederateHandle(FederateHandle{}),
      rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectClassHandle(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeInteractionClassHandle(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectInstanceHandle(ObjectInstanceHandle{}),
      rti1516_2025::InvalidObjectInstanceHandle);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->normalizeServiceGroup(static_cast<ServiceGroup>(99)),
      rti1516_2025::InvalidServiceGroup);
  REQUIRE_THROWS_AS(
      owner->normalizeFederateHandle(FederateHandle{}),
      rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectClassHandle(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeInteractionClassHandle(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectInstanceHandle(ObjectInstanceHandle{}),
      rti1516_2025::InvalidObjectInstanceHandle);
  REQUIRE(owner->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES) ==
          static_cast<unsigned long>(rti1516_2025::SUPPORT_SERVICES));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const failureRecord = [](std::uint32_t serial,
                                std::string const& service,
                                std::int32_t argumentType,
                                std::string const& argumentName,
                                std::string const& argumentValue,
                                std::string const& exception) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serial) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":")" + service +
        R"(","HLAsuppliedArguments":[{"HLAargumentType":)" +
        std::to_string(argumentType) + R"(,"HLAargumentName":")" + argumentName +
        R"(","HLAargumentValue":")" + argumentValue +
        R"("}],"HLAsuccessIndicator":false,"HLAexception":")" + exception + R"("})";
  };
  auto const invalidFederate = asAscii(FederateHandle{}.toString());
  auto const invalidObjectClass = asAscii(ObjectClassHandle{}.toString());
  auto const invalidInteractionClass = asAscii(InteractionClassHandle{}.toString());
  auto const invalidObjectInstance = asAscii(ObjectInstanceHandle{}.toString());
  auto const expectedRecords =
      failureRecord(
          0U,
          "NormalizeServiceGroup",
          50,
          "Service group indicator",
          "UNSUPPORTED",
          "InvalidServiceGroup: Normalize Service Group requires a supported ServiceGroup indicator.") +
      failureRecord(
          1U,
          "NormalizeFederateHandle",
          15,
          "Federate handle",
          invalidFederate,
          "InvalidFederateHandle: Normalize Federate Handle requires a valid FederateHandle.") +
      failureRecord(
          2U,
          "NormalizeObjectClassHandle",
          36,
          "Object class handle",
          invalidObjectClass,
          "InvalidObjectClassHandle: Normalize Object Class Handle requires a valid ObjectClassHandle.") +
      failureRecord(
          3U,
          "NormalizeInteractionClassHandle",
          27,
          "Interaction class handle",
          invalidInteractionClass,
          "InvalidInteractionClassHandle: Normalize Interaction Class Handle requires a valid InteractionClassHandle.") +
      failureRecord(
          4U,
          "NormalizeObjectInstanceHandle",
          37,
          "Object instance handle",
          invalidObjectInstance,
          "InvalidObjectInstanceHandle: Normalize Object Instance Handle requires a valid ObjectInstanceHandle.") +
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Normalized value","HLAargumentValue":6}],"HLAservice":"NormalizeServiceGroup","HLAsuppliedArguments":[{"HLAargumentType":50,"HLAargumentName":"Service group indicator","HLAargumentValue":"SUPPORT_SERVICES"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
