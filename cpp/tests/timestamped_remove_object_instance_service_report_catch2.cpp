#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded service reporting records timestamped Remove Object Instance before immediate and TSO callbacks",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[callbacks][mom][service-report-file][service-reporting]"
    "[timestamped-remove-object-instance-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.delete-object-instance]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.remove-object-instance][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  auto immediateConfiguration = configurationForServiceReportDirectory(directory.path());
  auto constrainedConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");
  immediateConfiguration.withRtiAddress(L"in-process");
  constrainedConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED, immediateConfiguration));
  REQUIRE_NOTHROW(constrained->connect(
      constrainedReports, HLA_EVOKED, constrainedConfiguration));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"timestamped-remove-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-remove-report-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timestamped-remove-report-constrained", L"subscriber", federationName));

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
  auto const immediateReportFile = reportFileFor("timestamped-remove-report-immediate");
  auto const constrainedReportFile = reportFileFor("timestamped-remove-report-constrained");

  // Keep setup out of both recipient files. The support FOM starts these
  // switches enabled; their setters deliberately do not report themselves.
  REQUIRE_NOTHROW(immediate->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(immediate->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(constrained->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(constrained->setSendServiceReportsToFileSwitch(false));

  auto const employee = publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const name = publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(employee));
  while (immediate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  while (constrained->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  while (constrained->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(constrainedReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  // Selection begins immediately before the two callback paths under test.
  // Their files have no report serials to inherit from setup work.
  REQUIRE_NOTHROW(immediate->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(immediate->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(constrained->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(constrained->setSendServiceReportsToFileSwitch(true));
  auto const immediateBeforeRemoval = readTextFile(immediateReportFile);
  auto const constrainedBeforeRemoval = readTextFile(constrainedReportFile);

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
                                std::string const& producingFederate,
                                std::string const& receivedOrderType,
                                std::string const& messageId) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"RemoveObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        objectInstance + R"("},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVw"},{"HLAargumentType":38,"HLAargumentName":"Sent message order type","HLAargumentValue":"TIMESTAMP"},{"HLAargumentType":15,"HLAargumentName":"Producing joined federate designator","HLAargumentValue":")" +
        producingFederate + R"("},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":"6"},{"HLAargumentType":38,"HLAargumentName":"Optional receive message order type","HLAargumentValue":")" +
        receivedOrderType + R"("},{"HLAargumentType":33,"HLAargumentName":"Optional message retraction designator","HLAargumentValue":"MessageRetractionHandle<)" +
        messageId + R"(>"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };
  auto const timeAdvanceRequestRecord =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"TimeAdvanceRequest","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"6"}],"HLAsuccessIndicator":true,"HLAexception":null})";

  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  VariableLengthData const deletionTag(tagBytes, sizeof(tagBytes));
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      deletionTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());

  auto const handleText = retraction.toString();
  auto const opening = handleText.find(L'(');
  auto const closing = handleText.find(L')', opening);
  REQUIRE(opening != std::wstring::npos);
  REQUIRE(closing == handleText.size() - 1U);
  auto const messageIdText = handleText.substr(opening + 1U, closing - opening - 1U);
  REQUIRE_FALSE(messageIdText.empty());
  std::string messageId;
  messageId.reserve(messageIdText.size());
  for (wchar_t const character : messageIdText) {
    REQUIRE(character >= L'0');
    REQUIRE(character <= L'9');
    messageId.push_back(static_cast<char>(character));
  }

  auto const objectInstanceText = asAscii(objectInstance.toString());
  auto const publisherText = asAscii(publisherHandle.toString());
  auto const immediateExpected = removalRecord(
      0U, objectInstanceText, publisherText, "RECEIVE", messageId);
  auto const constrainedExpected = removalRecord(
      1U, objectInstanceText, publisherText, "TIMESTAMP", messageId);

  // HLA_EVOKED retains both delivery paths until their true callback
  // boundary. The non-time-constrained recipient therefore has no durable
  // removal record immediately after the timestamped Delete invocation.
  REQUIRE(immediateReports.objectRemovalReports.empty());
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE(readTextFile(immediateReportFile) == immediateBeforeRemoval);
  REQUIRE(readTextFile(constrainedReportFile) == constrainedBeforeRemoval);
  bool immediateReportPresentAtCallbackEntry = false;
  immediateReports.onTimestampedObjectRemoval = [&] {
    immediateReportPresentAtCallbackEntry =
        readTextFile(immediateReportFile) == immediateBeforeRemoval + immediateExpected;
  };
  while (immediate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(immediateReportPresentAtCallbackEntry);
  REQUIRE(immediateReports.objectRemovalReports.size() == 1U);
  auto const& immediateRemoval = immediateReports.objectRemovalReports.front();
  REQUIRE(immediateRemoval.objectInstance == objectInstance);
  REQUIRE(immediateRemoval.producingFederate == publisherHandle);
  REQUIRE(immediateRemoval.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(immediateRemoval.timeValue == L"6");
  REQUIRE(immediateRemoval.sentOrderType == TIMESTAMP);
  REQUIRE(immediateRemoval.receivedOrderType == RECEIVE);
  REQUIRE(immediateRemoval.retractionSupplied);
  REQUIRE(immediateRemoval.retractionValid);
  REQUIRE(readTextFile(immediateReportFile) == immediateBeforeRemoval + immediateExpected);

  // Time Advance Request is a separate accepted recipient service, so it
  // takes serial zero before the TSO callback's serial-one §6.17 record.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(readTextFile(constrainedReportFile) ==
          constrainedBeforeRemoval + timeAdvanceRequestRecord);
  bool constrainedReportPresentAtCallbackEntry = false;
  constrainedReports.onTimestampedObjectRemoval = [&] {
    constrainedReportPresentAtCallbackEntry =
        readTextFile(constrainedReportFile) ==
        constrainedBeforeRemoval + timeAdvanceRequestRecord + constrainedExpected;
  };
  // The regulating federate advances its current time to make the
  // timestamp-six delivery eligible. The constrained callback runs before
  // the recipient's matching grant.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  while (constrained->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(constrainedReportPresentAtCallbackEntry);
  REQUIRE(constrainedReports.objectRemovalReports.size() == 1U);
  auto const& constrainedRemoval = constrainedReports.objectRemovalReports.front();
  REQUIRE(constrainedRemoval.objectInstance == objectInstance);
  REQUIRE(constrainedRemoval.producingFederate == publisherHandle);
  REQUIRE(constrainedRemoval.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(constrainedRemoval.timeValue == L"6");
  REQUIRE(constrainedRemoval.sentOrderType == TIMESTAMP);
  REQUIRE(constrainedRemoval.receivedOrderType == TIMESTAMP);
  REQUIRE(constrainedRemoval.retractionSupplied);
  REQUIRE(constrainedRemoval.retractionValid);
  REQUIRE(readTextFile(constrainedReportFile) ==
          constrainedBeforeRemoval + timeAdvanceRequestRecord + constrainedExpected);

  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
