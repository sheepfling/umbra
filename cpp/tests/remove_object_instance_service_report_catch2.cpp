#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded service reporting records receive-order Remove Object Instance before its callback",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[mom][service-report-file][service-reporting]"
    "[remove-object-instance-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.remove-object-instance][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  auto subscriberConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");
  subscriberConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED, subscriberConfiguration));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"remove-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"remove-report-subscriber", L"subscriber", federationName));

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
  auto const subscriberReportFile = reportFileFor("remove-report-subscriber");

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
  auto const removalRecord = [](std::uint32_t serialNumber,
                                std::string const& objectInstance,
                                std::string const& producingFederate) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"RemoveObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        objectInstance + R"("},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVw"},{"HLAargumentType":38,"HLAargumentName":"Sent message order type","HLAargumentValue":"RECEIVE"},{"HLAargumentType":15,"HLAargumentName":"Producing joined federate designator","HLAargumentValue":")" +
        producingFederate + R"("},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null},{"HLAargumentType":34,"HLAargumentName":"Optional receive message order type","HLAargumentValue":null},{"HLAargumentType":34,"HLAargumentName":"Optional message retraction designator","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  auto const employee = publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const name = publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(employee));
  while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  auto const subscriberBeforeRemoval = readTextFile(subscriberReportFile);
  auto const expectedRecord = removalRecord(
      2U,
      asAscii(objectInstance.toString()),
      asAscii(publisherHandle.toString()));

  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  VariableLengthData const deletionTag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, deletionTag));

  // HLA_EVOKED keeps the planned recipient service out of the file until its
  // callback-time removal transition succeeds. At that boundary, it must be
  // durable before FederateAmbassador::removeObjectInstance enters user code.
  REQUIRE(subscriberReports.objectRemovalReports.empty());
  REQUIRE(readTextFile(subscriberReportFile) == subscriberBeforeRemoval);
  bool reportPresentAtCallbackEntry = false;
  subscriberReports.onRemoveObjectInstance = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(subscriberReportFile) == subscriberBeforeRemoval + expectedRecord;
  };
  while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(subscriberReports.objectRemovalReports.size() == 1U);
  auto const& removal = subscriberReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>{tagBytes[0], tagBytes[1], tagBytes[2]});
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(readTextFile(subscriberReportFile) == subscriberBeforeRemoval + expectedRecord);

  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
