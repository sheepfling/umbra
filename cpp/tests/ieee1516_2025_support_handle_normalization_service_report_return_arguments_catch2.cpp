#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records handle normalization return arguments",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-report-file][service-reporting][handle-normalization-service-reports]"
    "[rti.service.normalize-service-group][rti.service.normalize-federate-handle]"
    "[rti.service.normalize-object-class-handle]"
    "[rti.service.normalize-interaction-class-handle]"
    "[rti.service.normalize-object-instance-handle]") {
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
  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
                      L"handle-normalization-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, {efficiency}));
  auto const takeOrder = owner->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const objectInstance = owner->registerObjectInstance(server);
  auto const serviceGroupValue = owner->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES);
  auto const federateValue = owner->normalizeFederateHandle(ownerFederate);
  auto const objectClassValue = owner->normalizeObjectClassHandle(server);
  auto const interactionClassValue = owner->normalizeInteractionClassHandle(takeOrder);
  auto const objectInstanceValue = owner->normalizeObjectInstanceHandle(objectInstance);
  REQUIRE(serviceGroupValue == static_cast<unsigned long>(rti1516_2025::SUPPORT_SERVICES));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(owner->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES) == serviceGroupValue);
  REQUIRE(owner->normalizeFederateHandle(ownerFederate) == federateValue);
  REQUIRE(owner->normalizeObjectClassHandle(server) == objectClassValue);
  REQUIRE(owner->normalizeInteractionClassHandle(takeOrder) == interactionClassValue);
  REQUIRE(owner->normalizeObjectInstanceHandle(objectInstance) == objectInstanceValue);

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
  auto const ownerFederateText = asAscii(ownerFederate.toString());
  auto const serverText = asAscii(server.toString());
  auto const takeOrderText = asAscii(takeOrder.toString());
  auto const objectInstanceText = asAscii(objectInstance.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Normalized value","HLAargumentValue":)" +
      std::to_string(serviceGroupValue) +
      R"(}],"HLAservice":"NormalizeServiceGroup","HLAsuppliedArguments":[{"HLAargumentType":50,"HLAargumentName":"Service group indicator","HLAargumentValue":"SUPPORT_SERVICES"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Normalized value","HLAargumentValue":)" +
      std::to_string(federateValue) +
      R"(}],"HLAservice":"NormalizeFederateHandle","HLAsuppliedArguments":[{"HLAargumentType":15,"HLAargumentName":"Federate handle","HLAargumentValue":")" +
      ownerFederateText +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Normalized value","HLAargumentValue":)" +
      std::to_string(objectClassValue) +
      R"(}],"HLAservice":"NormalizeObjectClassHandle","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      serverText +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Normalized value","HLAargumentValue":)" +
      std::to_string(interactionClassValue) +
      R"(}],"HLAservice":"NormalizeInteractionClassHandle","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      takeOrderText +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Normalized value","HLAargumentValue":)" +
      std::to_string(objectInstanceValue) +
      R"(}],"HLAservice":"NormalizeObjectInstanceHandle","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance handle","HLAargumentValue":")" +
      objectInstanceText +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
