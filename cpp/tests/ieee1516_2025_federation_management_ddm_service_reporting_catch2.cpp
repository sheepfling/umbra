#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded service reporting records Commit Region Modifications arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[commit-region-modifications-service-report]"
    "[region-handle-set-array-encoding]"
    "[rti.service.commit-region-modifications]") {
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
      L"commit-region-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Setup mutations are intentionally outside the report lane. The selected
  // file and its serial sequence remain fixed while both switches are gated.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const region = owner->createRegion(DimensionHandleSet{barQuantity});
  auto const additionalRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE(region.isValid());
  REQUIRE(additionalRegion.isValid());
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(owner->setRangeBounds(
      additionalRegion, barQuantity, RangeBounds(0UL, 10UL)));
  RegionHandleSet const regions{region, additionalRegion};
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(owner->commitRegionModifications(regions));

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
  std::string regionValues{"["};
  bool firstRegion = true;
  for (auto const& regionHandle : regions) {
    if (!firstRegion) {
      regionValues.push_back(',');
    }
    firstRegion = false;
    regionValues.push_back('"');
    regionValues += asAscii(regionHandle.toString());
    regionValues.push_back('"');
  }
  regionValues.push_back(']');
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":)" +
      regionValues +
      R"(}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  auto const invalidCommitRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      asAscii(RegionHandle{}.toString()) +
      R"("]}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Commit Region Modifications requires valid RegionHandle values."})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord + invalidCommitRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records regional object-attribute association arguments",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[regional-object-attribute-association-service-report][associate-regions-for-updates-service-report-file]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]") {
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
      L"regional-association-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Build the object/region state outside the reporting lane. This keeps the
  // first serial tied to AssociateRegionsForUpdates rather than setup calls.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));

  auto const disjointRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      disjointRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{disjointRegion}));
  AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
      flavorOnly,
      RegionHandleSet{disjointRegion},
  }};

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, disjointPair));

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
  auto const objectValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const regionValue = asAscii(disjointRegion.toString());
  auto const associationArgumentName =
      std::string{"Collection of attribute designator set and region designator set pairs"};
  auto const associateRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"AssociateRegionsForUpdates","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + associateRecord);

  // This legacy success-path case suppresses the rejected call; the paired
  // failure matrix below verifies its Null/false/exception record.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(
          objectInstance,
          AttributeHandleSetRegionHandleSetPairVector{{
              flavorOnly,
              RegionHandleSet{RegionHandle{}},
          }}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText + associateRecord);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, disjointPair));
  auto const unassociateRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"UnassociateRegionsForUpdates","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + associateRecord + unassociateRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records all Register Object Instance overloads",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[register-object-instance-service-report]"
    "[rti.service.register-object-instance]"
    "[rti.service.register-object-instance-named]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.register-object-instance-with-regions-named]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, serviceReportFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"register-object-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Resolve all FOM handles, publication, reservations, and region state
  // while reporting is gated. The first selected serial is therefore the
  // first accepted Register Object Instance overload below.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const namedObjectName = std::wstring{L"Umbra.RegisteredNamed"};
  auto const namedRegionalObjectName = std::wstring{L"Umbra.RegisteredRegionalNamed"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(namedObjectName));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(namedRegionalObjectName));

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  ObjectInstanceHandle unnamedObject;
  ObjectInstanceHandle namedObject;
  ObjectInstanceHandle regionalObject;
  ObjectInstanceHandle namedRegionalObject;
  REQUIRE_NOTHROW(unnamedObject = owner->registerObjectInstance(soda));
  REQUIRE_NOTHROW(namedObject = owner->registerObjectInstance(soda, namedObjectName));
  REQUIRE_NOTHROW(
      regionalObject = owner->registerObjectInstanceWithRegions(soda, regionalPair));
  REQUIRE_NOTHROW(namedRegionalObject = owner->registerObjectInstanceWithRegions(
      soda, regionalPair, namedRegionalObjectName));

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
  auto const classValue = asAscii(soda.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const regionValue = asAscii(region.toString());
  auto const unnamedObjectValue = asAscii(unnamedObject.toString());
  auto const namedObjectValue = asAscii(namedObject.toString());
  auto const regionalObjectValue = asAscii(regionalObject.toString());
  auto const namedRegionalObjectValue = asAscii(namedRegionalObject.toString());
  auto const namedObjectNameValue = asAscii(namedObjectName);
  auto const namedRegionalObjectNameValue = asAscii(namedRegionalObjectName);

  auto const classArgument = [&classValue] {
    return std::string{
        R"({"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")"} +
        classValue + R"("})";
  };
  auto const regionalArgument = [&attributeValue, &regionValue] {
    return std::string{
        R"({"HLAargumentType":4,"HLAargumentName":"Collection of attribute designator set and region designator set pairs","HLAargumentValue":[{"attributeHandleSet":[")"} +
        attributeValue + R"("],"regionHandleSet":[")" + regionValue + R"("]}]})";
  };
  auto const namedArgument = [](std::string const& value) {
    return std::string{
        R"({"HLAargumentType":53,"HLAargumentName":"Object instance name","HLAargumentValue":")"} +
        value + R"("})";
  };
  auto const successRecord = [](std::uint32_t serial,
                                std::string const& service,
                                std::string const& supplied,
                                std::string const& objectValue) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        R"(,"HLAreturnedArgument":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        objectValue + R"("}],"HLAservice":")" + service +
        R"(","HLAsuppliedArguments":[)" + supplied +
        R"(],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  auto const registerRecord = successRecord(
      0U, "RegisterObjectInstance", classArgument(), unnamedObjectValue);
  auto const namedRegisterRecord = successRecord(
      1U,
      "RegisterObjectInstance",
      classArgument() + "," + namedArgument(namedObjectNameValue),
      namedObjectValue);
  auto const regionalRegisterRecord = successRecord(
      2U,
      "RegisterObjectInstanceWithRegions",
      classArgument() + "," + regionalArgument(),
      regionalObjectValue);
  auto const namedRegionalRegisterRecord = successRecord(
      3U,
      "RegisterObjectInstanceWithRegions",
      classArgument() + "," + regionalArgument() + "," +
          namedArgument(namedRegionalObjectNameValue),
      namedRegionalObjectValue);
  REQUIRE(readTextFile(reportFile) ==
          initialText + registerRecord + namedRegisterRecord + regionalRegisterRecord +
              namedRegionalRegisterRecord);

  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(unknownObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  auto const invalidClassValue = asAscii(unknownObjectClass.toString());
  auto const failedRecord = std::string{
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"RegisterObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")"} +
      invalidClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"ObjectClassNotDefined: Register Object Instance requires a defined ObjectClassHandle."})";
  REQUIRE(readTextFile(reportFile) ==
          initialText + registerRecord + namedRegisterRecord + regionalRegisterRecord +
              namedRegionalRegisterRecord + failedRecord);

  // Cleanup is deliberately gated so induced delete/resign records cannot
  // obscure the five records owned by this focused overload matrix.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(regionalObject, regionalPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
      namedRegionalObject, regionalPair));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(unnamedObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(namedObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(regionalObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteObjectInstance(namedRegionalObject, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded service reporting records regional object-class subscription arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[regional-object-attribute-subscription-service-report]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]") {
  TestFederateAmbassador reports;
  auto subscriber = makeRti();
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

  REQUIRE_NOTHROW(subscriber->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subscriber->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-subscription-report-subscriber", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Keep setup and the switch transition out of the selected service-report
  // sequence. The first report below is therefore the accepted §9.8 call.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  auto const soda = subscriber->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = subscriber->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = subscriber->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  auto const region = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      region, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{region},
  }};

  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda, regionalPair, true, L"High"));

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
  auto const objectClassValue = asAscii(soda.toString());
  auto const attributeValue = asAscii(flavor.toString());
  auto const regionValue = asAscii(region.toString());
  auto const associationArgumentName =
      std::string{"Collection of attribute designator set and region designator set pairs"};
  auto const subscribeRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SubscribeObjectClassAttributesWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":false},{"HLAargumentType":53,"HLAargumentName":"Optional update rate designator","HLAargumentValue":"High"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord);

  // Disable both report sinks for the suppressed invocation.  With the
  // successful §9.8 route now promoted to HLAreportServiceInvocation, turning
  // off only file output would correctly send the call through the interaction
  // sink and advance the joined-federate serial.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda, regionalPair, false));
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord);
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda, regionalPair, false));
  auto const passiveSubscribeRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SubscribeObjectClassAttributesWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]},{"HLAargumentType":6,"HLAargumentName":"Optional passive subscription indicator","HLAargumentValue":true},{"HLAargumentType":34,"HLAargumentName":"Optional update rate designator","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord + passiveSubscribeRecord);

  // Suppress the rejected invocation in this success-path regression.  The
  // dedicated failure matrix separately proves its Null/false/exception
  // report; with reporting enabled this call would correctly consume serial 2.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(
          soda,
          AttributeHandleSetRegionHandleSetPairVector{{
              flavorOnly,
              RegionHandleSet{RegionHandle{}},
      }}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText + subscribeRecord + passiveSubscribeRecord);
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(soda, regionalPair));
  auto const unsubscribeRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassAttributesWithRegions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":4,"HLAargumentName":")" +
      associationArgumentName +
      R"(","HLAargumentValue":[{"attributeHandleSet":[")" +
      attributeValue +
      R"("],"regionHandleSet":[")" +
      regionValue +
      R"("]}]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + subscribeRecord + passiveSubscribeRecord + unsubscribeRecord);

  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->deleteRegion(region));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
}
