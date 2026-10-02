#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Subscribe Object Class Attributes arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]") {
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
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"subscribe-object-class-report-subject", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated object-attribute subscription failure matrix owns
  // failed-service records; keep this argument regression focused on accepted
  // forms.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->subscribeObjectClassAttributes(unknownObjectClass, AttributeHandleSet{}, false, L"High"),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  auto const objectClassText = objectClass.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
  }
  auto const attributeText = attribute.toString();
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }

  // The C++ `active == false` selector maps to the service's Optional passive
  // subscription indicator == true. `High` is a real Restaurant FOM update
  // rate, so this first record exercises the non-null String representation.
  REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(objectClass, attributes, false, L"High"));
  auto const explicitRateRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SubscribeObjectClassAttributes","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":true},{"HLAargumentType":53,"HLAargumentName":"Optional update rate designator","HLAargumentValue":"High"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + explicitRateRecord);

  // An empty C++ update-rate designator selects the default rate. §11.5.1 and
  // the standard MIM require the optional argument's Table 5 slot to use Null.
  REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(objectClass, attributes));
  auto const defaultRateRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SubscribeObjectClassAttributes","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":false},{"HLAargumentType":34,"HLAargumentName":"Optional update rate designator","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + explicitRateRecord + defaultRateRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + explicitRateRecord + defaultRateRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
