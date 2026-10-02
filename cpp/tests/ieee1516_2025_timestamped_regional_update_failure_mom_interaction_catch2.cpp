#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void runTimestampedRegionalUpdateFailureMomInteractionScenario() {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  unsigned char const valueBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-update-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-regional-update-interaction-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber"}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
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
  auto const observerRegion = observer->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(observer->setRangeBounds(
      observerRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(observer->commitRegionModifications(RegionHandleSet{observerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const observerPair{{
      flavorOnly,
      RegionHandleSet{observerRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributesWithRegions(
      soda,
      observerPair));
  REQUIRE_NOTHROW(observer->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE(observerReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& objectValue,
                                std::wstring const& mapValue,
                                std::wstring const& timestampValue,
                                std::wstring const& exception) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == L"UpdateAttributeValues");
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 2);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE_FALSE(decodedSuccess.get());
    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 4U);
    auto const verifyArgument = [&](std::size_t argumentIndex,
                                    std::int32_t type,
                                    std::wstring const& name,
                                    std::wstring const& valueText) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == valueText);
    };
    verifyArgument(0U, 37, L"Object instance designator", L"\"" + objectValue + L"\"");
    verifyArgument(1U, 2, L"Constrained set of attribute designator and value pairs", mapValue);
    verifyArgument(2U, 60, L"User-supplied tag", L"\"cmVn\"");
    verifyArgument(3U, 31, L"Optional timestamp", L"\"" + timestampValue + L"\"");
    rti1516_2025::HLAfixedRecord nullReturned;
    nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() == L"null");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get() == exception);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  AttributeHandleValueMap const values{{flavor, value}};
  AttributeHandleValueMap const invalidValues{{AttributeHandle{}, value}};
  auto const objectValue = objectInstance.toString();
  auto const invalidObjectValue = ObjectInstanceHandle{}.toString();
  auto const mapValue = L"{\"" + flavor.toString() + L"\":\"AQI=\"}";
  auto const invalidMapValue = L"{\"" + AttributeHandle{}.toString() + L"\":\"AQI=\"}";
  auto const unknownObjectException =
      L"ObjectInstanceNotKnown: Timestamped Update Attribute Values requires a known ObjectInstanceHandle.";
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          ObjectInstanceHandle{},
          values,
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  verifyReport(0U, invalidObjectValue, mapValue, L"6", unknownObjectException);
  auto const invalidAttributeException =
      L"AttributeNotDefined: Timestamped Update Attribute Values requires defined AttributeHandle values.";
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          invalidValues,
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::AttributeNotDefined);
  verifyReport(1U, objectValue, invalidMapValue, L"6", invalidAttributeException);
  auto const invalidTimeException =
      L"InvalidLogicalTime: A timestamped service is earlier than the sender's current logical time plus lookahead.";
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  verifyReport(2U, objectValue, mapValue, L"4", invalidTimeException);
  REQUIRE(observerReports.interactionReports.size() == 3U);

  REQUIRE_NOTHROW(observer->unsubscribeObjectClassAttributesWithRegions(soda, observerPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(observer->deleteRegion(observerRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction",
    "[integration][development-profile][federation-management][object-management][ddm][time-management]"
    "[mom][service-reporting][service-report-interaction][service-failure][tso]"
    "[timestamped-regional-attribute-update-failure]"
    "[rti.service.timestamped-regional-attribute-update-failure-matrix-interaction]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]") {
  runTimestampedRegionalUpdateFailureMomInteractionScenario();
}

TEST_CASE(
    "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction (restored baseline copy)",
    "[integration][development-profile][federation-management][object-management][ddm][time-management]"
    "[mom][service-reporting][service-report-interaction][service-failure][tso]"
    "[timestamped-regional-attribute-update-failure]"
    "[rti.service.timestamped-regional-attribute-update-failure-matrix-interaction]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions][restored-baseline-copy]") {
  runTimestampedRegionalUpdateFailureMomInteractionScenario();
}

}  // namespace
