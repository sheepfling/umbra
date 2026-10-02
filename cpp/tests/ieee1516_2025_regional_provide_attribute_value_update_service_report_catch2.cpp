#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void runRegionalProvideAttributeValueUpdateServiceReportScenario() {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto ownerConfiguration = configurationForServiceReportDirectory(directory.path());
  auto requesterConfiguration = configurationForServiceReportDirectory(directory.path());
  ownerConfiguration.withRtiAddress(L"in-process");
  requesterConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, ownerConfiguration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, requesterConfiguration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-provide-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-provide-report-requester", L"requester", federationName));

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
  auto const ownerReportFile = reportFileFor("regional-provide-report-owner");
  auto const requesterReportFile = reportFileFor("regional-provide-report-requester");

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));
  auto const ownerInitialText = readTextFile(ownerReportFile);
  auto const requesterInitialText = readTextFile(requesterReportFile);

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const requestRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const requestPair{{
      flavorOnly,
      RegionHandleSet{requestRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE(readTextFile(ownerReportFile) == ownerInitialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(true));
  auto const ownerBeforeRequest = readTextFile(ownerReportFile);
  auto const requesterBeforeRequest = readTextFile(requesterReportFile);
  REQUIRE(ownerBeforeRequest == ownerInitialText);
  REQUIRE(requesterBeforeRequest == requesterInitialText);

  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const requestTag(tagBytes, sizeof(tagBytes));
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
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ProvideAttributeValueUpdate","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      asAscii(objectInstance.toString()) +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      asAscii(flavor.toString()) +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVn"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  auto const requesterRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"RequestAttributeValueUpdateWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      asAscii(soda.toString()) +
      R"("},{"HLAargumentType":4,"HLAargumentName":"Collection of attribute designator set and region designator set pairs","HLAargumentValue":[{"attributeHandleSet":[")" +
      asAscii(flavor.toString()) +
      R"("],"regionHandleSet":[")" +
      asAscii(requestRegion.toString()) +
      R"("]}]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVn"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  bool reportPresentAtCallbackEntry = false;
  ownerReports.onProvideAttributeValueUpdate = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(ownerReportFile) == ownerBeforeRequest + expectedRecord;
  };

  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda, requestPair, requestTag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeRequest);
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeRequest + requesterRecord);
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
  auto const& provide = ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(provide.objectInstance == objectInstance);
  REQUIRE(provide.attributes == flavorOnly);
  REQUIRE(variableLengthDataBytes(provide.userSuppliedTag) ==
          std::vector<unsigned char>{tagBytes[0], tagBytes[1], tagBytes[2]});
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeRequest + expectedRecord);
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeRequest + requesterRecord);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(requestRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

TEST_CASE(
    "Embedded regional Provide Attribute Value Update reports before callback delivery",
    "[integration][development-profile][federation-management][object-management][ddm][callbacks]"
    "[mom][service-report-file][service-reporting]"
    "[provide-attribute-value-update-regional-service-report]"
    "[regional-attribute-value-update-request-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]"
    "[regional-provide-attribute-value-update-service-report]") {
  runRegionalProvideAttributeValueUpdateServiceReportScenario();
}

TEST_CASE(
    "Embedded regional Provide Attribute Value Update reports before callback delivery (restored baseline copy)",
    "[integration][development-profile][federation-management][object-management][ddm][callbacks]"
    "[mom][service-report-file][service-reporting]"
    "[provide-attribute-value-update-regional-service-report]"
    "[regional-attribute-value-update-request-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]"
    "[regional-provide-attribute-value-update-restored-baseline]") {
  runRegionalProvideAttributeValueUpdateServiceReportScenario();
}
}  // namespace
