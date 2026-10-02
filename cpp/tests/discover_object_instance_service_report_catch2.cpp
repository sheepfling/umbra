#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded service reporting records Discover Object Instance before its callback",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[mom][service-report-file][service-reporting]"
    "[discover-object-instance-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.discover-object-instance][2025]") {
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
          L"discover-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"discover-report-subscriber", L"subscriber", federationName));

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
  auto const subscriberReportFile = reportFileFor("discover-report-subscriber");

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
  auto const discoveryRecord = [](std::uint32_t serialNumber,
                                  std::string const& objectInstance,
                                  std::string const& objectClass,
                                  std::string const& objectInstanceName,
                                  std::string const& producingFederate) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"DiscoverObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance handle","HLAargumentValue":")" +
        objectInstance + R"("},{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
        objectClass + R"("},{"HLAargumentType":53,"HLAargumentName":"Object instance name","HLAargumentValue":")" +
        objectInstanceName + R"("},{"HLAargumentType":15,"HLAargumentName":"Producing joined federate designator","HLAargumentValue":")" +
        producingFederate + R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  auto const employee = publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const name = publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  auto const subscriberBeforeDiscovery = readTextFile(subscriberReportFile);

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(employee));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  auto const expectedRecord = discoveryRecord(
      1U,
      asAscii(objectInstance.toString()),
      asAscii(employee.toString()),
      asAscii(objectInstanceName),
      asAscii(publisherHandle.toString()));

  // HLA_EVOKED leaves the planned service entirely invisible until callback
  // delivery. At that delivery boundary, its selected-file append must occur
  // before user callback code can observe Discover Object Instance.
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());
  REQUIRE(readTextFile(subscriberReportFile) == subscriberBeforeDiscovery);
  bool reportPresentAtCallbackEntry = false;
  subscriberReports.onDiscoverObjectInstance = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(subscriberReportFile) == subscriberBeforeDiscovery + expectedRecord;
  };
  while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  auto const& discovery = subscriberReports.objectDiscoveryReports.front();
  REQUIRE(discovery.objectInstance == objectInstance);
  REQUIRE(discovery.objectClass == employee);
  REQUIRE(discovery.objectInstanceName == objectInstanceName);
  REQUIRE(discovery.producingFederate == publisherHandle);
  REQUIRE(readTextFile(subscriberReportFile) == subscriberBeforeDiscovery + expectedRecord);

  // A planned but later-ineligible HLA_EVOKED discovery must remain absent
  // from the report file too. The accepted unsubscribe has its own direct
  // record; delivery of this second object must add neither a serial nor a
  // Discover Object Instance callback after the callback-time recheck.
  ObjectInstanceHandle cancelledObjectInstance;
  REQUIRE_NOTHROW(cancelledObjectInstance = publisher->registerObjectInstance(employee));
  REQUIRE(cancelledObjectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  auto const subscriberBeforeCancelledDiscovery = readTextFile(subscriberReportFile);
  while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(readTextFile(subscriberReportFile) == subscriberBeforeCancelledDiscovery);

  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
