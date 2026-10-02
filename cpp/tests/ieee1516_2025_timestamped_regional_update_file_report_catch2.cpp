#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void runTimestampedRegionalUpdateFileReportScenario(bool disconnectReceiver) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
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
  unsigned char const valueBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-update-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-update-report-receiver", L"subscriber", federationName));

  // Keep declarations, region association, and time-role callbacks outside
  // the accepted timestamped regional update record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  AttributeHandleValueMap const values{{flavor, value}};
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      timestamp);
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  auto asAscii = [](std::wstring const& text) {
    std::string result;
    result.reserve(text.size());
    for (wchar_t const character : text) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const objectValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const timestampValue = asAscii(timestamp.toString());
  auto const retractionValue = asAscii(retraction.toString());
  auto const open = retractionValue.find('(');
  auto const close = retractionValue.find(')');
  REQUIRE(open != std::string::npos);
  REQUIRE(close != std::string::npos);
  REQUIRE(close > open + 1U);
  auto const messageId = retractionValue.substr(open + 1U, close - open - 1U);
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":33,"HLAargumentName":"Message retraction designator","HLAargumentValue":"MessageRetractionHandle<)" +
      messageId +
      R"(>"}],"HLAservice":"UpdateAttributeValues","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":2,"HLAargumentName":"Constrained set of attribute designator and value pairs","HLAargumentValue":{")" +
      attributeValue +
      R"(":"AQI="}},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":")" +
      timestampValue + R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // Regional association affects the callback projection, not the §6.10
  // sender argument list. The accepted file record must still be durable
  // before the constrained reflection callback.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  bool durableAtCallback = false;
  receiverReports.onAttributeReflection = [&] {
    durableAtCallback = readTextFile(reportFile) == initialText + expectedRecord;
  };

  // Do not mix grant-driving time services into this one sender record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(timestamp));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(durableAtCallback);

  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == values.size());
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(reflection.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(reflection.timeValue == timestamp.toString());
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  if (disconnectReceiver) {
    REQUIRE_NOTHROW(receiver->disconnect());
  }
}

TEST_CASE(
    "Embedded service reporting records timestamped regional Update Attribute Values before reflection callback (restored baseline copy)",
    "[integration][development-profile][federation-management][object-management][ddm][time-management]"
    "[mom][service-report-file][service-reporting][timestamped-regional-attribute-update][tso]"
    "[timestamped-regional-attribute-update-service-report]"
    "[rti.service.update-attribute-values][rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  runTimestampedRegionalUpdateFileReportScenario(true);
}

TEST_CASE(
    "Embedded service reporting records timestamped regional Update Attribute Values before reflection callback",
    "[integration][development-profile][federation-management][object-management][ddm][time-management]"
    "[mom][service-report-file][service-reporting][timestamped-regional-attribute-update][tso]"
    "[timestamped-regional-attribute-update-service-report]"
    "[rti.service.update-attribute-values][rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  runTimestampedRegionalUpdateFileReportScenario(false);
}
}  // namespace
